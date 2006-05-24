/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef Q3DOCKWINDOW_H
#define Q3DOCKWINDOW_H

#include <Qt3Support/q3frame.h>

QT_BEGIN_HEADER

QT_MODULE(Qt3SupportLight)

#ifndef QT_NO_MAINWINDOW

class Q3DockWindowHandle;
class Q3DockWindowTitleBar;
class QPainter;
class Q3DockWindowResizeHandle;
class QBoxLayout;
class QHBoxLayout;
class QVBoxLayout;
class Q3DockArea;
class QWidgetResizeHandler;
class Q3MainWindow;
class Q3DockAreaLayout;
class Q3DockWindowPrivate;
class Q3ToolBar;
class QWindowsXPStyle;
class QRubberBand;

class Q_COMPAT_EXPORT Q3DockWindow : public Q3Frame
{
    Q_OBJECT
    Q_ENUMS(CloseMode Place)
    Q_PROPERTY(int closeMode READ closeMode  WRITE setCloseMode)
    Q_PROPERTY(bool resizeEnabled READ isResizeEnabled  WRITE setResizeEnabled)
    Q_PROPERTY(bool movingEnabled READ isMovingEnabled  WRITE setMovingEnabled)
    Q_PROPERTY(bool horizontallyStretchable READ isHorizontallyStretchable  WRITE setHorizontallyStretchable)
    Q_PROPERTY(bool verticallyStretchable READ isVerticallyStretchable  WRITE setVerticallyStretchable)
    Q_PROPERTY(bool stretchable READ isStretchable)
    Q_PROPERTY(bool newLine READ newLine  WRITE setNewLine)
    Q_PROPERTY(bool opaqueMoving READ opaqueMoving  WRITE setOpaqueMoving)
    Q_PROPERTY(int offset READ offset  WRITE setOffset)
    Q_PROPERTY(Place place READ place)

    friend class Q3DockWindowHandle;
    friend class Q3DockWindowTitleBar;
    friend class Q3DockArea;
    friend class Q3DockAreaLayout;
    friend class Q3MainWindow;
    friend class QCEMainWindow;
    friend class Q3ToolBar;
    friend class QWindowsXPStyle;

public:
    enum Place { InDock, OutsideDock };
    enum CloseMode { Never = 0, Docked = 1, Undocked = 2, Always = Docked | Undocked };

    Q3DockWindow(Place p = InDock, QWidget* parent=0, const char* name=0, Qt::WFlags f = 0);
    Q3DockWindow(QWidget* parent, const char* name=0, Qt::WFlags f = 0);
    ~Q3DockWindow();

    virtual void setWidget(QWidget *w);
    QWidget *widget() const;

    Place place() const { return curPlace; }

    Q3DockArea *area() const;

    virtual void setCloseMode(int m);
    bool isCloseEnabled() const;
    int closeMode() const;

    virtual void setResizeEnabled(bool b);
    virtual void setMovingEnabled(bool b);
    bool isResizeEnabled() const;
    bool isMovingEnabled() const;

    virtual void setHorizontallyStretchable(bool b);
    virtual void setVerticallyStretchable(bool b);
    bool isHorizontallyStretchable() const;
    bool isVerticallyStretchable() const;
    void setHorizontalStretchable(bool b) { setHorizontallyStretchable(b); }
    void setVerticalStretchable(bool b) { setVerticallyStretchable(b); }
    bool isHorizontalStretchable() const { return isHorizontallyStretchable(); }
    bool isVerticalStretchable() const { return isVerticallyStretchable(); }
    bool isStretchable() const;

    virtual void setOffset(int o);
    int offset() const;

    virtual void setFixedExtentWidth(int w);
    virtual void setFixedExtentHeight(int h);
    QSize fixedExtent() const;

    virtual void setNewLine(bool b);
    bool newLine() const;

    Qt::Orientation orientation() const;

    QSize sizeHint() const;
    QSize minimumSize() const;
    QSize minimumSizeHint() const;

    QBoxLayout *boxLayout();

    virtual void setOpaqueMoving(bool b);
    bool opaqueMoving() const;

    bool eventFilter(QObject *o, QEvent *e);

    QString windowTitle() const;

Q_SIGNALS:
    void orientationChanged(Qt::Orientation o);
    void placeChanged(Q3DockWindow::Place p);
    void visibilityChanged(bool);

public Q_SLOTS:
    virtual void undock(QWidget *w);
    virtual void undock() { undock(0); }
    virtual void dock();
    virtual void setOrientation(Qt::Orientation o);

protected:
    void resizeEvent(QResizeEvent *e);
    void showEvent(QShowEvent *e);
    void hideEvent(QHideEvent *e);
    void contextMenuEvent(QContextMenuEvent *e);

    void drawFrame(QPainter *);
    void drawContents(QPainter *);

    bool event(QEvent *e);

private Q_SLOTS:
    void toggleVisible() { setVisible(!isVisible()); }

private:
    Q3DockWindow(Place p, QWidget* parent, const char* name, Qt::WFlags f, bool toolbar);

    void handleMove(const QPoint &pos, const QPoint &gp, bool drawRect);
    void updateGui();
    void updateSplitterVisibility(bool visible);

    void startRectDraw(const QPoint &so, bool drawRect);
    void endRectDraw(bool drawRect);
    void updatePosition(const QPoint &globalPos );
    QWidget *areaAt(const QPoint &gp);
    void removeFromDock(bool fixNewLines = true);
    void swapRect(QRect &r, Qt::Orientation o, const QPoint &offset, Q3DockArea *area);
    void init();

private:
    Q3DockWindowHandle *horHandle, *verHandle;
    Q3DockWindowTitleBar *titleBar;
    QWidget *wid;
    QRubberBand *rubberBand;
    Q3DockArea *dockArea, *tmpDockArea;
    QRect currRect;
    Place curPlace;
    Place state;
    bool resizeEnabled : 1;
    bool moveEnabled : 1;
    bool nl : 1;
    bool opaque : 1;
    bool isToolbar : 1;
    bool stretchable[3];
    Qt::Orientation startOrientation;
    int cMode;
    QPoint startOffset;
    int offs;
    QSize fExtent;
    Q3DockWindowResizeHandle *hHandleTop, *hHandleBottom, *vHandleLeft, *vHandleRight;
    QVBoxLayout *hbox;
    QHBoxLayout *vbox;
    QBoxLayout *childBox;
    void *dockWindowData;
    QPoint lastPos;
    QSize lastSize;
    QWidgetResizeHandler *widgetResizeHandler;
    Q3DockWindowPrivate *d;

private:
    Q_DISABLE_COPY(Q3DockWindow)
};

inline Q3DockArea *Q3DockWindow::area() const
{
    return dockArea;
}

#endif // QT_NO_MAINWINDOW

QT_END_HEADER

#endif // Q3DOCKWINDOW_H
