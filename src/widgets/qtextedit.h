/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qtextview.h#6 $
**
** Definition of the QTextEdit class
**
** Created : 990101
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef QTEXTEDIT_H
#define QTEXTEDIT_H

#ifndef QT_H
#include "qlist.h"
#include "qpixmap.h"
#include "qcolor.h"
#include "qtextview.h"
#endif // QT_H

#ifndef QT_NO_TEXTEDIT

class Q_EXPORT QTextEdit : public QTextView
{
    Q_OBJECT

public:
    QTextEdit(QWidget *parent = 0, const char *name = 0 );
    ~QTextEdit();

private:
    bool isReadOnly() const { return FALSE; }

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QTextEdit( const QTextEdit & );
    QTextEdit& operator=( const QTextEdit & );
#endif
};

#endif // QT_NO_TEXTEDIT

#endif
