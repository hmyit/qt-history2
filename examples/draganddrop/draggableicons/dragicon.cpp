#include <QtGui>

#include "dragicon.h"

DragIcon::DragIcon(QWidget *parent)
    : QLabel(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
}

void DragIcon::mousePressEvent(QMouseEvent *event)
{
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << *pixmap() << QPoint(event->pos() - rect().topLeft());

    QMimeData *mimeData = new QMimeData;
    mimeData->setData("application/x-dnditemdata", itemData);
        
    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(*pixmap());
    drag->setHotSpot(event->pos() - rect().topLeft());
    
    hide();

    if (drag->start(Qt::CopyAction | Qt::MoveAction) == Qt::MoveAction)
        close();
    else
        show();
}
