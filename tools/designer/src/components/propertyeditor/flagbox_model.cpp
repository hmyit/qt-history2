
#include "flagbox_model_p.h"

FlagBoxModel::FlagBoxModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

FlagBoxModel::~FlagBoxModel()
{
}

void FlagBoxModel::setItems(const QList<FlagBoxModelItem> &items)
{
    m_items = items;
    emit reset();
}

int FlagBoxModel::rowCount(const QModelIndex &parent) const
{
    return !parent.isValid() ? m_items.count() : 0;
}

int FlagBoxModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QModelIndex FlagBoxModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

QModelIndex FlagBoxModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return createIndex(row, column, 0);
}

QVariant FlagBoxModel::data(const QModelIndex &index, int role) const
{
    const FlagBoxModelItem &item = m_items.at(index.row());

    switch (role) {
    case DisplayRole:
    case EditRole:
        return item.name();

    case DecorationRole:
        return item.isChecked();

    default:
        return QVariant();
    } // end switch
}

bool FlagBoxModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    FlagBoxModelItem &item = m_items[index.row()];

    switch (role) {
    case EditRole:
    case DisplayRole:
        item.setName(value.toString());
        return true;

    case DecorationRole:
        item.setChecked(value.toBool());
        emit dataChanged(index, index);
        return true;

    default:
        return false;
    } // end switch
}

