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

#ifndef QACCESSIBLECOMPAT_H
#define QACCESSIBLECOMPAT_H

#include <qaccessiblewidget.h>

#ifndef QT_NO_ACCESSIBILITY

class Q3ListView;
class Q3TextEdit;
class QIconView;
class QListBox;

class Q3AccessibleScrollView : public QAccessibleWidget
{
public:
    Q3AccessibleScrollView(QWidget *w, Role role);

    virtual int itemAt(int x, int y) const;
    virtual QRect itemRect(int item) const;
    virtual int itemCount() const;
};

class QAccessibleListView : public Q3AccessibleScrollView
{
public:
    QAccessibleListView(QWidget *o);

    int itemAt(int x, int y) const;
    QRect itemRect(int item) const;
    int itemCount() const;

    QString text(Text t, int child) const;
    Role role(int child) const;
    State state(int child) const;

    bool setSelected(int child, bool on, bool extend);
    void clearSelection();
    QVector<int> selection() const;

protected:
    Q3ListView *listView() const;
};

#ifndef QT_NO_ICONVIEW
class QAccessibleIconView : public Q3AccessibleScrollView
{
public:
    QAccessibleIconView(QWidget *o);

    int itemAt(int x, int y) const;
    QRect itemRect(int item) const;
    int itemCount() const;

    QString text(Text t, int child) const;
    Role role(int child) const;
    State state(int child) const;

    bool setSelected(int child, bool on, bool extend);
    void clearSelection();
    QVector<int> selection() const;

protected:
    QIconView *iconView() const;
};
#endif

class QAccessibleTextEdit : public Q3AccessibleScrollView
{
public:
    QAccessibleTextEdit(QWidget *o);

    int itemAt(int x, int y) const;
    QRect itemRect(int item) const;
    int itemCount() const;

    QString text(Text t, int child) const;
    void setText(Text t, int control, const QString &text);
    Role role(int child) const;

protected:
    Q3TextEdit *textEdit() const;
};

class Q3WidgetStack;

class QAccessibleWidgetStack : public QAccessibleWidget
{
public:
    QAccessibleWidgetStack(QWidget *o);

    int childCount() const;
    int indexOfChild(const QAccessibleInterface*) const;

    int childAt(int x, int y) const;

    int navigate(RelationFlag rel, int entry, QAccessibleInterface **target) const;

protected:
    Q3WidgetStack *widgetStack() const;
};

class QAccessibleListBox : public Q3AccessibleScrollView
{
public:
    QAccessibleListBox(QWidget *o);

    int itemAt(int x, int y) const;
    QRect itemRect(int item) const;
    int itemCount() const;

    QString text(Text t, int child) const;
    Role role(int child) const;
    State state(int child) const;

    bool setSelected(int child, bool on, bool extend);
    void clearSelection();
    QVector<int> selection() const;

protected:
    QListBox *listBox() const;
};


#endif // QT_NO_ACCESSIBILITY
#endif
