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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef QDESIGNER_MENUBAR_H
#define QDESIGNER_MENUBAR_H

#include "shared_global_p.h"

#include <QtCore/QPointer>
#include <QtGui/QAction>
#include <QtGui/QMenuBar>
#include <QtCore/QMimeData>

class QTimer;
class QToolButton;
class QDesignerFormWindowInterface;
class QDesignerActionProviderExtension;
class QDesignerMenuBar;

namespace qdesigner_internal {

class MenuToolBox;

class MenuMimeData: public QMimeData
{
    Q_OBJECT
public:
    MenuMimeData() {}
    virtual ~MenuMimeData() {}

    virtual bool hasFormat(const QString &mimeType) const
    { return mimeType == QLatin1String("action-repository/menu"); }
};

class MenuToolBox: public QWidget
{
    Q_OBJECT
public:
    MenuToolBox(QDesignerMenuBar *menuBar);
    virtual ~MenuToolBox();

    QDesignerMenuBar *menuBar() const;

private slots:
    void slotCreateMenu();

private:
    QToolButton *m_createMenuButton;
};

} // namespace qdesigner_internal

class QDESIGNER_SHARED_EXPORT QDesignerMenuBar: public QMenuBar
{
    Q_OBJECT
public:
    QDesignerMenuBar(QWidget *parent = 0);
    virtual ~QDesignerMenuBar();

    bool eventFilter(QObject *object, QEvent *event);

    QDesignerFormWindowInterface *formWindow() const;
    QDesignerActionProviderExtension *actionProvider();

private slots:
    void slotRemoveSelectedAction(QAction *action);
    void slotCheckSentinel();

protected:
    void startDrag(const QPoint &pos);
    virtual void actionEvent(QActionEvent *event);
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dragMoveEvent(QDragMoveEvent *event);
    virtual void dragLeaveEvent(QDragLeaveEvent *event);
    virtual void dropEvent(QDropEvent *event);

    bool handleEvent(QWidget *widget, QEvent *event);
    bool handleMousePressEvent(QWidget *widget, QMouseEvent *event);
    bool handleMouseReleaseEvent(QWidget *widget, QMouseEvent *event);
    bool handleMouseMoveEvent(QWidget *widget, QMouseEvent *event);
    bool handleContextMenuEvent(QWidget *widget, QContextMenuEvent *event);

    void adjustIndicator(const QPoint &pos);
    int findAction(const QPoint &pos) const;

    bool blockSentinelChecker(bool b);

private:
    QTimer *m_sentinelChecker;
    QAction *m_sentinel;
    bool m_blockSentinelChecker;
    QPointer<QMenu> m_activeMenu;
    QPoint m_startPosition;
    qdesigner_internal::MenuToolBox *m_toolBox;
};

#endif // QDESIGNER_MENUBAR_H
