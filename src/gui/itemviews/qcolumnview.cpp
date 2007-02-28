/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
****************************************************************************/

#ifndef QT_NO_QCOLUMNVIEW

#include "qcolumnview.h"
#include "qcolumnview_p.h"
#include "qcolumnviewgrip_p.h"

#include <qlistview.h>
#include <qabstractitemdelegate.h>
#include <qscrollbar.h>
#include <qpainter.h>
#include <qdebug.h>
#include <qpainterpath.h>

#define ANIMATION_DURATION_MSEC 150

/*!
    \class QColumnView
    \brief The QColumnView class provides a model/view implementation of a column view.
    \ingroup model-view
    \mainclass

    QColumnView displays a model in a number of QListViews, one for each
    hierarchy in the tree.  This is sometimes referred to as a cascading list.

    The QColumnView class is one of the \l{Model/View Classes}
    and is part of Qt's \l{Model/View Programming}{model/view framework}.

    QColumnView implements the interfaces defined by the
    QAbstractItemView class to allow it to display data provided by
    models derived from the QAbstractItemModel class.

    \sa \link model-view-programming.html Model/View Programming\endlink
*/

/*!
    Constructs a column view with a \a parent to represent a model's
    data. Use setModel() to set the model.

    \sa QAbstractItemModel
*/
QColumnView::QColumnView(QWidget * parent)
:  QAbstractItemView(*new QColumnViewPrivate, parent)
{
    Q_D(QColumnView);
    setTextElideMode(Qt::ElideMiddle);
    connect(&(d->currentAnimation), SIGNAL(frameChanged(int)), horizontalScrollBar(), SLOT(setValue(int)));
    connect(&(d->currentAnimation), SIGNAL(finished()), this, SLOT(_q_changeCurrentColumn()));
    delete d->itemDelegate;
    setItemDelegate(new QColumnViewDelegate(this));
}

/*!
    \internal
*/
QColumnView::QColumnView(QColumnViewPrivate & dd, QWidget * parent)
:  QAbstractItemView(dd, parent)
{
}

/*!
    Destroys the column view.
*/
QColumnView::~QColumnView()
{
}

/*!
    \property QColumnView::resizeGripsVisible
    \brief the way to specify if the list views gets resize grips or not

    By default, \c visible is set to true

    \sa setRootIndex()
*/
void QColumnView::setResizeGripsVisible(bool visible)
{
    Q_D(QColumnView);
    if (d->showResizeGrips == visible)
        return;
    d->showResizeGrips = visible;
    for (int i = 0; i < d->columns.count(); ++i) {
        QAbstractItemView *view = d->columns[i];
        if (visible) {
            QColumnViewGrip *grip = new QColumnViewGrip(view);
            view->setCornerWidget(grip);
            connect(grip, SIGNAL(gripMoved(int)), this, SLOT(_q_gripMoved(int)));
        } else {
            QWidget *widget = view->cornerWidget();
            view->setCornerWidget(0);
            widget->deleteLater();
        }
    }
}

bool QColumnView::resizeGripsVisible() const
{
    Q_D(const QColumnView);
    return d->showResizeGrips;
}

/*!
    \reimp
*/
void QColumnView::setModel(QAbstractItemModel *model)
{
    Q_D(QColumnView);
    d->closeColumns();
    QAbstractItemView::setModel(model);
}

/*!
    \reimp
*/
void QColumnView::setRootIndex(const QModelIndex &index)
{
    Q_D(QColumnView);
    if (!model())
        return;

    d->closeColumns();
    Q_ASSERT(d->columns.count() == 0);

    QAbstractItemView *view = d->createColumn(index, true);
    view->selectionModel()->deleteLater();
    view->setSelectionModel(selectionModel());

    QAbstractItemView::setRootIndex(index);
    d->updateScrollbars();
}

/*!
    \reimp
*/
bool QColumnView::isIndexHidden(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return false;
}

/*!
    \reimp
*/
QModelIndex QColumnView::indexAt(const QPoint &point) const
{
    Q_D(const QColumnView);
    for (int i = 0; i < d->columns.size(); ++i) {
        QPoint topLeft = d->columns.at(i)->frameGeometry().topLeft();
        QPoint adjustedPoint(point.x() - topLeft.x(), point.y() - topLeft.y());
        QModelIndex index = d->columns.at(i)->indexAt(adjustedPoint);
        if (index.isValid())
            return index;
    }
    return QModelIndex();
}

/*!
    \reimp
*/
QRect QColumnView::visualRect(const QModelIndex &index) const
{
    if (!index.isValid())
        return QRect();

    Q_D(const QColumnView);
    for (int i = 0; i < d->columns.size(); ++i) {
        QRect rect = d->columns.at(i)->visualRect(index);
        if (!rect.isNull()) {
            rect.translate(d->columns.at(i)->frameGeometry().topLeft());
            return rect;
        }
    }
    return QRect();
}

/*!
    \reimp
 */
void QColumnView::scrollContentsBy(int dx, int dy)
{
    Q_D(QColumnView);
    if (d->columns.isEmpty() || dx == 0)
        return;

    dx = isRightToLeft() ? -dx : dx;
    for (int i = 0; i < d->columns.count(); ++i)
        d->columns.at(i)->move(d->columns.at(i)->x() + dx, 0);
    d->offset += dx;
    QAbstractItemView::scrollContentsBy(dx, dy);
}

/*!
    \reimp
*/
void QColumnView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    Q_D(QColumnView);
    Q_UNUSED(hint);
    if (!model() || !index.isValid() || d->columns.isEmpty())
        return;

    if (d->currentAnimation.state() == QTimeLine::Running)
        return;

    d->currentAnimation.stop();
    QModelIndex indexParent = index.parent();
    // Find the left edge of the column that contains index
    int currentColumn = 0;
    int leftEdge = 0;
    while (currentColumn < d->columns.size()) {
        if (indexParent == d->columns.at(currentColumn)->rootIndex())
            break;
        leftEdge += d->columns.at(currentColumn)->width();
        ++currentColumn;
    }

    int indexColumn = currentColumn;
    // Find the width of what we want to show (i.e. the right edge)
    int visibleWidth = d->columns.at(currentColumn)->width();
    // We want to always try to show two columns
    if (currentColumn + 1 < d->columns.size()) {
        ++currentColumn;
        visibleWidth += d->columns.at(currentColumn)->width();
    }

    int rightEdge = leftEdge + visibleWidth;
    if (isRightToLeft()) {
        leftEdge = viewport()->width() - leftEdge;
        rightEdge = leftEdge - visibleWidth;
        qSwap(rightEdge, leftEdge);
    }

    // If it is already visible don't animate
    if (leftEdge > -horizontalOffset()
        && rightEdge
           <= ( -horizontalOffset() + viewport()->size().width())) {
            d->columns.at(indexColumn)->scrollTo(index);
            d->_q_changeCurrentColumn();
            return;
    }

    int newScrollbarValue = 0;
    if (isRightToLeft()) {
        if (leftEdge < 0) {
            // scroll to the right
            newScrollbarValue = viewport()->size().width() - leftEdge;
        } else {
            // scroll to the left
            newScrollbarValue = rightEdge + horizontalOffset();
        }
    } else {
        if (leftEdge > -horizontalOffset()) {
            // scroll to the right
            newScrollbarValue = rightEdge - viewport()->size().width();
        } else {
            // scroll to the left
            newScrollbarValue = leftEdge;
        }
    }

    //horizontalScrollBar()->setValue(newScrollbarValue);
    //d->_q_changeCurrentColumn();
    //return;
    // or do the following currentAnimation

    int oldValue = horizontalScrollBar()->value();

    if (oldValue < newScrollbarValue) {
        d->currentAnimation.setFrameRange(oldValue, newScrollbarValue);
        d->currentAnimation.setDirection(QTimeLine::Forward);
        d->currentAnimation.setCurrentTime(0);
    } else {
        d->currentAnimation.setFrameRange(newScrollbarValue, oldValue);
        d->currentAnimation.setDirection(QTimeLine::Backward);
    }
    d->currentAnimation.start();
}

/*!
    \reimp
    Move left should go to the parent index
    Move right should go to the child index or down if there is no child
*/
QModelIndex QColumnView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    // the child views which have focus get to deal with this first and if
    // they don't accept it then it comes up this this view and we only grip left/right
    Q_UNUSED(modifiers);
    if (!model())
        return QModelIndex();

    QModelIndex current = currentIndex();
    if (isRightToLeft()) {
        if (cursorAction == MoveLeft)
            cursorAction = MoveRight;
        else if (cursorAction == MoveRight)
            cursorAction = MoveLeft;
    }
    switch (cursorAction) {
    case MoveLeft:
        if (current.parent().isValid() && current.parent() != rootIndex())
            return (current.parent());
        else
            return current;
        break;

    case MoveRight:
        if (current.isValid() && model()->hasChildren(current))
            return model()->index(0, 0, current);
        else
            return current.sibling(current.row() + 1, current.column());
        break;

    default:
        break;
    }

    return QModelIndex();
}

/*!
    \reimp
*/
void QColumnView::resizeEvent( QResizeEvent *event )
{
    Q_D(QColumnView);
    d->doLayout();
    d->updateScrollbars();
    if (!isRightToLeft()) {
        int diff = event->oldSize().width() - event->size().width();
        if (diff < 0 && horizontalScrollBar()->isVisible()
            && horizontalScrollBar()->value() == horizontalScrollBar()->maximum()) {
            horizontalScrollBar()->setMaximum(horizontalScrollBar()->maximum() + diff);
        }
    }
    QAbstractItemView::resizeEvent(event);
}

/*!
    \internal
*/
void QColumnViewPrivate::updateScrollbars()
{
    Q_Q(QColumnView);
    if (currentAnimation.state() == QTimeLine::Running)
        return;

    // find the total horizontal length of the laid out columns
    int horizontalLength = 0;
    if (!columns.isEmpty()) {
        horizontalLength = (columns.last()->x() + columns.last()->width()) - columns.first()->x();
        if (horizontalLength <= 0) // reverse mode
            horizontalLength = (columns.first()->x() + columns.first()->width()) - columns.last()->x();
    }

    QSize viewportSize = q->viewport()->size();
    if (horizontalLength < viewportSize.width() && q->horizontalScrollBar()->value() == 0) {
        q->horizontalScrollBar()->setRange(0, 0);
    } else {
        int visibleLength = qMin(horizontalLength + q->horizontalOffset(), viewportSize.width());
        int hiddenLength = horizontalLength - visibleLength;
        if (hiddenLength != q->horizontalScrollBar()->maximum())
            q->horizontalScrollBar()->setRange(0, hiddenLength);
    }
    if (!columns.isEmpty()) {
        int pageStepSize = columns.at(0)->width();
        if (pageStepSize != q->horizontalScrollBar()->pageStep())
            q->horizontalScrollBar()->setPageStep(pageStepSize);
    }
    bool visible = (q->horizontalScrollBar()->maximum() > 0);
    if (visible != q->horizontalScrollBar()->isVisible())
        q->horizontalScrollBar()->setVisible(visible);
}

/*!
    \reimp
*/
int QColumnView::horizontalOffset() const
{
    Q_D(const QColumnView);
    return d->offset;
}

/*!
    \reimp
*/
int QColumnView::verticalOffset() const
{
    return 0;
}

/*!
    \reimp
*/
QRegion QColumnView::visualRegionForSelection(const QItemSelection &selection) const
{
    int ranges = selection.count();

    if (ranges == 0)
        return QRect();

    // Note that we use the top and bottom functions of the selection range
    // since the data is stored in rows.
    int firstRow = selection.at(0).top();
    int lastRow = selection.at(0).top();
    for (int i = 0; i < ranges; ++i) {
        firstRow = qMin(firstRow, selection.at(i).top());
        lastRow = qMax(lastRow, selection.at(i).bottom());
    }

    QModelIndex firstItem =
        model()->index(qMin(firstRow, lastRow), 0, rootIndex());
    QModelIndex lastItem =
        model()->index(qMax(firstRow, lastRow), 0, rootIndex());

    if (firstItem == lastItem)
        return visualRect(firstItem);

    QRegion firstRegion = visualRect(firstItem);
    QRegion lastRegion = visualRect(lastItem);
    return firstRegion.unite(lastRegion);
}

/*!
    \reimp
*/
void QColumnView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{
    Q_UNUSED(rect);
    Q_UNUSED(command);
}

/*!
    \reimp
*/
void QColumnView::setSelectionModel(QItemSelectionModel * newSelectionModel)
{
    Q_D(const QColumnView);
    for (int i = 0; i < d->columns.size(); ++i) {
        if (d->columns.at(i)->selectionModel() == selectionModel()) {
            if (d->columns.at(i)->selectionModel() != selectionModel())
                d->columns.at(i)->selectionModel()->deleteLater();
            d->columns.at(i)->setSelectionModel(newSelectionModel);
            break;
        }
    }
    QAbstractItemView::setSelectionModel(newSelectionModel);
}

/*!
    \reimp
*/
QSize QColumnView::sizeHint() const
{
    Q_D(const QColumnView);
    QSize sizeHint;
    for (int i = 0; i < d->columns.size(); ++i) {
        sizeHint += d->columns.at(i)->sizeHint();
    }
    return sizeHint.expandedTo(QSize(540,370));
}

/*!
    \internal
    Move all widgets from the corner grip and to the right
  */
void QColumnViewPrivate::_q_gripMoved(int offset)
{
    Q_Q(QColumnView);

    QObject *grip = q->sender();
    if (!grip)
        return;

    if (q->isRightToLeft())
        offset = -1 * offset;

    bool found = false;
    for (int i = 0; i < columns.size(); ++i) {
        if (!found && columns.at(i)->cornerWidget() == grip) {
            found = true;
            columnSizes[i] = columns.at(i)->width();
            if (q->isRightToLeft())
                columns.at(i)->move(columns.at(i)->x() + offset, 0);
            continue;
        }
        if (!found)
            continue;

        int currentX = columns.at(i)->x();
        columns.at(i)->move(currentX + offset, 0);
    }

    updateScrollbars();
}

/*!
    \internal

    Find where the current columns intersect parent's columns

    Delete any extra columns and insert any needed columns.
  */
void QColumnViewPrivate::closeColumns(const QModelIndex &parent, bool build)
{
    if (columns.isEmpty())
        return;

    QList<QModelIndex> dirsToAppend;

    // Find the last column that matches the parent's tree
    int currentColumn = -1;
    QModelIndex parentIndex = parent;
    while (currentColumn == -1 && parentIndex.isValid()) {
        if (columns.isEmpty())
            break;
        parentIndex = parentIndex.parent();
        if (!parentIndex.isValid())
            break;
        for (int i = columns.size() - 1; i >= 0; --i) {
            if (columns.at(i)->rootIndex() == parentIndex) {
                currentColumn = i;
                break;
            }
        }
        if (currentColumn == -1)
            dirsToAppend.append(parentIndex);
    }
    if (currentColumn == -1 && parent.isValid())
        currentColumn = 0;

    // Optimization so we don't go deleting and then creating the same thing
    bool alreadyExists = false;
    if (build && columns.size() > currentColumn + 1) {
        bool viewingParent = (columns.at(currentColumn + 1)->rootIndex() == parent);
        bool viewingChild = (!model->hasChildren(parent)
                             && !columns.at(currentColumn + 1)->rootIndex().isValid());
        if (viewingParent || viewingChild) {
            currentColumn++;
            alreadyExists = true;
        }
    }

    // Delete columns that don't match our path
    for (int i = columns.size() - 1; i > currentColumn; --i) {
        QAbstractItemView* notShownAnymore = columns.at(i);
        columns.removeAt(i);
        notShownAnymore->setVisible(false);
        if (notShownAnymore != previewColumn)
            notShownAnymore->deleteLater();
    }

    // Now fill in missing columns
    while (!dirsToAppend.isEmpty())
        createColumn(dirsToAppend.takeLast(), true);

    if (build && !alreadyExists)
        createColumn(parent, false);
}

void QColumnViewPrivate::_q_clicked(const QModelIndex &index)
{
    Q_Q(QColumnView);
    if (!index.isValid())
        return;
    if (q->currentIndex().parent() != index.parent())
        q->setCurrentIndex(index);
}

/*!
    \internal
    Create a new column for \a index.  A grip is attached if requested and it is shown
    if requested.

    Return the new view

    \sa createColumn() setPreviewWidget()
    \sa doLayout()
*/
QAbstractItemView *QColumnViewPrivate::createColumn(const QModelIndex &index, bool show)
{
    Q_Q(QColumnView);
    QAbstractItemView *view = 0;
    if (model->hasChildren(index)) {
        view = q->createColumn(index);
        q->connect(view, SIGNAL(clicked(const QModelIndex &)),
                   q, SLOT(_q_clicked(const QModelIndex &)));
    }
    else {
        view = previewColumn;
        view->setMinimumWidth(qMax(view->minimumWidth(), previewWidget->minimumWidth()));
    }
    view->setFocusPolicy(Qt::NoFocus);
    view->setParent(q->viewport());
    Q_ASSERT(view);

    // Setup corner grip
    if (showResizeGrips) {
        QColumnViewGrip *grip = new QColumnViewGrip(view);
        view->setCornerWidget(grip);
        q->connect(grip, SIGNAL(gripMoved(int)), q, SLOT(_q_gripMoved(int)));
    }

    if (columnSizes.count() > columns.count()) {
        view->setGeometry(0, 0, columnSizes.at(columns.count()), q->viewport()->height());
    } else {
        int initialWidth = view->sizeHint().width();
        if (q->isRightToLeft())
            view->setGeometry(q->viewport()->width() - initialWidth, 0, initialWidth, q->viewport()->height());
        else
            view->setGeometry(0, 0, initialWidth, q->viewport()->height());
        columnSizes.resize(qMax(columnSizes.count(), columns.count() + 1));
        columnSizes[columns.count()] = initialWidth;
    }
    if (!columns.isEmpty() && columns.last()->isHidden())
        columns.last()->setVisible(true);

    columns.append(view);
    doLayout();
    updateScrollbars();
    if (show && view->isHidden())
        view->setVisible(true);
    return view;
}

/*!
    \fn void QColumnView::updatePreviewWidget(const QModelIndex &index)

    This signal is emitted when the preview widget should be updated to
    provide rich information about \a index

    \sa previewWidget()
 */

/*!
    To use a custom widget for the final column when you select
    an item overload this function and return a widget.
    \a index is the root index that will be assigned to the view.

    Return the new view.  QColumnView will automatically take ownership of the widget.

    \sa setPreviewWidget()
 */
QAbstractItemView *QColumnView::createColumn(const QModelIndex &index)
{
    Q_D(QColumnView);

    QListView *view = new QListView(viewport());
    view->setFrameShape(QFrame::NoFrame);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setMinimumWidth(100);

    // Copy the 'view' behavior
#ifndef QT_NO_DRAGANDDROP
    view->setDragDropMode(dragDropMode());
    view->setDragDropOverwriteMode(dragDropOverwriteMode());
    view->setDropIndicatorShown(showDropIndicator());
#endif
    view->setAlternatingRowColors(alternatingRowColors());
    view->setAutoScroll(hasAutoScroll());
    view->setEditTriggers(editTriggers());
    view->setHorizontalScrollMode(horizontalScrollMode());
    view->setIconSize(iconSize());
    view->setSelectionBehavior(selectionBehavior());
    view->setSelectionMode(selectionMode());
    view->setTabKeyNavigation(tabKeyNavigation());
    view->setTextElideMode(textElideMode());
    view->setVerticalScrollMode(verticalScrollMode());

    view->setModel(model());

    // Copy the custom delegate per row
    QMapIterator<int, QPointer<QAbstractItemDelegate> > i(d->rowDelegates);
    while (i.hasNext()) {
        i.next();
        view->setItemDelegateForRow(i.key(), i.value());
    }

    // set the delegate to be the columnview delegate
    QAbstractItemDelegate *delegate = view->itemDelegate();
    view->setItemDelegate(d->itemDelegate);
    delete delegate;

    view->setRootIndex(index);

    if (model()->canFetchMore(index))
        model()->fetchMore(index);

    return view;
}

/*!
    Returns the preview widget, or 0 if there is none.

    \sa setPreviewWidget(), updatePreviewWidget()
*/
QWidget *QColumnView::previewWidget() const
{
    Q_D(const QColumnView);
    return d->previewWidget;
}

/*!
    Sets the preview \a widget.

    The \a widget becomes a child of the column view, and will be
    destroyed when the column area is deleted or when a new widget is
    set.

    \sa previewWidget(), updatePreviewWidget()
*/
void QColumnView::setPreviewWidget(QWidget *widget) {
    Q_D(QColumnView);
    d->setPreviewWidget(widget);
}

/*!
    \internal
*/
void QColumnViewPrivate::setPreviewWidget(QWidget *widget)
{
    if (previewColumn)
        previewColumn->deleteLater();
    previewColumn = new QColumnViewPreviewColumn(widget);
    previewColumn->setFrameShape(QFrame::NoFrame);
    previewColumn->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    previewColumn->setSelectionMode(QAbstractItemView::NoSelection);
    previewColumn->setMinimumWidth(qMax(previewColumn->verticalScrollBar()->width(),
                                   previewColumn->minimumWidth()));
    previewWidget = widget;
    previewWidget->setParent(previewColumn->viewport());
}

/*!
    \reimp
*/
void QColumnView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_D(QColumnView);
    if (!model())
        return;

    if (!current.isValid()) {
        QAbstractItemView::currentChanged(current, previous);
        return;
    }

    QModelIndex currentParent = current.parent();
    // optimize for just moving up/down in a list where the child view doesn't change
    if (currentParent == previous.parent()
            && model()->hasChildren(current) && model()->hasChildren(previous)) {
        for (int i = 0; i < d->columns.size(); ++i) {
            if (currentParent == d->columns.at(i)->rootIndex()) {
                if (d->columns.size() > i + 1) {
                    QAbstractItemView::currentChanged(current, previous);
                    return;
                }
                break;
            }
        }
    }

    // Scrolling to the right we need to have an empty spot
    bool found = false;
    if (currentParent == previous) {
        for (int i = 0; i < d->columns.size(); ++i) {
            if (currentParent == d->columns.at(i)->rootIndex()) {
                found = true;
                if (d->columns.size() < i + 2) {
                    d->createColumn(current, false);
                }
                break;
            }
        }
    }
    if (!found)
        d->closeColumns(current, true);

    if (!model()->hasChildren(current))
        emit updatePreviewWidget(current);

    QAbstractItemView::currentChanged(current, previous);
}

/*
    We have change the current column and need to update focus and selection models
    on the new current column.
*/
void QColumnViewPrivate::_q_changeCurrentColumn()
{
    Q_Q(QColumnView);
    if (columns.isEmpty())
        return;

    QModelIndex current = q->currentIndex();
    if (!current.isValid())
        return;

    // We might have scrolled far to the left so we need to close all of the children
    closeColumns(current, true);

    // Set up the "current" column with focus
    int currentColumn = qMax(0, columns.size() - 2);
    QAbstractItemView *parentColumn = columns.at(currentColumn);
    parentColumn->setCurrentIndex(current);
    parentColumn->setFocus(Qt::OtherFocusReason);
    q->setFocusProxy(parentColumn);

    // find the column that is our current selection model and give it a new one.
    for (int i = 0; i < columns.size(); ++i) {
        if (columns.at(i)->selectionModel() == q->selectionModel()) {
            QItemSelectionModel *replacementSelectionModel =
                new QItemSelectionModel(parentColumn->model());
            replacementSelectionModel->setCurrentIndex(
                q->selectionModel()->currentIndex(), QItemSelectionModel::Current);
            replacementSelectionModel->select(
                q->selectionModel()->selection(), QItemSelectionModel::Select);
            QAbstractItemView *view = columns.at(i);
            view->setSelectionModel(replacementSelectionModel);
            q->disconnect(view, SIGNAL(activated(const QModelIndex &)),
                    q, SIGNAL(activated(const QModelIndex &)));
            q->disconnect(view, SIGNAL(clicked(const QModelIndex &)),
                    q, SIGNAL(clicked(const QModelIndex &)));
            q->disconnect(view, SIGNAL(doubleClicked(const QModelIndex &)),
                    q, SIGNAL(doubleClicked(const QModelIndex &)));
            q->disconnect(view, SIGNAL(entered(const QModelIndex &)),
                    q, SIGNAL(entered(const QModelIndex &)));
            q->disconnect(view, SIGNAL(pressed(const QModelIndex &)),
                    q, SIGNAL(pressed(const QModelIndex &)));
            view->setFocusPolicy(Qt::NoFocus);
            break;
        }
    }
    parentColumn->selectionModel()->deleteLater();
    parentColumn->setFocusPolicy(Qt::StrongFocus);
    parentColumn->setSelectionModel(q->selectionModel());
    q->connect(parentColumn, SIGNAL(activated(const QModelIndex &)),
            q, SIGNAL(activated(const QModelIndex &)));
    q->connect(parentColumn, SIGNAL(clicked(const QModelIndex &)),
            q, SIGNAL(clicked(const QModelIndex &)));
    q->connect(parentColumn, SIGNAL(doubleClicked(const QModelIndex &)),
            q, SIGNAL(doubleClicked(const QModelIndex &)));
    q->connect(parentColumn, SIGNAL(entered(const QModelIndex &)),
            q, SIGNAL(entered(const QModelIndex &)));
    q->connect(parentColumn, SIGNAL(pressed(const QModelIndex &)),
            q, SIGNAL(pressed(const QModelIndex &)));

    // We want the parent selection to stay highlighted (but dimmed depending upon the color theme)
    if (currentColumn > 0) {
        parentColumn = columns.at(currentColumn - 1);
        parentColumn->setCurrentIndex(current.parent());
    }

    if (columns.last()->isHidden())
        columns.last()->setVisible(true);

    updateScrollbars();
}

/*!
    \reimp
*/
void QColumnView::selectAll()
{
    if (!model() || !selectionModel())
        return;

    QModelIndexList indexList = selectionModel()->selectedIndexes();
    QModelIndex parent = rootIndex();
    QItemSelection selection;
    if (indexList.count() >= 1)
        parent = indexList.at(0).parent();
    if (indexList.count() == 1) {
        parent = indexList.at(0);
        if (!model()->hasChildren(parent))
            parent = parent.parent();
        else
            selection.append(QItemSelectionRange(parent, parent));
    }

    QModelIndex tl = model()->index(0, 0, parent);
    QModelIndex br = model()->index(model()->rowCount(rootIndex()) - 1,
                                    model()->columnCount(rootIndex()) - 1,
                                    rootIndex());
    selection.append(QItemSelectionRange(tl, br));
    selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
}

/*
 * private object implementation
 */
QColumnViewPrivate::QColumnViewPrivate()
:  QAbstractItemViewPrivate()
,showResizeGrips(true)
,offset(0)
,currentAnimation(ANIMATION_DURATION_MSEC)
,previewColumn(0)
{
    setPreviewWidget(new QWidget());
}

QColumnViewPrivate::~QColumnViewPrivate()
{
}

/*!
    \internal
    Place all of the columns where they belong inside of the viewport, resize as necessary.
*/
void QColumnViewPrivate::doLayout()
{
    Q_Q(QColumnView);
    if (!model || columns.isEmpty())
        return;

    int viewportHeight = q->viewport()->height();
    int x = columns.at(0)->x();

    if (q->isRightToLeft()) {
        x = q->viewport()->width() + q->horizontalOffset();
        for (int i = 0; i < columns.size(); ++i) {
            QAbstractItemView *view = columns.at(i);
            x -= view->width();
            if (x != view->x() || viewportHeight != view->height())
                view->setGeometry(x, 0, view->width(), viewportHeight);
        }
    } else {
        for (int i = 0; i < columns.size(); ++i) {
            QAbstractItemView *view = columns.at(i);
            int currentColumnWidth = view->width();
            if (x != view->x() || viewportHeight != view->height())
                view->setGeometry(x, 0, currentColumnWidth, viewportHeight);
            x += currentColumnWidth;
        }
    }
}

/*!
    \internal

    Draws a delegate with a > if an object has children.

    \sa {Model/View Programming}, QItemDelegate
*/
void QColumnViewDelegate::paint(QPainter *painter,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
    drawBackground(painter, option, index );

    bool reverse = (option.direction == Qt::RightToLeft);
    int width = ((option.rect.height() * 2) / 3);
    // Modify the options to give us room to add an arrow
    QStyleOptionViewItem opt = option;
    if (reverse)
        opt.rect.adjust(width,0,0,0);
    else
        opt.rect.adjust(0,0,-width,0);

    if (!(index.model()->flags(index) & Qt::ItemIsEnabled)) {
        opt.showDecorationSelected = true;
        opt.state |= QStyle::State_Selected;
    }

    QItemDelegate::paint(painter, opt, index);

    // Draw >
    if (index.model()->hasChildren(index)) {
        qApp->style()->drawPrimitive(QStyle::PE_IndicatorColumnViewArrow, &opt, painter);
    }
}

#endif // QT_NO_QCOLUMNVIEW

#include "moc_qcolumnview.cpp"