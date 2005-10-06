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

#ifndef QABSTRACTBUTTON_H
#define QABSTRACTBUTTON_H

#include <QtGui/qicon.h>
#include <QtGui/qkeysequence.h>
#include <QtGui/qwidget.h>

QT_MODULE(Gui)

class QButtonGroup;
class QAbstractButtonPrivate;

class Q_GUI_EXPORT QAbstractButton : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QIcon icon READ icon WRITE setIcon)
    Q_PROPERTY(QSize iconSize READ iconSize WRITE setIconSize)
#ifndef QT_NO_SHORTCUT
    Q_PROPERTY(QKeySequence shortcut READ shortcut WRITE setShortcut)
#endif
    Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable)
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
    Q_PROPERTY(bool autoRepeat READ autoRepeat WRITE setAutoRepeat)
    Q_PROPERTY(bool autoExclusive READ autoExclusive WRITE setAutoExclusive)
    Q_PROPERTY(bool down READ isDown WRITE setDown DESIGNABLE false)

public:
    explicit QAbstractButton(QWidget* parent=0);
    ~QAbstractButton();

    void setText(const QString &text);
    QString text() const;

    void setIcon(const QIcon &icon);
    QIcon icon() const;

    QSize iconSize() const;

#ifndef QT_NO_SHORTCUT
    void setShortcut(const QKeySequence &key);
    QKeySequence shortcut() const;
#endif

    void setCheckable(bool);
    bool isCheckable() const;

    bool isChecked() const;

    void setDown(bool);
    bool isDown() const;

    void setAutoRepeat(bool);
    bool autoRepeat() const;

    void setAutoExclusive(bool);
    bool autoExclusive() const;

#ifndef QT_NO_BUTTONGROUP
    QButtonGroup *group() const;
#endif

public Q_SLOTS:
    void setIconSize(const QSize &size);
    void animateClick(int msec = 100);
    void click();
    void toggle();
    void setChecked(bool);

Q_SIGNALS:
    void pressed();
    void released();
    void clicked(bool checked = false);
    void toggled(bool checked);

protected:
    virtual void paintEvent(QPaintEvent *e) = 0;
    virtual bool hitButton(const QPoint &pos) const;
    virtual void checkStateSet();
    virtual void nextCheckState();

    bool event(QEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void focusInEvent(QFocusEvent *e);
    void focusOutEvent(QFocusEvent *e);
    void changeEvent(QEvent *e);
    void timerEvent(QTimerEvent *e);

#ifdef QT3_SUPPORT
public:
    QT3_SUPPORT_CONSTRUCTOR QAbstractButton(QWidget *parent, const char *name, Qt::WFlags f=0);
    inline QT3_SUPPORT bool isOn() const { return isChecked(); }
    inline QT3_SUPPORT const QPixmap *pixmap() const { return 0; } // help styles compile
    inline QT3_SUPPORT void setPixmap( const QPixmap &p ) { setIcon(QIcon(p)); }
    QT3_SUPPORT QIcon *iconSet() const;
    inline QT3_SUPPORT void setIconSet(const QIcon &icon) { setIcon(icon); }
    inline QT3_SUPPORT bool isToggleButton() const { return isCheckable(); }
    inline QT3_SUPPORT void setToggleButton(bool b) { setCheckable(b); }
    inline QT3_SUPPORT void setAccel(const QKeySequence &key) { setShortcut(key); }
    inline QT3_SUPPORT QKeySequence accel() const { return shortcut(); }

public Q_SLOTS:
    inline QT_MOC_COMPAT void setOn(bool b) { setChecked(b); }
#endif

protected:
    QAbstractButton(QAbstractButtonPrivate &dd, QWidget* parent = 0);

private:
    Q_DECLARE_PRIVATE(QAbstractButton)
    Q_DISABLE_COPY(QAbstractButton)
    friend class QButtonGroup;
};

#endif // QABSTRACTBUTTON_H
