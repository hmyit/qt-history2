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

#ifndef QMENU_H
#define QMENU_H

#include <QtGui/qwidget.h>
#include <QtCore/qstring.h>
#include <QtGui/qicon.h>
#include <QtGui/qaction.h>

class QMenuPrivate;
#ifdef QT3_SUPPORT
class QMenuItem;
#include <QtGui/qpixmap.h>
#endif

class Q_GUI_EXPORT QMenu : public QWidget
{
private:
    Q_OBJECT
    Q_DECLARE_PRIVATE(QMenu)

    Q_PROPERTY(bool tearOffEnabled READ isTearOffEnabled WRITE setTearOffEnabled)
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(QIcon icon READ icon WRITE setIcon)

public:
    explicit QMenu(QWidget *parent = 0);
    explicit QMenu(const QString &title, QWidget *parent = 0);
    ~QMenu();

#ifdef Q_NO_USING_KEYWORD
    inline void addAction(QAction *action) { QWidget::addAction(action); }
#else
    using QWidget::addAction;
#endif
    QAction *addAction(const QString &text);
    QAction *addAction(const QIcon &icon, const QString &text);
    QAction *addAction(const QString &text, const QObject *receiver, const char* member, const QKeySequence &shortcut = 0);
    QAction *addAction(const QIcon &icon, const QString &text, const QObject *receiver, const char* member, const QKeySequence &shortcut = 0);

    QAction *addMenu(QMenu *menu);
    QMenu *addMenu(const QString &title);
    QMenu *addMenu(const QIcon &icon, const QString &title);

    QAction *addSeparator();

    QAction *insertMenu(QAction *before, QMenu *menu);
    QAction *insertSeparator(QAction *before);

    void clear();

    void setTearOffEnabled(bool);
    bool isTearOffEnabled() const;

    bool isTearOffMenuVisible() const;
    void hideTearOffMenu();

    void setDefaultAction(QAction *);
    QAction *defaultAction() const;

    void setActiveAction(QAction *act);
    QAction *activeAction() const;

    void popup(const QPoint &pos, QAction *at=0);
    QAction *exec();
    QAction *exec(const QPoint &pos, QAction *at=0);
    static QAction *exec(QList<QAction*> actions, const QPoint &pos, QAction *at=0);

    QSize sizeHint() const;

    QRect actionGeometry(QAction *) const;
    QAction *actionAt(const QPoint &) const;

    QAction *menuAction() const;

    QString title() const;
    void setTitle(const QString &title);

    QIcon icon() const;
    void setIcon(const QIcon &icon);

    void setNoReplayFor(QWidget *widget);
#ifdef Q_WS_MAC
    MenuRef macMenu(MenuRef merge=0);
#endif

signals:
    void aboutToShow();
    void triggered(QAction *action);
    void hovered(QAction *action);

protected:
    int columnCount() const;

    void changeEvent(QEvent *);
    void keyPressEvent(QKeyEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void wheelEvent(QWheelEvent *);
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
    void hideEvent(QHideEvent *);
    void paintEvent(QPaintEvent *);
    void actionEvent(QActionEvent *);
    void timerEvent(QTimerEvent *);
    bool event(QEvent *);

private slots:
    void internalSetSloppyAction();
    void internalDelayedPopup();

#ifdef QT3_SUPPORT
public:
    //menudata
    inline QT3_SUPPORT uint count() const { return actions().count(); }
    inline QT3_SUPPORT int insertItem(const QString &text, const QObject *receiver, const char* member,
                                    const QKeySequence& shortcut = 0, int id = -1, int index = -1) {
        return insertAny(0, &text, receiver, member, &shortcut, 0, id, index);
    }
    inline QT3_SUPPORT int insertItem(const QIcon& icon, const QString &text,
                                    const QObject *receiver, const char* member,
                                    const QKeySequence& shortcut = 0, int id = -1, int index = -1) {
        return insertAny(&icon, &text, receiver, member, &shortcut, 0, id, index);
    }
    inline QT3_SUPPORT int insertItem(const QPixmap &pixmap, const QObject *receiver, const char* member,
                                    const QKeySequence& shortcut = 0, int id = -1, int index = -1) {
        QIcon icon(pixmap);
        return insertAny(&icon, 0, receiver, member, &shortcut, 0, id, index);
    }
    inline QT3_SUPPORT int insertItem(const QString &text, int id=-1, int index=-1) {
        return insertAny(0, &text, 0, 0, 0, 0, id, index);
    }
    inline QT3_SUPPORT int insertItem(const QIcon& icon, const QString &text, int id=-1, int index=-1) {
        return insertAny(&icon, &text, 0, 0, 0, 0, id, index);
    }
    inline QT3_SUPPORT int insertItem(const QString &text, QMenu *popup, int id=-1, int index=-1) {
        return insertAny(0, &text, 0, 0, 0, popup, id, index);
    }
    inline QT3_SUPPORT int insertItem(const QIcon& icon, const QString &text, QMenu *popup, int id=-1, int index=-1) {
        return insertAny(&icon, &text, 0, 0, 0, popup, id, index);
    }
    inline QT3_SUPPORT int insertItem(const QPixmap &pixmap, int id=-1, int index=-1) {
        QIcon icon(pixmap);
        return insertAny(&icon, 0, 0, 0, 0, 0, id, index);
    }
    inline QT3_SUPPORT int insertItem(const QPixmap &pixmap, QMenu *popup, int id=-1, int index=-1) {
        QIcon icon(pixmap);
        return insertAny(&icon, 0, 0, 0, 0, popup, id, index);
    }
    QT3_SUPPORT int insertSeparator(int index=-1);
    inline QT3_SUPPORT void removeItem(int id) {
        if(QAction *act = findActionForId(id))
            removeAction(act); }
    inline QT3_SUPPORT void removeItemAt(int index) {
        if(QAction *act = actions().value(index))
            removeAction(act); }
#ifndef QT_NO_ACCEL
    inline QT3_SUPPORT QKeySequence accel(int id) const {
        if(QAction *act = findActionForId(id))
            return act->shortcut();
        return QKeySequence(); }
    inline QT3_SUPPORT void setAccel(const QKeySequence& key, int id) {
        if(QAction *act = findActionForId(id)) {
            qDebug("act= %p", act);
            act->setShortcut(key);
        }
    }
#endif
    inline QT3_SUPPORT QIcon iconSet(int id) const {
        if(QAction *act = findActionForId(id))
            return act->icon();
        return QIcon(); }
    inline QT3_SUPPORT QString text(int id) const {
        if(QAction *act = findActionForId(id))
            return act->text();
        return QString(); }
    inline QT3_SUPPORT QPixmap pixmap(int id) const {
        if(QAction *act = findActionForId(id))
            return act->icon().pixmap(QSize(22, 22));
        return QPixmap(); }
    inline QT3_SUPPORT void setWhatsThis(int id, const QString &w) {
        if(QAction *act = findActionForId(id))
            act->setWhatsThis(w); }
    inline QT3_SUPPORT QString whatsThis(int id) const {
        if(QAction *act = findActionForId(id))
            return act->whatsThis();
        return QString(); }

    inline QT3_SUPPORT void changeItem(int id, const QString &text) {
        if(QAction *act = findActionForId(id))
            act->setText(text); }
    inline QT3_SUPPORT void changeItem(int id, const QPixmap &pixmap) {
        if(QAction *act = findActionForId(id))
            act->setIcon(QIcon(pixmap)); }
    inline QT3_SUPPORT void changeItem(int id, const QIcon &icon, const QString &text) {
        if(QAction *act = findActionForId(id)) {
            act->setIcon(icon);
            act->setText(text);
        }
    }
    inline QT3_SUPPORT bool isItemActive(int id) const {
        return findActionForId(id) == activeAction();
    }
    inline QT3_SUPPORT bool isItemEnabled(int id) const {
        if(QAction *act = findActionForId(id))
            return act->isEnabled();
        return false; }
    inline QT3_SUPPORT void setItemEnabled(int id, bool enable) {
        if(QAction *act = findActionForId(id))
            act->setEnabled(enable);
    }
    inline QT3_SUPPORT bool isItemChecked(int id) const {
        if(QAction *act = findActionForId(id))
            return act->isChecked();
        return false;
    }
    inline QT3_SUPPORT void setItemChecked(int id, bool check) {
        if(QAction *act = findActionForId(id))
            act->setChecked(check);
    }
    inline QT3_SUPPORT bool isItemVisible(int id) const {
        if(QAction *act = findActionForId(id))
            return act->isVisible();
        return false;
    }
    inline QT3_SUPPORT void setItemVisible(int id, bool visible) {
        if(QAction *act = findActionForId(id))
            act->setVisible(visible);
    }
    inline QT3_SUPPORT QRect itemGeometry(int index) {
        if(QAction *act = actions().value(index))
            return actionGeometry(act);
        return QRect();
    }
    inline QT3_SUPPORT QFont itemFont(int id) const {
        if(QAction *act = findActionForId(id))
            return act->font();
        return QFont();
    }
    inline QT3_SUPPORT void setItemFont(int id, const QFont &font) {
        if(QAction *act = findActionForId(id))
            act->setFont(font);
    }
    inline QT3_SUPPORT int indexOf(int id) const {
        return actions().indexOf(findActionForId(id));
    }
    inline QT3_SUPPORT int idAt(int index) const {
        return findIdForAction(actions().value(index));
    }
    inline QT3_SUPPORT void activateItemAt(int index) {
        if(QAction *ret = actions().value(index))
            ret->activate(QAction::Trigger);
    }
    inline QT3_SUPPORT bool connectItem(int id, const QObject *receiver, const char* member) {
        if(QAction *act = findActionForId(id)) {
            QObject::connect(act, SIGNAL(activated(int)), receiver, member, Qt::DirectCompatConnection);
            return true;
        }
        return false;
    }
    inline QT3_SUPPORT bool disconnectItem(int id,const QObject *receiver, const char* member) {
        if(QAction *act = findActionForId(id)) {
            QObject::disconnect(act, SIGNAL(triggered()), receiver, member);
            return true;
        }
        return false;
    }
    inline QT3_SUPPORT QMenuItem *findItem(int id) const {
        return reinterpret_cast<QMenuItem*>(findActionForId(id));
    }

    inline QT3_SUPPORT void setCheckable(bool){}
    inline QT3_SUPPORT bool isCheckable() const {return true;}

    QT3_SUPPORT QMenuItem *findPopup( QMenu *popup, int *index );

    QT3_SUPPORT bool setItemParameter(int id, int param);
    QT3_SUPPORT int itemParameter(int id) const;

    //frame
    QT3_SUPPORT int frameWidth() const;

    //popupmenu
    inline QT3_SUPPORT void popup(const QPoint & pos, int indexAtPoint) { popup(pos, actions().value(indexAtPoint)); }
    inline QT3_SUPPORT int insertTearOffHandle(int = 0, int = 0) {
        setTearOffEnabled(true);
        return -1;
    }

protected:
    inline QT3_SUPPORT int itemAtPos(const QPoint &p, bool ignoreSeparator = true) {
        QAction *ret = actionAt(p);
        if(ignoreSeparator && ret && ret->isSeparator())
            return -1;
        return findIdForAction(ret);
    }
    inline QT3_SUPPORT int columns() const { return columnCount(); }
    inline QT3_SUPPORT int itemHeight(int index) {
        return actionGeometry(actions().value(index)).height();
    }
    inline QT3_SUPPORT int itemHeight(QMenuItem *mi) {
        return actionGeometry(reinterpret_cast<QAction *>(mi)).height();
    }

signals:
    QT_MOC_COMPAT void aboutToHide();
    QT_MOC_COMPAT void activated(int itemId);
    QT_MOC_COMPAT void highlighted(int itemId);

private:
    Q_PRIVATE_SLOT(d, void actionTriggered())
    Q_PRIVATE_SLOT(d, void actionHovered())

    int insertAny(const QIcon *icon, const QString *text, const QObject *receiver, const char *member,
                  const QKeySequence *shorcut, const QMenu *popup, int id, int index);
    QAction *findActionForId(int id) const;
    int findIdForAction(QAction*) const;
#endif

private:
    Q_DISABLE_COPY(QMenu)

    friend class QMenuBar;
    friend class QMenuBarPrivate;
    friend class QTornOffMenu;
    friend class Q3PopupMenu;
    friend class QComboBox;

#ifdef Q_WS_MAC
    friend bool watchingAboutToShow(QMenu *);
    friend OSStatus qt_mac_menu_event(EventHandlerCallRef, EventRef, void *);
    friend bool qt_mac_activate_action(MenuRef, uint, QAction::ActionEvent, bool);
#endif
};

#endif // QMENU_H
