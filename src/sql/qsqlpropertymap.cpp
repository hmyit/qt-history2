/****************************************************************************
**
** Definition of QSqlPropertyMap class
**
** Created : 2000-11-20
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

#ifndef QT_NO_SQL

#include "qwidget.h"
#include "qcleanuphandler.h"
#include "qobjcoll.h"
#include "qsqlpropertymap.h"

/*!
  \class QSqlPropertyMap qsqlpropertymap.h
  \module sql
  \brief A class used for mapping SQL editor class names to SQL editor
  properties

  The SQL module uses Qt <a href="properties.html">object properties</a>
  to insert and extract values from editor widgets.

  This class is used to map SQL editor class names to the properties
  used to insert and extract values into and from the editor.

  For instance, a QLineEdit can be used to edit text strings and other
  data types in QSqlTable or QSqlForm. QLineEdit defines several
  properties, but it is only the "text" property that is used to
  insert and extract text into and from the QLineEdit. Both QSqlTable
  and QSqlForm uses a QSqlPropertyMap for inserting and extracting
  values to and from an editor widget.

  If you want to use custom editors with your QSqlTable or QSqlForm,
  you have to install your own QSqlPropertyMap for that table or form.
  Example:

  \code
  QSqlPropertyMap myMap;
  QSqlCursor      myCursor( "mytable" );
  QSqlForm        myForm;
  MySuperEditor   myEditor( this );

  myMap.insert( "MySuperEditor", "content" );
  myForm.installPropertyMap( &myMap );
  
  // Insert a field into the form that uses MySuperEditor  
  myForm.insert( &myEditor, myCursor.field( "somefield" ) );
  
  // Will update myWidget with the value from the mapped database
  // field
  myForm.readFields();
  ...
  myForm.writeFields();
  \endcode
  
  You could also replace the default QSqlPropertyMap that are used 
  if no custom maps are installed.
  \code
  
  // Keep in mind that QSqlPropertyMap takes ownership of the new
  // default factory/map
  
  MyEditorFactory * myFactory = new MyEditorFactory;
  QSqlPropertyMap * myMap = new QSqlPropertyMap;
  
  myMap->insert( "MySuperEditor", "content" );  
  QSqlEditorFactory::installDefaultFactory( myFactory );
  QSqlPropertyMap::installDefaultMap( myMap );
  ...
  
  \endcode

  \sa QSqlTable, QSqlForm, QSqlEditorFactory
*/

/*!

  Constructs a QSqlPropertyMap.
 */
QSqlPropertyMap::QSqlPropertyMap()
{
    propertyMap["QLineEdit"]    = "text";
    propertyMap["QSpinBox"]     = "value";
    propertyMap["QDial"]        = "value";
    propertyMap["QCheckButton"] = "checked";
    propertyMap["QSlider"]      = "value";
    propertyMap["QComboBox"]    = "currentItem";
    propertyMap["QDateEdit"]    = "date";
    propertyMap["QTimeEdit"]    = "time";
    propertyMap["QDateTimeEdit"]= "dateTime";
    propertyMap["QLabel"]       = "text";
}

/*!

  Destroys the QSqlPropertyMap.
 */
QSqlPropertyMap::~QSqlPropertyMap()
{
}

/*!

  Returns thw property of \a widget as a QVariant.
*/
QVariant QSqlPropertyMap::property( QWidget * widget )
{
    if( !widget ) return QVariant();
#ifdef QT_CHECK_RANGE
    if ( !propertyMap.contains( QString(widget->metaObject()->className()) ) )
	qWarning("QSqlPropertyMap::property: %s does not exist", widget->metaObject()->className() );
#endif
    return widget->property( propertyMap[ widget->metaObject()->className() ] );
}

/*!

  Sets the property associated with \a widget to \a value.

*/
void QSqlPropertyMap::setProperty( QWidget * widget, const QVariant & value )
{
    if( !widget ) return;

    widget->setProperty( propertyMap[ widget->metaObject()->className() ],
			 value );
}

/*!

  Insert a new classname/property pair, which is used for custom SQL
  field editors. There ust be a Q_PROPERTY clause in the \a classname
  class declaration for the \a property.

*/
void QSqlPropertyMap::insert( const QString & classname,
			      const QString & property )
{
    propertyMap[ classname ] = property;
}

/*!

  Removes a classname/property pair from the map.

*/
void QSqlPropertyMap::remove( const QString & classname )
{
    propertyMap.remove( classname );
}

static QSqlPropertyMap * defaultmap = 0;
static QCleanupHandler< QSqlPropertyMap > qsql_cleanup_property_map;

/*!

  Returns the application global QSqlPropertyMap.
*/
QSqlPropertyMap * QSqlPropertyMap::defaultMap()
{
    if( defaultmap == 0 ){
	defaultmap = new QSqlPropertyMap();
	qsql_cleanup_property_map.add( defaultmap );
    }
    return defaultmap;
}

/*!

  Replaces the default property map with \a map. All QSqlTable and
  QSqlForm instantiations will use this new map for inserting and
  extracting values to and from editors. <em>QSqlPropertyMap takes
  ownership of map, and destroys it when it is no longer needed. </em>
*/
void QSqlPropertyMap::installDefaultMap( QSqlPropertyMap * map )
{
    if( map == 0 ) return;
    
    if( defaultmap != 0 ){
	qsql_cleanup_property_map.remove( defaultmap );
	delete defaultmap;
    }
    defaultmap = map;
    qsql_cleanup_property_map.add( defaultmap );
}

#endif // QT_NO_SQL
