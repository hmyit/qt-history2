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

#ifndef QMDIAREA_CONTAINER_H
#define QMDIAREA_CONTAINER_H

#include <QtDesigner/QDesignerContainerExtension>
#include <QtDesigner/QExtensionFactory>

class QMdiArea;

namespace qdesigner_internal {

class QMdiAreaContainer: public QObject, public QDesignerContainerExtension
{
    Q_OBJECT
    Q_INTERFACES(QDesignerContainerExtension)
public:
    QMdiAreaContainer(QMdiArea *widget, QObject *parent = 0);

    virtual int count() const;
    virtual QWidget *widget(int index) const;
    virtual int currentIndex() const;
    virtual void setCurrentIndex(int index);
    virtual void addWidget(QWidget *widget);
    virtual void insertWidget(int index, QWidget *widget);
    virtual void remove(int index);

private:
    QMdiArea *m_mdiArea;
};

class QMdiAreaContainerFactory: public QExtensionFactory
{
    Q_OBJECT
public:
    QMdiAreaContainerFactory(QExtensionManager *parent = 0);

protected:
    virtual QObject *createExtension(QObject *object, const QString &iid, QObject *parent) const;
};

}  // namespace qdesigner_internal

#endif // QMDIAREA_CONTAINER_H