/**********************************************************************
** Copyright (C) 2002 Trolltech AS.  All rights reserved.
**
** This file is part of Qt Designer.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef LISTBOXRENAME_H
#define LISTBOXRENAME_H

#include <qobject.h>
#include <qlistbox.h>

class QLineEdit;

class ListBoxRename : public QObject
{
    Q_OBJECT
public:
    ListBoxRename( QListBox * eventSource, const char * name = 0 );
    bool eventFilter( QObject *, QEvent * event );

signals:
    void itemTextChanged( const QString & );

public slots:
    void showLineEdit();
    void hideLineEdit();
    void renameClickedItem();

private:
    QListBoxItem * clickedItem;
    QListBox * src;
    QLineEdit * ed;
    bool activity;
};

#endif //LISTBOXRENAME_H
