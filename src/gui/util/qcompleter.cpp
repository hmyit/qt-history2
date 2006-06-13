/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/*!
    \class QCompleter
    \brief The QCompleter class provides completions based on a item model.

    You can use QCompleter to provide autocompletions in any Qt
    widget (e.g. QLineEdit, QComboBox or QTextEdit). When the user
    starts typing a word, QCompleter suggests possible ways of
    completing the word, based on a word list. The word list is
    provided as a QAbstractItemModel. (For simple applications, where
    the word list is static, you can use QStringListModel.)

    A QCompleter is used typically with a QLineEdit, QComboBox or a
    QTextEdit. For example,

    \code
        QStringList completions;
        completions << "alpha" << "omega" << "omicron" << "zeta";
        QLineEdit *lineEdit = new QLineEdit(this);
        QCompleter *completer = new QCompleter(completions, this);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setCompleter(completer);
    \endcode

    A QDirModel can be used to provide autocompletion of filenames. For
    example,

    \code
        QCompleter *completer = new QCompleter;
        completer->setModel(new QDirModel(completer));
        lineEdit->setCompleter(completer);
    \endcode

    To set the model on which QCompleter should operate, call
    setModel(). By default, QCompleter will attempt to match the completionPrefix()
    against the Qt::EditRole data stored in column 0 in the model case sensitively.
    This can be changed using setCompletionRole(), setCompletionColumn(),
    and setCaseSensitivity().

    If the model is sorted on the column and role that are used for completion,
    you can call setModelSorting() with either QtCompletor::CaseSensitivelySortedModel
    or QCompleter::CaseInsensitivelySortedModel as the argument. On large models,
    this can lead to significant performance improvements, because QCompleter can
    then use binary search instead of linear search.

    The model can be a \l{QAbstractListModel}{list model},
    a \l{QAbstractTableModel}{table model}, or a
    \l{QAbstractItemModel}{tree model}. Completion on tree models
    is slightly more involved and is covered in the \l{Handling
    Tree Models} section below.

    The completionMode() determines the mode used to provide completions to the user.

    \tableofcontents
    \section1 Iterating through completions

    To retrieve a single candidate string, call setCompletionPrefix() with the text
    that needs to be completed and then use currentCompletion(). You can use setCurrentRow()
    and currentCompletion() to navigate to and access any completion.  You can iterate
    through the list of completions as below:

    \code
        for (int i = 0; completer->setCurrentRow(i); i++)
            qDebug() << completer->currentCompletion() << " is match number " << i;
    \endcode

    completionCount() returns the total number of completions for the current prefix.
    Using completionCount() must be avoided, if posible, since it requires a scan of
    the entire model.

    \section1 The completion model

    completionModel() return a list model that contains all possible
    completions for the current completion prefix, in the order in which
    they appear in the model. Calling setCompletionPrefix() automatically
    refreshes the completion model.

    \section1 Handling Tree Models

    QCompleter can look for completions in tree models, assuming
    that any item (or sub-item or sub-sub-item) can be unambiguously
    represented as a string by specifying the path to the item. The
    completion is then performed one level at a time.

    Let's take the example of a user typing in a file system path.
    The model is a (hierarchical) QDirModel. The completion
    occurs for every element in the path. For example, if the current
    text is \c C:\Wind, QCompleter might suggest \c Windows to
    complete the current path element. Similarly, if the current text
    is \c C:\Windows\Sy, QCompleter might suggest \c System.

    For this kind of completion to work, QCompleter needs to be able to
    split the path into a list of strings that are matched at each level.
    For \c C:\Windows\Sy, it needs to be split as "C:", "Windows" and "Sy".
    The default implementation of splitPath(), splits the completionPrefix
    using QDir::separator() if the model is a QDirModel.

    To provide completions, QCompleter needs to know the path from an index. This
    is provided by pathFromIndex().The default implementation, returns the data for
    the completionRole() for list models and the absolute file path if the mode
    is a QDirModel.
*/

#include "qcompleter_p.h"
#include "QtGui/qscrollbar.h"
#include "QtGui/qstringlistmodel.h"
#include "QtGui/qdirmodel.h"
#include "QtGui/qheaderview.h"
#include "QtGui/qlistview.h"
#include "QtGui/qapplication.h"
#include "QtGui/qevent.h"
#include "QtGui/qheaderview.h"
#include "QtGui/qdesktopwidget.h"

void QCompletionModel::setSourceModel(QAbstractItemModel *source)
{
    if (model)
        QObject::disconnect(model, 0, this, 0);

    QAbstractProxyModel::setSourceModel(source);
    model = sourceModel();

    // TODO: Optimize updates in the source model
    connect(model, SIGNAL(modelReset()), this, SLOT(invalidate()));
    connect(model, SIGNAL(layoutChanged()), this, SLOT(invalidate()));
    connect(model, SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SLOT(invalidate()));
    connect(model, SIGNAL(rowsRemoved(const QModelIndex&, int, int)), this, SLOT(invalidate()));
    connect(model, SIGNAL(columnsInserted(const QModelIndex&, int, int)), this, SLOT(invalidate()));
    connect(model, SIGNAL(columnsRemoved(const QModelIndex&, int, int)), this, SLOT(invalidate()));
    connect(model, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(invalidate()));

    invalidate();
}

void QCompletionModel::createEngine()
{
    bool sortedEngine = false;
    switch (c->sorting) {
    case QCompleter::UnsortedModel:
        sortedEngine = false;
        break;
    case QCompleter::CaseSensitivelySortedModel:
        sortedEngine = c->cs == Qt::CaseSensitive;
        break;
    case QCompleter::CaseInsensitivelySortedModel:
        sortedEngine = c->cs == Qt::CaseInsensitive;
        break;
    }

    delete engine;
    if (sortedEngine)
        engine = new QSortedModelEngine(c);
    else
        engine = new QUnsortedModelEngine(c);
}

QModelIndex QCompletionModel::mapToSource(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    int row;
    QModelIndex parent = engine->curParent;
    if (!showAll) {
        if (!engine->matchCount())
            return QModelIndex();
        Q_ASSERT(index.row() < engine->matchCount());
        QIndexMapper& rootIndices = engine->historyMatch.indices;
        if (index.row() < rootIndices.count()) {
            row = rootIndices[index.row()];
            parent = QModelIndex();
        } else {
            row = engine->curMatch.indices[index.row() - rootIndices.count()];
        }
    } else {
        row = index.row();
    }

    return model->index(row, index.column(), parent);
}

QModelIndex QCompletionModel::mapFromSource(const QModelIndex& idx) const
{
    if (!idx.isValid())
        return QModelIndex();

    int row = -1;
    if (!showAll) {
        if (!engine->matchCount())
            return QModelIndex();

        QIndexMapper& rootIndices = engine->historyMatch.indices;
        if (idx.parent().isValid()) {
            if (idx.parent() != engine->curParent)
                return QModelIndex();
        } else {
            row = rootIndices.indexOf(idx.row());
            if (row == -1 && engine->curParent.isValid())
                return QModelIndex(); // source parent and our parent dont match
        }

        if (row == -1) {
            QIndexMapper& indices = engine->curMatch.indices;
            engine->filterOnDemand(idx.row() - indices.last());
            row = indices.indexOf(idx.row()) + rootIndices.count();
        }

        if (row == -1)
            return QModelIndex();
    } else {
        if (idx.parent() != engine->curParent)
            return QModelIndex();
        row = idx.row();
    }

    return createIndex(row, idx.column());
}

bool QCompletionModel::setCurrentRow(int row)
{
    if (row < 0 || !engine->matchCount())
        return false;

    if (row >= engine->matchCount())
        engine->filterOnDemand(row + 1 - engine->matchCount());

    if (row >= engine->matchCount()) // invalid row
        return false;

    engine->curRow = row;
    return true;
}

QModelIndex QCompletionModel::currentIndex(bool sourceIndex) const
{
    if (!engine->matchCount())
        return QModelIndex();

    int row = engine->curRow;
    if (showAll)
        row = engine->curMatch.indices[engine->curRow];

    QModelIndex idx = createIndex(row, c->column);
    if (!sourceIndex)
        return idx;
    return mapToSource(idx);
}

QModelIndex QCompletionModel::index(int row, int column, const QModelIndex& parent) const
{
    if (row < 0 || column < 0 || column >= columnCount(parent) || parent.isValid())
        return QModelIndex();

    if (!showAll) {
        if (!engine->matchCount())
            return QModelIndex();
        if (row >= engine->historyMatch.indices.count()) {
            int want = row + 1 - engine->matchCount();
            if (want > 0)
                engine->filterOnDemand(want);
            if (row >= engine->matchCount())
                return QModelIndex();
        }
    } else {
        if (row >= model->rowCount(engine->curParent))
            return QModelIndex();
    }

    return createIndex(row, column);
}

int QCompletionModel::completionCount() const
{
    if (!engine->matchCount())
        return 0;

    engine->filterOnDemand(INT_MAX);
    return engine->matchCount();
}

int QCompletionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    if (showAll) {
        // Show all items below current parent, even if we have no valid matches
        if (engine->curParts.count() != 1  && !engine->matchCount()
            && !engine->curParent.isValid())
            return 0;
        return model->rowCount(engine->curParent);
    }

    return completionCount();
}

void QCompletionModel::setFiltered(bool filtered)
{
    if (showAll == !filtered)
        return;
    showAll = !filtered;
    reset();
}

bool QCompletionModel::hasChildren(const QModelIndex &parent) const
{
    if (parent.isValid())
        return false;

    if (showAll)
        return model->hasChildren(mapToSource(parent));

    if (!engine->matchCount())
        return false;

    return true;
}

QVariant QCompletionModel::data(const QModelIndex& index, int role) const
{
    return model->data(mapToSource(index), role);
}

void QCompletionModel::invalidate()
{
    engine->cache.clear();
    filter(engine->curParts);
}

void QCompletionModel::filter(const QStringList& parts)
{
    engine->filter(parts);
    reset();
}

//////////////////////////////////////////////////////////////////////////////
void QCompletionEngine::filter(const QStringList& parts)
{
    const QAbstractItemModel *model = c->proxy->sourceModel();
    curParts = parts;
    if (curParts.isEmpty())
        curParts.append(QString());

    curRow = -1;
    curParent = QModelIndex();
    curMatch = QMatchData();
    historyMatch = filterHistory();

    QModelIndex parent;
    for (int i = 0; i < curParts.count() - 1; i++) {
        QString part = curParts[i];
        int emi = filter(part, parent, -1).exactMatchIndex;
        if (emi == -1)
            return;
        parent = model->index(emi, c->column, parent);
    }

    // Note that we set the curParent to a valid parent, even if we have no matches
    // When filtering is disabled, we show all the items under this parent
    curParent = parent;
    if (curParts.last().isEmpty())
        curMatch = QMatchData(QIndexMapper(0, model->rowCount(curParent) - 1), -1, false);
    else
        curMatch = filter(curParts.last(), curParent, 1); // build atleast one
    curRow = curMatch.isValid() ? 0 : -1;
}

QMatchData QCompletionEngine::filterHistory()
{
    if (curParts.count() <= 1 || c->proxy->showAll)
        return QMatchData();
    QAbstractItemModel *source = c->proxy->model;
    bool dirModel = false;
#ifndef QT_NO_DIRMODEL
    dirModel = (qobject_cast<QDirModel *>(source) != 0);
#endif
    QVector<int> v;
    QIndexMapper im(v);
    QMatchData m(im, -1, true);

    for (int i = 0; i < source->rowCount(); i++) {
        QString str = source->index(i, c->column).data().toString();
        if (str.startsWith(c->prefix, c->cs)
#ifndef Q_OS_WIN
            && (!dirModel || str != QDir::separator())
#endif
            )
            m.indices.append(i);
    }
    return m;
}

// Returns a match hint from the cache by chopping the search string
bool QCompletionEngine::matchHint(QString part, const QModelIndex& parent, QMatchData *hint)
{
    if (c->cs == Qt::CaseInsensitive)
        part = part.toLower();

    const CacheItem& map = cache[parent];

    QString key = part;
    while (!key.isEmpty()) {
        key.chop(1);
        if (map.contains(key)) {
            *hint = map[key];
            return true;
        }
    }

    return false;
}

bool QCompletionEngine::lookupCache(QString part, const QModelIndex& parent, QMatchData *m)
{
   if (c->cs == Qt::CaseInsensitive)
        part = part.toLower();
   const CacheItem& map = cache[parent];
   if (!map.contains(part))
       return false;
   *m = map[part];
   return true;
}

// When the cache size exceeds 1MB, it clears out about 1/2 of the cache.
void QCompletionEngine::saveInCache(QString part, const QModelIndex& parent, const QMatchData& m)
{
    QMatchData old = cache[parent].take(part);
    cost = cost + m.indices.cost() - old.indices.cost();
    if (cost * sizeof(int) > 1024 * 1024) {
        QMap<QModelIndex, CacheItem>::iterator it1 ;
        for (it1 = cache.begin(); it1 != cache.end(); it1++) {
            CacheItem& ci = it1.value();
            int sz = ci.count()/2;
            QMap<QString, QMatchData>::iterator it2 = ci.begin();
            for (int i = 0; it2 != ci.end() && i < sz; i++, ++it2) {
                cost -= it2.value().indices.cost();
                ci.erase(it2);
            }
            if (ci.count() == 0)
                cache.erase(it1);
        }
    }

    if (c->cs == Qt::CaseInsensitive)
        part = part.toLower();
    cache[parent][part] = m;
}

///////////////////////////////////////////////////////////////////////////////////
QIndexMapper QSortedModelEngine::indexHint(QString part, const QModelIndex& parent)
{
    const QAbstractItemModel *model = c->proxy->sourceModel();

    if (c->cs == Qt::CaseInsensitive)
        part = part.toLower();

    const CacheItem& map = cache[parent];

    // Try to find a lower and upper bound for the search from previous results
    int to = model->rowCount(parent) - 1;
    int from = 0;
    const CacheItem::const_iterator it = map.lowerBound(part);

    // look backward for first valid hint
    for(CacheItem::const_iterator it1 = it; it1-- != map.constBegin();) {
        const QMatchData& value = it1.value();
        if (value.isValid()) {
            from = value.indices.last() + 1;
            break;
        }
    }

    // look forward for first valid hint
    for(CacheItem::const_iterator it2 = it; it2 != map.constEnd(); ++it2) {
        const QMatchData& value = it2.value();
        if (value.isValid() && !it2.key().startsWith(part)) {
            to = value.indices[0] - 1;
            break;
        }
    }

    return QIndexMapper(from, to);
}

QMatchData QSortedModelEngine::filter(const QString& part, const QModelIndex& parent, int)
{
    const QAbstractItemModel *model = c->proxy->sourceModel();

    QMatchData hint;
    if (lookupCache(part, parent, &hint))
        return hint;

    QIndexMapper indices;

    if (matchHint(part, parent, &hint)) {
        if (!hint.isValid())
            return QMatchData();
        indices = hint.indices;
    } else {
        indices = indexHint(part, parent);
    }

    // binary search the model within 'indices' for 'part' under 'parent'
    int high = indices.to() + 1;
    int low = indices.from() - 1;
    int probe;
    QModelIndex probeIndex;
    QString probeData;

    while (high - low > 1)
    {
        probe = (high + low) / 2;
        probeIndex = model->index(probe, c->column, parent);
        probeData = model->data(probeIndex, c->role).toString();
        if (QString::compare(probeData, part, c->cs) >= 0)
            high = probe;
        else
            low = probe;
    }

    if (low == indices.to()) { // not found
        saveInCache(part, parent, QMatchData());
        return QMatchData();
    }

    probeIndex = model->index(low + 1, c->column, parent);
    probeData = model->data(probeIndex, c->role).toString();
    if (!probeData.startsWith(part, c->cs)) {
        saveInCache(part, parent, QMatchData());
        return QMatchData();
    }

    int emi = QString::compare(probeData, part, c->cs) == 0 ? low+1 : -1;

    int from = low + 1;
    high = indices.to() + 1;
    low = from;

    while (high - low > 1)
    {
        probe = (high + low) / 2;
        probeIndex = model->index(probe, c->column, parent);
        probeData = model->data(probeIndex, c->role).toString();
        if (probeData.startsWith(part, c->cs))
            low = probe;
        else
            high = probe;
    }

    QMatchData m(QIndexMapper(from, high - 1), emi, false);
    saveInCache(part, parent, m);
    return m;
}

////////////////////////////////////////////////////////////////////////////////////////
int QUnsortedModelEngine::buildIndices(const QString& str, const QModelIndex& parent, int n,
                                      const QIndexMapper& indices, QMatchData* m)
{
    Q_ASSERT(m->partial);
    Q_ASSERT(n != -1 || m->exactMatchIndex == -1);
    const QAbstractItemModel *model = c->proxy->sourceModel();
    int i, count = 0;

    for (i = 0; i < indices.count() && count != n; ++i) {
        QModelIndex idx = model->index(indices[i], c->column, parent);
        QString data = model->data(idx, c->role).toString();
        if (!data.startsWith(str, c->cs))
            continue;
        m->indices.append(indices[i]);
        ++count;
        if (m->exactMatchIndex == -1 && QString::compare(data, str, c->cs) == 0) {
            m->exactMatchIndex = indices[i];
            if (n == -1)
                return indices[i];
        }
    }
    return indices[i-1];
}

void QUnsortedModelEngine::filterOnDemand(int n)
{
    Q_ASSERT(matchCount());
    if (!curMatch.partial)
        return;
    Q_ASSERT(n >= -1);
    const QAbstractItemModel *model = c->proxy->sourceModel();
    int lastRow = model->rowCount(curParent) - 1;
    QIndexMapper im(curMatch.indices.last() + 1, lastRow);
    int lastIndex = buildIndices(curParts.last(), curParent, n, im, &curMatch);
    curMatch.partial = (lastRow != lastIndex);
    saveInCache(curParts.last(), curParent, curMatch);
}

QMatchData QUnsortedModelEngine::filter(const QString& part, const QModelIndex& parent, int n)
{
    QMatchData hint;

    QVector<int> v;
    QIndexMapper im(v);
    QMatchData m(im, -1, true);

    const QAbstractItemModel *model = c->proxy->sourceModel();
    bool foundInCache = lookupCache(part, parent, &m);

    if (!foundInCache) {
        if (matchHint(part, parent, &hint) && !hint.isValid())
            return QMatchData();
    }

    if (!foundInCache && !hint.isValid()) {
        const int lastRow = model->rowCount(parent) - 1;
        QIndexMapper all(0, lastRow);
        int lastIndex = buildIndices(part, parent, n, all, &m);
        m.partial = (lastIndex != lastRow);
    } else {
        if (!foundInCache) { // build from hint as much as we can
            buildIndices(part, parent, INT_MAX, hint.indices, &m);
            m.partial = hint.partial;
        }
        if (m.partial && ((n == -1 && m.exactMatchIndex == -1) || (m.indices.count() < n))) {
            // need more and have more
            const int lastRow = model->rowCount(parent) - 1;
            QIndexMapper rest(hint.indices.last() + 1, lastRow);
            int want = n == -1 ? -1 : n - m.indices.count();
            int lastIndex = buildIndices(part, parent, want, rest, &m);
            m.partial = (lastRow != lastIndex);
        }
    }

    saveInCache(part, parent, m);
    return m;
}

///////////////////////////////////////////////////////////////////////////////
QCompleterPrivate::QCompleterPrivate()
: widget(0), proxy(0), popup(0), cs(Qt::CaseSensitive), role(Qt::EditRole), column(0),
  sorting(QCompleter::UnsortedModel)
{
}

void QCompleterPrivate::init(QAbstractItemModel *m)
{
    Q_Q(QCompleter);
    proxy = new QCompletionModel(this, q);
    q->setModel(m);
    QListView *listView = new QListView;
    listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    listView->setSelectionMode(QAbstractItemView::SingleSelection);
    listView->setModelColumn(column);
    q->setPopup(listView);
    q->setCompletionMode(QCompleter::PopupCompletion);
}

void QCompleterPrivate::setCurrentIndex(const QModelIndex& index, bool select)
{
    if (!select)
        popup->selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
    else {
        if (!index.isValid())
            popup->selectionModel()->clear();
        else
            popup->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select
                                                            | QItemSelectionModel::Rows);
    }
    if (!index.isValid())
        popup->scrollToTop();
    else
        popup->scrollTo(index, QAbstractItemView::PositionAtTop);
}

void QCompleterPrivate::_q_completionSelected(const QItemSelection& selection)
{
    QModelIndex index;
    if (!selection.indexes().isEmpty())
        index = selection.indexes().first();

    _q_complete(index, true);
}

void QCompleterPrivate::_q_complete(QModelIndex index, bool highlighted)
{
    Q_Q(QCompleter);
    QString completion;
    if (!index.isValid())
        completion = prefix;
    else {
        index = proxy->mapToSource(index);
        index = index.sibling(index.row(), column); // for clicked()
        completion = q->pathFromIndex(index);
#ifndef QT_NO_DIRMODEL
        // add a trailing separator in inline
        if (mode == QCompleter::InlineCompletion) {
            if (qobject_cast<QDirModel *>(proxy->sourceModel()) && QFileInfo(completion).isDir())
                completion += QDir::separator();
        }
#endif
    }

    if (highlighted) {
        emit q->highlighted(index);
        emit q->highlighted(completion);
    } else {
        emit q->activated(index);
        emit q->activated(completion);
    }
}

// FIXME: Does not work for RTL
void QCompleterPrivate::showPopup(const QRect& rect)
{
    const QRect screen = QApplication::desktop()->availableGeometry(widget);
    Qt::LayoutDirection dir = widget->layoutDirection();
    QPoint pos;
    int rw, rh, w;
    int h = (popup->sizeHintForRow(0) * qMin(7, popup->model()->rowCount()) + 3) + 3;
    if (rect.isValid()) {
        int sbw = popup->horizontalScrollBar()->sizeHint().width();
        w = popup->sizeHintForColumn(column) + sbw;
        rh = rect.height();
        rw = rect.width() + w;
        pos = widget->mapToGlobal(dir == Qt::RightToLeft ? rect.bottomRight() : rect.bottomLeft());
    } else {
        rh = widget->height();
        rw = widget->width();
        pos = widget->mapToGlobal(QPoint(0, widget->height() - 2));
        w = widget->width();
    }

    if ((pos.x() + rw) > (screen.x() + screen.width()))
        pos.setX(screen.x() + screen.width() - w);
    if (pos.x() < screen.x())
        pos.setX(screen.x());
    if (((pos.y() + rh) > (screen.y() + screen.height())) && ((pos.y() - h - rh) >= 0))
        pos.setY(pos.y() - qMax(h, popup->minimumHeight()) - rh + 2);

    popup->setGeometry(pos.x(), pos.y(), w, h);

    if (!popup->isVisible())
        popup->show();
}

/*!
    Constructs a QCompleter object with the given \a parent.
*/
QCompleter::QCompleter(QObject *parent)
: QObject(*new QCompleterPrivate(), parent)
{
    Q_D(QCompleter);
    d->init();
}

/*!
    Constructs a QCompleter object that provides completions from \a model
    and with the given \a parent.
*/
QCompleter::QCompleter(QAbstractItemModel *model, QObject *parent)
    : QObject(*new QCompleterPrivate(), parent)
{
    Q_D(QCompleter);
    d->init(model);
}

/*!
    Constructs a QCompleter object with provides \a list as the possible
    completions and with the given \a parent.
*/
QCompleter::QCompleter(const QStringList& list, QObject *parent)
: QObject(*new QCompleterPrivate(), parent)
{
    Q_D(QCompleter);
    d->init(new QStringListModel(list, this));
}

/*!
    Destroys the QCompleter object.
*/
QCompleter::~QCompleter()
{
}

/*!
    Sets the widget for which completion are provided for to \a widget. The
    widget is set automatically when a QCompleter is set on a QLineEdit using
    QLineEdit::setCompleter(). The widget needs to be set explicity when
    providing completions for custom widgets.
 */
void QCompleter::setWidget(QWidget *widget)
{
    Q_D(QCompleter);
    d->widget = widget;
    setCompletionMode(d->mode); // will install event filter depending on mode
}

/*!
    Returns the widget for which QCompleter is providing completions.
 */
QWidget *QCompleter::widget() const
{
    Q_D(const QCompleter);
    return d->widget;
}

/*!
    Sets the model which provides completions to \a model. The \a model can
    be list model or a tree model. If a model has been already previously set 
    and it has the QCompleter as its parent, it is deleted.

    For convenience, if \a model is a QDirModel, QCompleter switches its 
    caseSensitivity to Qt::CaseInsensitive on Windows and Qt::CaseSensitive
    on other platforms.

    \sa completionModel(), modelSorting, {Handling Tree Models}
*/
void QCompleter::setModel(QAbstractItemModel *model)
{
    Q_D(QCompleter);
    QAbstractItemModel *oldModel = d->proxy->model;
    d->proxy->setSourceModel(model);
    if (d->popup)
        setPopup(d->popup); // set the model and make new connections
    if (oldModel && oldModel->QObject::parent() == this)
        delete oldModel;
#ifndef QT_NO_DIRMODEL
    if (qobject_cast<QDirModel *>(model)) {
#ifdef Q_OS_WIN
        setCaseSensitivity(Qt::CaseInsensitive);
#else
        setCaseSensitivity(Qt::CaseSensitive);
#endif
    }
#endif // QT_NO_DIRMODEL
}

/*!
    Returns the model that provides completion strings.

    \sa completionModel()
*/
QAbstractItemModel *QCompleter::model() const
{
    Q_D(const QCompleter);
    return d->proxy->sourceModel();
}

/*!
    \enum QCompleter::CompletionMode

    This enum specifies how completions are provided to the user.

    \value PopupCompletion                Displays a popup contains the current completions
    \value InlineCompletion               Completions appear inline
    \value UnfilteredPopupCompletion      Displays a popup containing all the possible completions \
                                          with the most likely suggestion selected in the popup.
*/

/*!
    \property QCompleter::completionMode
    \brief how the completions are provided to the user

    The default value is QCompleter::PopupCompletion.
*/
void QCompleter::setCompletionMode(QCompleter::CompletionMode mode)
{
    Q_D(QCompleter);

    d->mode = mode;
    d->proxy->setFiltered(mode != QCompleter::UnfilteredPopupCompletion);

    if (mode == QCompleter::InlineCompletion) {
        if (d->widget)
            d->widget->removeEventFilter(this);
        return;
    }

    if (d->widget)
        d->widget->installEventFilter(this);
}

QCompleter::CompletionMode QCompleter::completionMode() const
{
    Q_D(const QCompleter);
    return d->mode;
}

/*!
    Sets the popup used to display completions to \a popup. QCompleter takes ownership
    of the view.

    A QListView is automatically created when the completionMode() is set to
    QCompleter::PopupCompletion or QCompleter::UnfilteredPopupCompletion. The default
    popup displays the completionColumn().

    Ensure that this function is called before the view settings are modified. This is
    required since view's properties may require that a model has been set on the view
    (for example, hiding columns in the view requires a model to be set on the view).
*/
void QCompleter::setPopup(QAbstractItemView *popup)
{
    Q_D(QCompleter);
    Q_ASSERT(popup != 0);
    if (d->popup)
        QObject::disconnect(d->popup, 0, this, 0);
    if (d->popup != popup)
        delete d->popup;
    if (popup->model() != d->proxy)
        popup->setModel(d->proxy);
    popup->hide();
    popup->setParent(0, Qt::Popup);
    popup->setFocusPolicy(Qt::NoFocus);
    popup->installEventFilter(this);
    popup->setItemDelegate(new QCompleterItemDelegate(popup));

    QObject::connect(popup, SIGNAL(clicked(const QModelIndex&)),
                     this, SLOT(_q_complete(QModelIndex)));
    QObject::connect(popup, SIGNAL(clicked(const QModelIndex&)), popup, SLOT(hide()));

    QObject::connect(popup->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
                     this, SLOT(_q_completionSelected(const QItemSelection&)));
    d->popup = popup;
}

/*!
    Returns the popup used to display completions.

    \sa setPopup()
*/
QAbstractItemView *QCompleter::popup() const
{
    Q_D(const QCompleter);
    return d->popup;
}

/*!
  \reimp
*/
bool QCompleter::event(QEvent *ev)
{
    return QObject::event(ev);
}

/*!
  \reimp
*/
bool QCompleter::eventFilter(QObject *o, QEvent *e)
{
    Q_D(QCompleter);

    if (o == d->widget && e->type() == QEvent::FocusOut) {
        if (d->popup && d->popup->isVisible())
            return true;
    }

    if (o != d->popup)
        return QObject::eventFilter(o, e);

    switch (e->type()) {
    case QEvent::KeyPress: {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        QModelIndex curIndex = d->popup->currentIndex();
        QModelIndexList selList = d->popup->selectionModel()->selectedIndexes();

        const int key = ke->key();
        if ((key == Qt::Key_Up || key == Qt::Key_Down) && selList.isEmpty() && curIndex.isValid()
            && d->mode == QCompleter::UnfilteredPopupCompletion) {
              d->setCurrentIndex(curIndex);
              return true;
        }

        switch (key) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            d->popup->hide();
            if (curIndex.isValid())
                d->_q_complete(curIndex);
            break; // propagate the event

        case Qt::Key_Tab:
            d->popup->hide();
            d->_q_complete(curIndex);
            if (d->mode == QCompleter::PopupCompletion || !selList.isEmpty())
                break;
            return true;

        case Qt::Key_Backtab:
            d->popup->hide();
            break;

        case Qt::Key_F4:
            if (ke->modifiers() & Qt::AltModifier) {
                d->popup->hide();
                return true;
            }
            break;

        case Qt::Key_Escape:
            d->popup->hide();
            return true;

        case Qt::Key_End:
        case Qt::Key_Home:
            if (ke->modifiers() & Qt::ControlModifier)
                return false;
            break;

        case Qt::Key_Up:
            if (!curIndex.isValid()) {
                int rowCount = d->proxy->rowCount();
                QModelIndex lastIndex = d->proxy->index(rowCount - 1, 0);
                d->setCurrentIndex(lastIndex);
                return true;
            } else if (curIndex.row() == 0) {
                d->setCurrentIndex(QModelIndex());
                return true;
            }
            return false;

        case Qt::Key_Down:
            if (!curIndex.isValid()) {
                QModelIndex firstIndex = d->proxy->index(0, 0);
                d->setCurrentIndex(firstIndex);
                return true;
            } else if (curIndex.row() == d->proxy->rowCount() - 1) {
                d->setCurrentIndex(QModelIndex());
                return true;
            }
            return false;

        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            return false;

        default:
            break;
        }

        QApplication::sendEvent(d->widget, ke);
        return true;
    }

    case QEvent::MouseButtonPress:
        if (!d->popup->underMouse()) {
            d->popup->hide();
            return true;
        }
        return false;

    default:
        return false;
    }
}

/*!
    For QCompleter::PopupCompletion andr QCompletion::UnfilteredPopupCompletion modes,
    calling this function displays the popup displaying the current completions. By default,
    if \a rect is not specified, the popup is displayed on the bottom of the widget().
    If \a rect is specified the popup is displayed on the left edge of the rectangle.

    For QCompleter::InlineCompletion mode, the highlighted() signal is fired with
    the current completion.
*/
void QCompleter::complete(const QRect& rect)
{
    Q_D(QCompleter);
    QModelIndex idx = d->proxy->currentIndex(false);
    if (d->mode == QCompleter::InlineCompletion) {
        if (idx.isValid())
            d->_q_complete(idx, true);
        return;
    }

    Q_ASSERT(d->widget != 0);
    if ((d->mode == QCompleter::PopupCompletion && !idx.isValid())
        || (d->mode == QCompleter::UnfilteredPopupCompletion && d->proxy->rowCount() == 0)) {
        d->popup->hide(); // no suggestion, hide
        return;
    }

    if (d->mode == QCompleter::UnfilteredPopupCompletion)
        d->setCurrentIndex(idx, false);

    d->showPopup(rect);
}

/*!
    Sets the current row to \a row. This function may be used along with
    currentCompletion() to iterate through all the possible completions.

    \sa currentCompletion(), completionCount()
*/
bool QCompleter::setCurrentRow(int row)
{
    Q_D(QCompleter);
    return d->proxy->setCurrentRow(row);
}

/*!
    Returns the current row.

    \sa setCurrentRow()
*/
int QCompleter::currentRow() const
{
    Q_D(const QCompleter);
    return d->proxy->currentRow();
}

/*!
    Returns the number of completions for the current prefix. For an unsorted model with
    a large number of items this can be expensive. Use setCurrentRow() and
    currentCompletion() to iterate through all the completions.
*/
int QCompleter::completionCount() const
{
    Q_D(const QCompleter);
    return d->proxy->completionCount();
}

/*!
    \enum QCompleter::ModelSorting

    This enum specifies how the items in the model is sorted.

    \value UnsortedModel                    The model is unsorted.
    \value CaseSensitivelySortedModel       The model is sorted case sensitively.
    \value CaseInsensitivelySortedModel     The model is sorted case insensitively.

    \sa completionRole, completionColumn
*/

/*!
    \property QCompleter::modelSorting
    \brief whether and how the model is sorted

    By default, no assumptions are made about the order of the items
    in the model that provides the completions. If the model's data
    for the matchColumn() and matchRole() is sorted in ascending order, you
    can set this property to CaseSensitivelySortedModel or
    CaseInsensitivelySortedModel. On large models, this can lead to
    significant performance improvements, because QCompleter can
    then use binary search instead of linear search. (Be aware that
    these performance improvements cannot take place when
    QCompleter's caseSensitivity and the model's sorting case
    differ.)

    \sa setCaseSensitivity(), QCompleter::ModelSorting
*/
void QCompleter::setModelSorting(QCompleter::ModelSorting sorting)
{
    Q_D(QCompleter);
    if (d->sorting == sorting)
        return;
    d->sorting = sorting;
    d->proxy->createEngine();
    d->proxy->invalidate();
}

QCompleter::ModelSorting QCompleter::modelSorting() const
{
    Q_D(const QCompleter);
    return d->sorting;
}

/*!
    \property QCompleter::completionColumn
    \brief the column in the model in which completions are searched for.

    If the popup() is a QListView, it is automatically setup to display
    this column.

    By default, the match column is 0.

    \sa completionRole, caseSensitivity
*/
void QCompleter::setCompletionColumn(int column)
{
    Q_D(QCompleter);
    if (d->column == column)
        return;
    if (QListView *listView = qobject_cast<QListView *>(d->popup))
        listView->setModelColumn(column);
    d->column = column;
    d->proxy->invalidate();
}

int QCompleter::completionColumn() const
{
    Q_D(const QCompleter);
    return d->column;
}

/*!
    \property QCompleter::completionRole
    \brief the item role to be used to query the contents of items for matching.

    The default role is Qt::EditRole.

    \sa completionColumn, caseSensitivity
*/
void QCompleter::setCompletionRole(int role)
{
    Q_D(QCompleter);
    if (d->role == role)
        return;
    d->role = role;
    d->proxy->invalidate();
}

int QCompleter::completionRole() const
{
    Q_D(const QCompleter);
    return d->role;
}

/*!
    \property QCompleter::caseSensitivity
    \brief the case sensitivity of the matching

    The default is Qt::CaseSensitive.

    \sa completionColumn, completionRole, modelSorting
*/
void QCompleter::setCaseSensitivity(Qt::CaseSensitivity cs)
{
    Q_D(QCompleter);
    if (d->cs == cs)
        return;
    d->cs = cs;
    d->proxy->createEngine();
    d->proxy->invalidate();
}

Qt::CaseSensitivity QCompleter::caseSensitivity() const
{
    Q_D(const QCompleter);
    return d->cs;
}

/*!
    \property QCompleter::completionPrefix
    \brief the completion prefix used to provide completions.

    The completionModel() is updated to reflect the list of possible
    matches for \a prefix.
*/
void QCompleter::setCompletionPrefix(const QString &prefix)
{
    Q_D(QCompleter);
    d->prefix = prefix;
    d->proxy->filter(splitPath(prefix));
}

QString QCompleter::completionPrefix() const
{
    Q_D(const QCompleter);
    return d->prefix;
}

/*!
    Returns the current completion index.

    \sa setCurrentRow(), currentCompletion()
*/
QModelIndex QCompleter::currentIndex() const
{
    Q_D(const QCompleter);
    return d->proxy->currentIndex(true);
}

/*!
    Returns the current completion string. This includes the completionPrefix.
    When used alongside setCurrentRow(), it can be used to iterate through
    all the matches.

    \sa setCurrentRow(), currentIndex()
*/
QString QCompleter::currentCompletion() const
{
    return pathFromIndex(currentIndex());
}

/*!
    Returns the completion model. The completion model is a list model that
    contains all the possible matches for the current completion prefix.
    The completion model is auto-updated to reflect the current completions.

    \sa completionPrefix, model()
*/
const QAbstractProxyModel *QCompleter::completionModel() const
{
    Q_D(const QCompleter);
    return d->proxy;
}

/*!
    Returns the path from index \a index. QCompleter uses this to provide
    the completion text for the index \a index.

    The default implementation returns the Qt::Edit role of the item for
    list models. It returns the absolute file path if the model is a QDirModel.

    \sa splitPath()
*/
QString QCompleter::pathFromIndex(const QModelIndex& index) const
{
    Q_D(const QCompleter);
    if (!index.isValid())
        return QString();

    QAbstractItemModel *sourceModel = d->proxy->sourceModel();
#ifndef QT_NO_DIRMODEL
    QDirModel *dirModel = qobject_cast<QDirModel *>(sourceModel);
    if (!dirModel)
#endif
        return sourceModel->data(index, d->role).toString();

    QModelIndex idx = index;
    QStringList list;
    do {
        QString t = sourceModel->data(idx, Qt::EditRole).toString();
        list.prepend(t);
        QModelIndex parent = idx.parent();
        idx = parent.sibling(parent.row(), index.column());
    } while (idx.isValid());

#ifndef Q_OS_WIN
    if (list.count() == 1) // only the separator or some other text
        return list[0];
    list[0].clear() ; // the join below will provide the separator
#endif

    return list.join(QDir::separator());
}

/*!
    Splits \a path into strings that are used to match at each level in the model().
    The default implementation of splitPath() splits a file system path based on
    QDir::separator() when the sourceModel() is a QDirModel.

    When used with list models, the first item in the returned list is used for matching.

    \sa pathFromIndex(), {Handling Tree Models}
*/
QStringList QCompleter::splitPath(const QString& path) const
{
    Q_D(const QCompleter);
    bool isDirModel = false;
#ifndef QT_NO_DIRMODEL
    isDirModel = qobject_cast<QDirModel *>(d->proxy->sourceModel()) != 0;
#endif

    if (!isDirModel || path.isEmpty())
        return QStringList(completionPrefix());

    QString pathCopy = QDir::convertSeparators(path);
    QString sep = QDir::separator();
#ifdef Q_OS_WIN
    if (pathCopy == QLatin1String("\\") || pathCopy == QLatin1String("\\\\"))
        return QStringList(pathCopy);
    QString doubleSlash(QLatin1String("\\\\"));
    if (pathCopy.startsWith(doubleSlash))
        pathCopy = pathCopy.mid(2);
    else
        doubleSlash.clear();
#endif

    QRegExp re(QLatin1String("[") + QRegExp::escape(sep) + QLatin1String("]"));
    QStringList parts = pathCopy.split(re);

#ifdef Q_OS_WIN
    if (!doubleSlash.isEmpty())
        parts[0].prepend(doubleSlash);
#else
    if (path[0] == sep[0]) // readd the "/" at the beginning as the split removed it
        parts[0] = sep[0];
#endif

    return parts;
}

/*!
    \fn void QCompleter::activated(const QModelIndex& index)

    This signal is sent when an item in the popup() is activated by the user.
    (by clicking or pressing return). The item's \a index is given.

*/

/*!
    \fn void QCompleter::activated(const QString &text)

    This signal is sent when an item in the popup() is activated by the user (by
    clicking or pressing return). The item's \a text is given.

*/

/*!
    \fn void QCompleter::highlighted(const QModelIndex& index)

    This signal is sent when an item in the popup() is highlighted by
    the user. It is also sent if complete() is called with the completionMode()
    set to QCompleter::InlineCompletion. The item's \a index is given.
*/

/*!
    \fn void QCompleter::highlighted(const QString &text)

    This signal is sent when an item in the popup() is highlighted by
    the user. It is also sent if complete() is called with the completionMode()
    set to QCOmpleter::InlineCompletion. The item's \a text is given.
*/

#include "moc_qcompleter.cpp"
