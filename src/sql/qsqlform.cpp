#include "qobjcoll.h"
#include "qsqlfield.h"
#include "qsqlform.h"
#include "qsqlview.h"

#ifndef QT_NO_SQL

/*!
  \class QSqlPropertyMap qsqlform.h
  \brief Class used for mapping SQL editor values to SQL fields and vice versa
  
  \module sql

  This class is used to associate a class with a specific property. This
  is used on the GUI side of the database module to map SQL field
  editor data to SQL fields and vice versa.
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
}

/*!
  
  Returns the QVariant which is a property of \a object.
*/
QVariant QSqlPropertyMap::property( QObject * object )
{
    if( !object ) return QVariant();
    
    return object->property( propertyMap[ object->metaObject()->className() ] );
}

/*!
  
  Sets the property associated with \a object to \a value.
*/
void QSqlPropertyMap::setProperty( QObject * object, const QVariant & value )
{
    if( !object ) return;
    
    object->setProperty( propertyMap[ object->metaObject()->className() ], 
			 value );
}

/*!
  
  Add a new classname/property pair, which is used for custom SQL
  field editors. Remember to add a Q_PROPERTY clause in the \a classname
  class declaration.
*/
void QSqlPropertyMap::addClass( const QString & classname, 
				const QString & property )
{
    propertyMap[ classname ] = property;
}


/*!
  \class QSqlFormMap qsqlform.h
  \brief Class used for mapping SQL fields to Qt widgets and vice versa
  
  \module sql

  This class is used to associate a class with a specific property. This
  is used on the GUI side of the database module to map SQL fields
  to Qt widgets and vice versa.
 */

/*!
  
  Constructs a QSqlFormMap.
*/
QSqlFormMap::QSqlFormMap()
{
    m = new QSqlPropertyMap;
}

/*!
  
  Destructor.
*/
QSqlFormMap::~QSqlFormMap()
{
    if( m ) 
	delete m;
}

/*!
  
  Installs a custom QSqlPropertyMap.
*/
void QSqlFormMap::installPropertyMap( QSqlPropertyMap * map )
{
    if( m )
	delete m;
    
    if( map )
	m = map;
    else
	m = new QSqlPropertyMap;
}


/*!

  Add a widget/field pair to the current map.
*/
void QSqlFormMap::add( QWidget * widget, QSqlField * field )
{
    map[widget] = field;
}
    
/*!

  Returns the field number widget \a widget is mapped to.
*/
QSqlField * QSqlFormMap::whichField( QWidget * widget ) const 
{
    if( map.contains( widget ) )
	return map[widget];
    else
	return 0;
}

/*!

  Returns the widget which field \a field is mapped to.
*/
QWidget * QSqlFormMap::whichWidget( QSqlField * field ) const
{
    QMap< QWidget *, QSqlField * >::ConstIterator it;
    for( it = map.begin(); it != map.end(); ++it ){
	if( *it == field )
	    return it.key();
    }
    return 0;
}

/*!
  
  Update the widgets in the map with values from the actual database
  fields.
*/
void QSqlFormMap::syncWidgets()
{
    QSqlField * f;
    QMap< QWidget *, QSqlField * >::Iterator it;
	 
    for(it = map.begin() ; it != map.end(); ++it ){
	f = whichField( it.key() );
	if( !f ) continue;
	m->setProperty( it.key(), f->value() );
    }       
}

/*!
  
  Update the actual database fields with values from the widgets.
*/  
void QSqlFormMap::syncFields()
{
    QSqlField * f;
    QMap< QWidget *, QSqlField * >::Iterator it;
	 
    for(it = map.begin() ; it != map.end(); ++it ){
	f = whichField( it.key() );
	if( !f ) continue;
	f->setValue( m->property( it.key() ) );
    }       
}

/*!
  
  \class QSqlForm qsqlform.h
  \brief Class used for creating SQL forms
  
  \module sql
  
  This class can be used to create SQL form widgets for accessing,
  updating, inserting and deleting data from a database easily.  
*/

/*!
  
  Constructs a SQL form.
*/
QSqlForm::QSqlForm( QWidget * parent, const char * name )
    : QWidget( parent, name )
{
    v = 0;
    map = new QSqlFormMap();
}
/*!
  
  Destructor.
*/
QSqlForm::~QSqlForm()
{
    delete map;
}

/*!
  
  This function is used to associate a widget with a database field.
*/
void QSqlForm::associate( QWidget * widget, QSqlField * field )
{
    map->add( widget, field );
}

/*!
  
  Set the SQL view that the widgets in the form should be associated
  with. <em> Do not delete the \a view until the QSqlForm goes out of
  scope.<\em> 
*/
void QSqlForm::setView( QSqlView * view )
{
    v = view;
}

/*!
  
  Returns the QSqlView that this form is associated with.
*/
QSqlView * QSqlForm::view() const
{
    return v;
}

/*!
  
  Refresh the widgets in the form with values from the associated SQL
  fields.
*/
void QSqlForm::syncWidgets()
{
    if( v ){
	map->syncWidgets();
	emit recordChanged( v->at() );
    } else
	qWarning( "QSqlForm: No view associated with this form." );
}

/*!
  
  Refresh the SQL fields with values from the associated widgets.
*/
void QSqlForm::syncFields()
{
    if( v )
	map->syncFields();
    else
	qWarning( "QSqlForm: No view associated with this form." );
}

void QSqlForm::first()
{
    if( v && v->first() ){
	syncWidgets();
    }
}

void QSqlForm::last()
{
    if( v && v->last() ){
	syncWidgets();
    }
}

void QSqlForm::next()
{
    if( v && v->next() ){
	syncWidgets();
    }
}

void QSqlForm::previous()
{
    if( v && v->previous() ){
	syncWidgets();
    }
}

bool QSqlForm::insert()
{
    if( v ){
	syncFields();
	v->insert();
	v->select( v->filter(), v->sort() );
	v->last();
	syncWidgets();
	return TRUE;
    }
    return FALSE;
}

bool QSqlForm::update()
{
    if( v ){
	syncFields();
	int at = v->at();
	if( v->update( v->primaryIndex() ) && 
	    v->select( v->filter(), v->sort() ) &&
	    v->seek( at ) )
	    return TRUE;
    }
    
    return FALSE;
}

bool QSqlForm::del()
{
    if( v && v->del( v->primaryIndex() ) ){
	v->select( v->filter(), v->sort() );
	v->first();
	syncWidgets();
	return TRUE;
    }
    return FALSE;
}

void QSqlForm::seek( int i )
{
    if( v && v->seek(i) ){
	syncWidgets();
    }
}
#endif // QT_NO_SQL
