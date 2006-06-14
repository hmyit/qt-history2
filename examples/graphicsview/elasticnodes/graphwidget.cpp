/****************************************************************************
**
** Copyright (C) 2006-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "graphwidget.h"
#include "edge.h"
#include "node.h"

#include <QDebug>
#include <QGraphicsScene>
#include <QWheelEvent>

GraphWidget::GraphWidget()
    : timerId(0)
{
    QGraphicsScene *scene = new QGraphicsScene(this);
    scene->setSceneRect(-200, -200, 400, 400);
    setScene(scene);
    setCacheMode(CacheBackground);

    Node *node1 = new Node(this);
    Node *node2 = new Node(this);
    Node *node3 = new Node(this);
    Node *node4 = new Node(this);
    centerNode = new Node(this);
    Node *node6 = new Node(this);
    Node *node7 = new Node(this);
    Node *node8 = new Node(this);
    Node *node9 = new Node(this);
    scene->addItem(node1);
    scene->addItem(node2);
    scene->addItem(node3);
    scene->addItem(node4);
    scene->addItem(centerNode);
    scene->addItem(node6);
    scene->addItem(node7);
    scene->addItem(node8);
    scene->addItem(node9);
    scene->addItem(new Edge(node1, node2));
    scene->addItem(new Edge(node2, node3));
    scene->addItem(new Edge(node2, centerNode));
    scene->addItem(new Edge(node3, node6));
    scene->addItem(new Edge(node4, node1));
    scene->addItem(new Edge(node4, centerNode));
    scene->addItem(new Edge(centerNode, node6));
    scene->addItem(new Edge(centerNode, node8));
    scene->addItem(new Edge(node6, node9));
    scene->addItem(new Edge(node7, node4));
    scene->addItem(new Edge(node8, node7));
    scene->addItem(new Edge(node9, node8));

    node1->setPos(-50, -50);
    node2->setPos(0, -50);
    node3->setPos(50, -50);
    node4->setPos(-50, 0);
    centerNode->setPos(0, 0);
    node6->setPos(50, 0);
    node7->setPos(-50, 50);
    node8->setPos(0, 50);
    node9->setPos(50, 50);
    
    setMinimumSize(400, 400);
    scale(0.8, 0.8);

    itemMoved();

    setWindowTitle(tr("Elastic Nodes"));
}

void GraphWidget::itemMoved()
{
    if (!timerId)
        timerId = startTimer(1000 / 25);
}

void GraphWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Up:
       centerNode->moveBy(0, -20);
       break;
    case Qt::Key_Down:
       centerNode->moveBy(0, 20);
       break;
    case Qt::Key_Left:
       centerNode->moveBy(-20, 0);
       break;
    case Qt::Key_Right:
       centerNode->moveBy(20, 0);
       break;
    case Qt::Key_Plus:
       scale(1.2, 1.2);
       break;
    case Qt::Key_Minus:
       scale(1 / 1.2, 1 / 1.2);
       break;
    case Qt::Key_Space:
    case Qt::Key_Enter:
       foreach (QGraphicsItem *item, scene()->items()) {
           if (qgraphicsitem_cast<Node *>(item))
               item->setPos(-150 + rand() % 300, -150 + rand() % 300);
       }
       break;
    default:
       QGraphicsView::keyPressEvent(event);
    }
}

void GraphWidget::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

    QList<Node *> nodes;
    foreach (QGraphicsItem *item, scene()->items()) {
        if (Node *node = qgraphicsitem_cast<Node *>(item))
            nodes << node;
    }

    foreach (Node *node, nodes)
        node->calculateForces();

    bool itemsMoved = false;
    foreach (Node *node, nodes) {
        if (node->advance())
            itemsMoved = true;
    }

    if (!itemsMoved) {
        killTimer(timerId);
        timerId = 0;
    }
}

void GraphWidget::wheelEvent(QWheelEvent *event)
{
    scale(1 + event->delta() / 1200.0, 1 + event->delta() / 1200.0);
}

void GraphWidget::drawBackground(QPainter *painter, const QRectF &rect)
{
    Q_UNUSED(rect);

    // Shadow
    QRectF sceneRect = this->sceneRect();
    QRectF rightShadow(sceneRect.right(), sceneRect.top() + 5, 5, sceneRect.height());
    QRectF bottomShadow(sceneRect.left() + 5, sceneRect.bottom(), sceneRect.width(), 5);
    if (rightShadow.intersects(rect) || rightShadow.contains(rect))
	painter->fillRect(rightShadow, Qt::darkGray);
    if (bottomShadow.intersects(rect) || bottomShadow.contains(rect))
	painter->fillRect(bottomShadow, Qt::darkGray);

    // Fill
    QLinearGradient gradient(sceneRect.topLeft(), sceneRect.bottomRight());
    gradient.setColorAt(0, Qt::white);
    gradient.setColorAt(1, Qt::lightGray);
    painter->fillRect(rect.intersect(sceneRect), gradient);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(sceneRect);

    // Text
    QRectF textRect(sceneRect.left(), sceneRect.top(), sceneRect.width(), 70);
    if (textRect.intersects(rect) || textRect.contains(rect)) {
	QFont font = painter->font();
	font.setPointSize(14);
	font.setBold(true);
	painter->setFont(font);
	painter->setPen(Qt::lightGray);
	QString message(tr("Click and drag the nodes around\nor zoom with the mouse wheel"));
	painter->drawText(sceneRect.translated(2, 2), Qt::AlignTop | Qt::AlignHCenter,
			  message);
	painter->setPen(Qt::black);
	painter->drawText(sceneRect, Qt::AlignTop | Qt::AlignHCenter,
			  message);
    }
}
