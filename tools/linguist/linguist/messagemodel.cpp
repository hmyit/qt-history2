/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "messagemodel.h"
#include "contextmodel.h"
#include "trwindow.h"

MessageItem::MessageItem(const MetaTranslatorMessage &message, 
                         const QString &text, const QString &comment, ContextItem *ctxtI) 
                         : QObject(ctxtI), m(message), tx(text), com(comment), cntxtItem(ctxtI)
{
    if (m.translation().isEmpty()) {
        QString t = "";
        m.setTranslation(t);
    }

    fini = true;
    d = false;

    if (m.type() == MetaTranslatorMessage::Unfinished)
         setFinished(false);
}

void MessageItem::setFinished(bool finished)
{
    if (!fini && finished) {
        m.setType(MetaTranslatorMessage::Finished);
        cntxtItem->decrementUnfinishedCount();
    } else if (fini && !finished) {
        m.setType(MetaTranslatorMessage::Unfinished);
        cntxtItem->incrementUnfinishedCount();
    }
    fini = finished;
}

void MessageItem::setDanger(bool danger)
{
    if (!d && danger) {
        cntxtItem->incrementDangerCount();
    } else if (d && !danger) {
        cntxtItem->decrementDangerCount();
    }
    d = danger;
}

MessageModel::MessageModel(QObject *parent)
: QAbstractTableModel(parent), cntxtItem(0)
{
    
}

void MessageModel::setContextItem(ContextItem *ctxtI)
{
    if (ctxtI == cntxtItem)
        return;

    if (cntxtItem != 0) {
        int r = cntxtItem->messageItemsInList();
        emit rowsRemoved(QModelIndex::Null, 0, r-1);
        cntxtItem = 0;
    }

    cntxtItem = ctxtI;

    if (cntxtItem != 0)
        emit rowsInserted(QModelIndex::Null, 0, cntxtItem->messageItemsInList()-1);
}

void MessageModel::updateItem(QModelIndex indx)
{
    QModelIndex strtindx = createIndex(indx.row(), 0);
    QModelIndex endindx = createIndex(indx.row(), 2);

    emit dataChanged(strtindx, endindx);
}

int MessageModel::rowCount() const
{
    if (cntxtItem != 0)
        return cntxtItem->messageItemsInList();

    return 0;
}

int MessageModel::columnCount() const
{
    return 3;
}

QVariant MessageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ((role == QAbstractItemModel::DisplayRole) && (orientation == Qt::Horizontal)) {
        switch(section)
        {
        case 0:
            return tr("Done");
        case 1:
            return tr("Source text");
        case 2:
            return tr("Translation");
        }

        return "Error";
    }
    else {
        return QString::null;
    }
}

QVariant MessageModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    int column = index.column();

    if (cntxtItem == 0)
        return QString::null;

    if (row >= cntxtItem->messageItemsInList() || !index.isValid())
        return QString::null;

    MessageItem *msgItem = cntxtItem->messageItem(row);

    if (role == QAbstractItemModel::DisplayRole) {
        switch(column) {
        case 0: // done
            return QString::null;
        case 1: // source text
            return msgItem->sourceText();
        case 2: // translation
            return msgItem->translation();
        }
    }
    else if ((role == QAbstractItemModel::DecorationRole) && (column == 0)) {
        if (msgItem->message().type() == MetaTranslatorMessage::Unfinished && msgItem->translation().isEmpty())
            return QVariant(*TrWindow::pxEmpty);
        else if (msgItem->message().type() == MetaTranslatorMessage::Unfinished && msgItem->danger())
            return QVariant(*TrWindow::pxDanger);
        else if (msgItem->message().type() == MetaTranslatorMessage::Finished && msgItem->danger())
            return QVariant(*TrWindow::pxObs);
        else if (msgItem->message().type() == MetaTranslatorMessage::Finished)
            return QVariant(*TrWindow::pxOn);
        else if (msgItem->message().type() == MetaTranslatorMessage::Unfinished)
            return QVariant(*TrWindow::pxOff);
        else if (msgItem->message().type() == MetaTranslatorMessage::Obsolete)
            return QVariant(*TrWindow::pxObsolete);
    }

    return QString::null;
}

void MessageModel::sort(int column, const QModelIndex &parent, Qt::SortOrder order)
{
    if (cntxtItem != 0) {
        cntxtItem->sortMessages(column, order);
        emit dataChanged(index(0,0), 
            index(cntxtItem->messageItemsInList()-1, 2));
    }

    Q_UNUSED(parent);
}
