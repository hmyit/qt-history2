/****************************************************************************
**
** Implementation of QSqlNavigator class
**
** Created : 2000-11-03
**
** Copyright (C) 2000 Trolltech AS.  All rights reserved.
**
** This file is part of the sql module of the Qt GUI Toolkit.
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
** Licensees holding valid Qt Enterprise Edition licenses may use this
** file in accordance with the Qt Commercial License Agreement provided
** with the Software.
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

#include "qsqlnavigator.h"

#ifndef QT_NO_SQL

#include "qsqlcursor.h"
#include "qsqlresult.h"
#include "qsqlform.h"

/*!
  \class QSqlNavigator qsqlnavigator.h
  \brief Navigate a database cursor/form

  \module sql

  This class //###

*/

/*! \enum Boundy

  This enum type describes where the navigator is currently positioned.

  The currently defined values are:
  <ul>

  <li> \c Unknown - the boundry cannot be determined (usually because
  there is no default cursor).

  <li> \c None - the navigator is not positioned on a boundry.

  <li> \c BeforeBeginning - the navigator is positioned before the
  beginning of the available records.

  <li> \c Beginning - the navigator is positioned at the beginning of
  the available records.

  <li> \c End - the navigator is positioned at the end of
  the available records.

  <li> \c AfterEnd - the navigator is positioned after the end of the
  available records.

  </ul>
*/

/*!  Constructs a navigator.

*/

QSqlNavigator::QSqlNavigator()
{
}

/*!  Reads the fields from the default form and performs an insert on
  the default cursor.  Returns 1 if the insert was successfull,
  otherwise 0 is returned.  If an error occurred during the insert
  into the database, handleError() is called.

  \sa defaultCursor() defaultForm() handleError()

*/

int QSqlNavigator::insertRecord()
{
    QSqlCursor* cursor = defaultCursor();
    QSqlForm* form = defaultForm();
    if ( !cursor || !form )
	return 0;
    form->writeFields();
    int ar = cursor->insert();
    if ( !ar || !cursor->isActive() )
	handleError( cursor->lastError() );
    // ### must seek to inserted record
    return ar;
}

/*!  Reads the fields from the default form and performs an update on
  the default cursor. Returns 1 if the update was successfull,
  otherwise 0 is returned.  If an error occurred during the update on
  the database, handleError() is called.

*/

int QSqlNavigator::updateRecord()
{
    QSqlCursor* cursor = defaultCursor();
    QSqlForm* form = defaultForm();
    if ( !cursor || !form )
	return 0;
    form->writeFields();
    int n = cursor->at();
    int ar = cursor->update();
    if ( !ar || !cursor->isActive() )
	handleError( cursor->lastError() );
    cursor->select( cursor->filter(), cursor->sort() );
    cursor->seek( n );
    return ar;
}

/*!  Performs a delete on the default cursor and updates the default
  form.  Returns 1 if the delete was successfull, otherwise 0 is
  returned.  If an error occurred during the delete from the database,
  handleError() is called.


*/

int QSqlNavigator::deleteRecord()
{
    QSqlCursor* cursor = defaultCursor();
    QSqlForm* form = defaultForm();
    if ( !cursor || !form )
	return 0;
    int n = cursor->at();
    int ar = cursor->del();
    if ( ar ) {
	cursor->select( cursor->filter(), cursor->sort() );
	if ( !cursor->seek( n ) )
	    cursor->last();
	cursor->primeUpdate();
	QSqlForm* form = defaultForm();
	if ( form )
	    form->readFields();
    } else {
	if ( !cursor->isActive() )
	    handleError( cursor->lastError() );
    }
    return ar;
}

/*!  Moves the default cursor to the first record and updates the
default form.  Returns TRUE if the navigator successfully moved to the
first record, otherwise FALSE is returned.

*/

bool QSqlNavigator::firstRecord()
{
    QSqlCursor* cursor = defaultCursor();
    if ( !cursor )
	return FALSE;
    if ( cursor->first() ) {
	cursor->primeUpdate();
	QSqlForm* form = defaultForm();
	if ( form )
	    form->readFields();
	return TRUE;
    }
    return FALSE;
}

/*!  Moves the default cursor to the last record and updates the
default form.  Returns TRUE if the navigator successfully moved to the
last record, otherwise FALSE is returned.

*/

bool QSqlNavigator::lastRecord()
{
    QSqlCursor* cursor = defaultCursor();
    if ( !cursor )
	return FALSE;
    if ( cursor->last() ) {
	cursor->primeUpdate();
	QSqlForm* form = defaultForm();
	if ( form )
	    form->readFields();
	return TRUE;
    }
    return FALSE;
}

/*!  Moves the default cursor to the next record and updates the
default form.  Returns TRUE if the navigator successfully moved to the
next record.  Otherwise, the navigator moves the default cursor to the
last record and FALSE is returned.

*/

bool QSqlNavigator::nextRecord()
{
    QSqlCursor* cursor = defaultCursor();
    if ( !cursor )
	return FALSE;
    bool b = cursor->next();
    if( !b )
	cursor->last();
    cursor->primeUpdate();
    QSqlForm* form = defaultForm();
    if ( form )
	form->readFields();
    return b;
}

/*!  Moves the default cursor to the previous record and updates the
default form.  Returns TRUE if the navigator successfully moved to the
previous record.  Otherwise, the navigator moves the default cursor to
the first record and FALSE is returned.

*/

bool QSqlNavigator::prevRecord()
{
    QSqlCursor* cursor = defaultCursor();
    if ( !cursor )
	return FALSE;
    bool b = cursor->prev();
    if( !b )
	cursor->first();
    cursor->primeUpdate();
    QSqlForm* form = defaultForm();
    if ( form )
	form->readFields();
    return b;
}

/*!  Clears the default cursor values and clears the widgets in the
default form.

*/

void QSqlNavigator::clearForm()
{
    QSqlCursor* cursor = defaultCursor();
    if ( cursor )
	cursor->editBuffer()->clearValues();
    QSqlForm* form = defaultForm();
    if ( form )
	form->clearValues();
}

/*! Returns a pointer to the default cursor used for navigation, or 0
if there is no default cursor.  The default implementation returns 0.

*/

QSqlCursor* QSqlNavigator::defaultCursor()
{
    return 0;
}


/*! Returns a pointer to the default form used during navigation, or 0
if there is no default form.  The default implementation returns 0.

*/

QSqlForm* QSqlNavigator::defaultForm()
{
    return 0;
}


/*!  Virtual function which is called when an error has occurred on
  the default cursor.  The default implementation does nothing.

*/

void QSqlNavigator::handleError( const QSqlError& )
{
}

/*! Returns an enum indicating the boundry status of the navigator.
This is done by moving the default cursor and checking the position,
however the current default form values will not be altered.  After
checking for the boundry, the cursor is moved back to its former
position.

  \sa Boundry
*/
QSqlNavigator::Boundry QSqlNavigator::boundry()
{
    QSqlCursor* cursor = defaultCursor();
    if ( !cursor )
	return Unknown;
    if ( !cursor->isActive() )
	return Unknown;
    if ( !cursor->isValid() ) {
	if ( cursor->at() == QSqlResult::BeforeFirst )
	    return BeforeBeginning;
	if ( cursor->at() == QSqlResult::AfterLast )
	    return AfterEnd;
	return Unknown;
    }
    if ( cursor->at() == 0 )
	return Beginning;
    // otherwise...
    int currentAt = cursor->at();
    Boundry b = None;
    if ( !cursor->prev() )
	b = Beginning;
    if ( b == None && !cursor->next() )
	b = End;
    cursor->seek( currentAt );
    return b;
}

#endif
