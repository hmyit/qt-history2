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

#ifndef QDESIGNER_H
#define QDESIGNER_H

#include <QtCore/QPointer>
#include <QtGui/QApplication>

#define qDesigner \
    (static_cast<QDesigner*>(QCoreApplication::instance()))

class QDesignerSettings;
class QDesignerSession;
class QDesignerMainWindow;
class QDesignerServer;

class QDesigner: public QApplication
{
    Q_OBJECT
public:
    QDesigner(int &argc, char **argv);
    virtual ~QDesigner();

    QDesignerMainWindow *mainWindow() const;
    QDesignerSession *session() const;
    QDesignerServer *server() const;

protected:
    bool event(QEvent *ev);

signals:
    void initialized();

private slots:
    void initialize();

private:
    QDesignerServer *m_server;
    QDesignerSession *m_session;
    QDesignerMainWindow *m_mainWindow;
};

#endif // QDESIGNER_H
