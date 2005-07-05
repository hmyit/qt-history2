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

#ifndef QAPPLICATION_H
#define QAPPLICATION_H

#include "QtCore/qcoreapplication.h"
#include "QtGui/qwindowdefs.h"
#include "QtCore/qpoint.h"
#include "QtCore/qsize.h"
#include "QtGui/qcursor.h"
#ifdef QT_INCLUDE_COMPAT
# include "QtGui/qdesktopwidget.h"
#endif
#ifdef QT3_SUPPORT
# include "QtGui/qwidget.h"
# include "QtGui/qpalette.h"
#endif
#ifdef Q_WS_QWS
# include "QtGui/qrgb.h"
#endif

class QSessionManager;
class QDesktopWidget;
class QStyle;
class QEventLoop;
class QIcon;
template <typename T> class QList;
class QInputContext;
#if defined(Q_WS_QWS)
class QDecoration;
#endif


class QApplication;
class QApplicationPrivate;
#define qApp (static_cast<QApplication *>(QCoreApplication::instance()))

class Q_GUI_EXPORT QApplication : public QCoreApplication
{
    Q_OBJECT
    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection)
    Q_PROPERTY(QIcon windowIcon READ windowIcon WRITE setWindowIcon)
    Q_PROPERTY(int cursorFlashTime READ cursorFlashTime WRITE setCursorFlashTime)
    Q_PROPERTY(int doubleClickInterval  READ doubleClickInterval WRITE setDoubleClickInterval)
    Q_PROPERTY(int keyboardInputInterval READ keyboardInputInterval WRITE setKeyboardInputInterval)
#ifndef QT_NO_WHEELEVENT
    Q_PROPERTY(int wheelScrollLines  READ wheelScrollLines WRITE setWheelScrollLines)
#endif
    Q_PROPERTY(QSize globalStrut READ globalStrut WRITE setGlobalStrut)
    Q_PROPERTY(int startDragTime  READ startDragTime WRITE setStartDragTime)
    Q_PROPERTY(int startDragDistance  READ startDragDistance WRITE setStartDragDistance)
    Q_PROPERTY(bool quitOnLastWindowClosed  READ quitOnLastWindowClosed WRITE setQuitOnLastWindowClosed)

public:
    QApplication(int &argc, char **argv);
    QApplication(int &argc, char **argv, bool GUIenabled);
    enum Type { Tty, GuiClient, GuiServer };
    QApplication(int &argc, char **argv, Type);
#if defined(Q_WS_X11)
    QApplication(Display* dpy, Qt::HANDLE visual = 0, Qt::HANDLE cmap = 0);
    QApplication(Display *dpy, int &argc, char **argv, Qt::HANDLE visual = 0, Qt::HANDLE cmap= 0);
#endif
    virtual ~QApplication();

    static Type type();

    static QStyle *style();
    static void setStyle(QStyle*);
    static QStyle *setStyle(const QString&);
    enum ColorSpec { NormalColor=0, CustomColor=1, ManyColor=2 };
    static int colorSpec();
    static void setColorSpec(int);

#ifndef QT_NO_CURSOR
    static QCursor *overrideCursor();
    static void setOverrideCursor(const QCursor &);
    static void changeOverrideCursor(const QCursor &);
    static void restoreOverrideCursor();
#endif
    static QPalette palette();
    static QPalette palette(const QWidget *);
    static QPalette palette(const char *className);
    static void setPalette(const QPalette &, const char* className = 0);
    static QFont font(const QWidget* = 0);
    static void setFont(const QFont &, const char* className = 0);
    static QFontMetrics fontMetrics();

    static void setWindowIcon(const QIcon &icon);
    static QIcon windowIcon();


#ifdef QT3_SUPPORT
    static QT3_SUPPORT QWidget *mainWidget();
    static QT3_SUPPORT void setMainWidget(QWidget *);
#endif

    static QWidgetList allWidgets();
    static QWidgetList topLevelWidgets();

    static QDesktopWidget *desktop();

    static QWidget *activePopupWidget();
    static QWidget *activeModalWidget();
#ifndef QT_NO_CLIPBOARD
    static QClipboard *clipboard();
#endif
    static QWidget *focusWidget();

    static QWidget *activeWindow();
    static void setActiveWindow(QWidget* act);

    static QWidget *widgetAt(const QPoint &p);
    static inline QWidget *widgetAt(int x, int y) { return widgetAt(QPoint(x, y)); }
    static QWidget *topLevelAt(const QPoint &p);
    static inline QWidget *topLevelAt(int x, int y)  { return topLevelAt(QPoint(x, y)); }

    static void syncX();
    static void beep();

    static Qt::KeyboardModifiers keyboardModifiers();
    static Qt::MouseButtons mouseButtons();

    static void setDesktopSettingsAware(bool);
    static bool desktopSettingsAware();

    static void setCursorFlashTime(int);
    static int cursorFlashTime();

    static void setDoubleClickInterval(int);
    static int doubleClickInterval();

    static void setKeyboardInputInterval(int);
    static int keyboardInputInterval();

#ifndef QT_NO_WHEELEVENT
    static void setWheelScrollLines(int);
    static int wheelScrollLines();
#endif
    static void setGlobalStrut(const QSize &);
    static QSize globalStrut();

    static void setStartDragTime(int ms);
    static int startDragTime();
    static void setStartDragDistance(int l);
    static int startDragDistance();

    static void setLayoutDirection(Qt::LayoutDirection direction);
    static Qt::LayoutDirection layoutDirection();

    static inline bool isRightToLeft() { return layoutDirection() == Qt::RightToLeft; }
    static inline bool isLeftToRight() { return layoutDirection() == Qt::LeftToRight; }

    static bool isEffectEnabled(Qt::UIEffect);
    static void setEffectEnabled(Qt::UIEffect, bool enable = true);

#if defined(Q_WS_MAC)
    virtual bool macEventFilter(EventHandlerCallRef, EventRef);
#endif
#if defined(Q_WS_X11)
    virtual bool x11EventFilter(XEvent *);
    virtual int x11ClientMessage(QWidget*, XEvent*, bool passive_only);
    int x11ProcessEvent(XEvent*);
#endif
#if defined(Q_WS_QWS)
    virtual bool qwsEventFilter(QWSEvent *);
    int qwsProcessEvent(QWSEvent*);
    void qwsSetCustomColors(QRgb *colortable, int start, int numColors);
#ifndef QT_NO_QWS_MANAGER
    static QDecoration &qwsDecoration();
    static void qwsSetDecoration(QDecoration *);
    static QDecoration *qwsSetDecoration(const QString &decoration);
#endif
#endif


#if defined(Q_WS_WIN)
    void winFocus(QWidget *, bool);
    static void winMouseButtonUp();
#endif

#ifndef QT_NO_SESSIONMANAGER
    // session management
    bool isSessionRestored() const;
    QString sessionId() const;
    QString sessionKey() const;
    virtual void commitData(QSessionManager& sm);
    virtual void saveState(QSessionManager& sm);
#endif
    void setInputContext(QInputContext *);
    QInputContext *inputContext() const;

    static int exec();
    bool notify(QObject *, QEvent *);


    static void setQuitOnLastWindowClosed(bool quit);
    static bool quitOnLastWindowClosed();

signals:
    void lastWindowClosed();

public slots:
    static void closeAllWindows();
    static void aboutQt();

protected:
#if defined(Q_WS_QWS)
    void setArgs(int, char **);
#endif
    bool event(QEvent *);
    bool compressEvent(QEvent *, QObject *receiver, QPostEventList *);

#ifdef QT3_SUPPORT
public:
    static inline QT3_SUPPORT void setReverseLayout(bool b) { setLayoutDirection(b?Qt::RightToLeft:Qt::LeftToRight); }
    static inline bool QT3_SUPPORT reverseLayout() { return layoutDirection() == Qt::RightToLeft; }
    static QT3_SUPPORT Qt::Alignment horizontalAlignment(Qt::Alignment align);
    typedef int ColorMode;
    enum { NormalColors = NormalColor, CustomColors = CustomColor };
    static inline QT3_SUPPORT ColorMode colorMode() { return static_cast<ColorMode>(colorSpec()); }
    static inline QT3_SUPPORT void setColorMode(ColorMode mode) { setColorSpec(int(mode)); }
#if defined(Q_OS_WIN32) || defined(Q_OS_CYGWIN)
    static QT3_SUPPORT Qt::WindowsVersion winVersion() { return (Qt::WindowsVersion)QSysInfo::WindowsVersion; }
#endif
#if defined(Q_OS_MAC)
    static QT3_SUPPORT Qt::MacintoshVersion macVersion() { return (Qt::MacintoshVersion)QSysInfo::MacintoshVersion; }
#endif
    inline static  QT3_SUPPORT void setOverrideCursor(const QCursor &cursor, bool replace)
        { if (replace) changeOverrideCursor(cursor); else setOverrideCursor(cursor); }
    inline static QT3_SUPPORT bool hasGlobalMouseTracking() {return true;}
    inline static QT3_SUPPORT void setGlobalMouseTracking(bool) {};
    inline static QT3_SUPPORT void flushX() { flush(); }
    static inline QT3_SUPPORT void setWinStyleHighlightColor(const QColor &c) {
        QPalette p(palette());
        p.setColor(QPalette::Highlight, c);
        setPalette(p);
    }
    static inline QT3_SUPPORT const QColor &winStyleHighlightColor()
        { return palette().color(QPalette::Active, QPalette::Highlight); }
    static inline QT3_SUPPORT void setPalette(const QPalette &pal, bool, const char* className = 0)
        { setPalette(pal, className); };
    static inline QT3_SUPPORT void setFont(const QFont &font, bool, const char* className = 0)
        { setFont(font, className); }

    static inline QT3_SUPPORT QWidget *widgetAt(int x, int y, bool child)
        { QWidget *w = widgetAt(x, y); return child ? w : (w ? w->window() : 0); }
    static inline QT3_SUPPORT QWidget *widgetAt(const QPoint &p, bool child)
        { QWidget *w = widgetAt(p); return child ? w : (w ? w->window() : 0); }
#endif // QT3_SUPPORT

private:
    Q_DISABLE_COPY(QApplication)
    Q_DECLARE_PRIVATE(QApplication)

    friend class QWidget;
    friend class QWidgetPrivate;
    friend class QETWidget;
    friend class Q3AccelManager;
    friend class QTranslator;
#ifndef QT_NO_SHORTCUT
    friend class QShortcut;
#endif
    friend class QAction;

#if defined(Q_WS_QWS)
    friend class QInputContext;
#endif
};

#endif // QAPPLICATION_H
