/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qhbuttongroup.cpp#2 $
**
** Implementation of QHButtonGroup class
**
** Created : 990602
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.troll.no/pricing.html or email sales@troll.no for
** information about the Professional Edition licensing, or see
** http://www.troll.no/qpl/ for QPL licensing information.
**
*****************************************************************************/

#include "qhbuttongroup.h"

// NOT REVISED
/*!
  \class QHButtonGroup qhbuttongroup.h

  \brief The QHButtonGroup widget organizes QButton widgets in a group
  with one horizontal row.

  \ingroup realwidgets

  QHButtonGroup is a convenience class that offers a thin layer on top of
  QButtonGroup. Think of it as a QHBox that offers a frame with a title
  and is specifically designed for buttons.
*/

/*!
  Constructs a horizontal button group with no title.

  The \e parent and \e name arguments are passed to the QWidget constructor.
*/
QHButtonGroup::QHButtonGroup( QWidget *parent, const char *name )
    : QButtonGroup( 1, Vertical /* sic! */, parent, name )
{
}

/*!
  Constructs a horizontal button group with a title.

  The \e parent and \e name arguments are passed to the QWidget constructor.
*/

QHButtonGroup::QHButtonGroup( const QString &title, QWidget *parent,
			    const char *name )
    : QButtonGroup( 1, Vertical /* sic! */, title, parent, name )
{
}

/*!
  Destroys the horizontal button group and its child widgets.
*/
QHButtonGroup::~QHButtonGroup()
{
}
