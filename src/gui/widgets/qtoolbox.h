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

#ifndef QTOOLBOX_H
#define QTOOLBOX_H

#ifndef QT_H
#include <qframe.h>
#include <qiconset.h>
#endif // QT_H

#ifndef QT_NO_TOOLBOX

class QToolBoxPrivate;

class Q_GUI_EXPORT QToolBox : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex)
    Q_PROPERTY(int count READ count)

public:
#ifdef QT_COMPAT
    QT_COMPAT_CONSTRUCTOR QToolBox(QWidget *parent, const char *name, Qt::WFlags f = 0);
#endif
    QToolBox(QWidget *parent = 0, Qt::WFlags f = 0);
    ~QToolBox();

    int addItem(QWidget *widget, const QString &text);
    int addItem(QWidget *widget, const QIconSet &icon, const QString &text);
    int insertItem(int index, QWidget *widget, const QString &text);
    int insertItem(int index, QWidget *widget, const QIconSet &icon, const QString &text);

    void removeItem(int index);

    void setItemEnabled(int index, bool enabled);
    bool isItemEnabled(int index) const;

    void setItemText(int index, const QString &text);
    QString itemText(int index) const;

    void setItemIcon(int index, const QIconSet &icon);
    QIconSet itemIcon(int index) const;

    void setItemToolTip(int index, const QString &toolTip);
    QString itemToolTip(int index) const;

    int currentIndex() const;
    QWidget *widget(int index) const;
    int indexOf(QWidget *widget) const;
    int count() const;

public slots:
    void setCurrentIndex(int index);

signals:
    void currentChanged(int index);

private slots:
    void buttonClicked();
    void widgetDestroyed(QObject*);

protected:
    virtual void itemInserted(int index);
    virtual void itemRemoved(int index);
    void showEvent(QShowEvent *e);
    void changeEvent(QEvent *);

private:
    void relayout();

private:
    QToolBoxPrivate *d;

#ifdef QT_COMPAT
public:
    inline QT_COMPAT void setItemLabel(int index, const QString &text) { setItemText(index, text); }
    inline QT_COMPAT QString itemLabel(int index) const { return itemText(index); }
    inline QT_COMPAT QWidget *currentItem() const { return widget(currentIndex()); }
    inline QT_COMPAT void setCurrentItem(QWidget *item) { setCurrentIndex(indexOf(item)); }
    inline QT_COMPAT void setItemIconSet(int index, const QIconSet &icon) { setItemIcon(index, icon); }
    inline QT_COMPAT QIconSet itemIconSet(int index) const { return itemIcon(index); }
    inline QT_COMPAT int removeItem(QWidget *item)
    { int i = indexOf(item); removeItem(i); return i; }
    inline QT_COMPAT QWidget *item(int index) const { return widget(index); }
#endif
};


inline int QToolBox::addItem(QWidget *item, const QString &text)
{ return insertItem(-1, item, QIconSet(), text); }
inline int QToolBox::addItem(QWidget *item, const QIconSet &iconSet,
                              const QString &text)
{ return insertItem(-1, item, iconSet, text); }
inline int QToolBox::insertItem(int index, QWidget *item, const QString &text)
{ return insertItem(index, item, QIconSet(), text); }

#endif // QT_NO_TOOLBOX
#endif
