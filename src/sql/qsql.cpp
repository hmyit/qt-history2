#include "qsql.h"

#ifndef QT_NO_SQL

#include "qsqlresult.h"
#include "qsqldriver.h"
#include "qsqldatabase.h"
#include "qsqlconnection.h"

QSqlResultShared::~QSqlResultShared()
{
    if ( sqlResult )
	delete sqlResult;
}

/*! \class QSql qsql.h

    \brief Class used for executing and manipulating SQL queries.

    \module database

    This class is used to execute SQL queries on a QSqlDatabase.  QSql
    encapsulates the functionality involved in creating, navigating
    and retrieving data from SQL queries.  This class works with QSqlResult
    to form a simple, but flexible, interface to SQL database engines.

    Once an SQL result is created, it is initially in an inactive state.
    A result becomes active when it is supplied with a SQL query (see operator<<) (see
    isActive()).  Alternatively, use the QSqlDatabase::query() or QSqlDatabase::exec()
    convenience methods to create active QSql objects.

    An active result object must be 'scrolled' to valid records within the
    SQL query as generated by the SQL database.  Data cannot be accessed until
    the result is positioned on a valid record (see isValid()).  To 'scroll', or
    navigate, through the result set, use the following methods:

    <ul>
    <li>next()
    <li>previous()
    <li>first()
    <li>last()
    <li>seek()
    </ul>

    These methods allow the programmer to move forward, backward or randomly through the records
    returned by the query.  Once an active result object is positioned on a valid record, data can
    be retrieved using operator[].  All data is tranferred from the SQL backend using QVariant (see
    QVariant.

    For example:

    \code
    QSqlDatabase* myDatabase;
    ...
    QSql mySql = myDatabase->query( "select name from customer;" );
    while ( mySql->next() ) {
	QString name = mySql[0];
	DoSomething( name );
    }
    \endcode

    \sa QSqlDatabase QSqlResult QSqlRowset QVariant
*/

/*! \fn QSql::QSql( QSqlResult * r )
    Creates a QSql object which uses QSqlResult to communicate with a database.  QSql is implicitly
    shared.
*/

/*! Destroys the object and frees any allocated resources.

*/

QSql::~QSql()
{
    if (d->deref()) {
	delete d;
    }
}

/*!
    Copy constructor.
*/

QSql::QSql( const QSql& other )
    : d(other.d)
{
    d->ref();
}

QSql::QSql( QSqlResult * r )
{
    d = new QSqlResultShared( r );
}

/*!

  Creates a QSql object using the SQL query \a query and which uses
  database \a databaseName.

*/
QSql::QSql( const QString& query, const QString& databaseName)
{
    d = new QSqlResultShared( 0 );
    *this = ( QSqlConnection::database( databaseName )->driver()->createResult() );
    if ( !query.isNull() )
	setQuery( query );
}

/*!
    Assigns \a other.
*/

QSql& QSql::operator=( const QSql& other )
{
    other.d->ref();
    deref();
    d = other.d;
    return *this;
}


/*!  Returns TRUE if field \a field is NULL, otherwise returns FALSE.  The result
     must be active and valid before calling this method.  In addition, for some drivers,
     isNull() will not return accurate information until after an attempt is made to retrieve
     data (see operator[]).

     \sa isActive() isValid() operator[]

*/

bool QSql::isNull( int field ) const
{
    if ( d->sqlResult->isActive() && d->sqlResult->isValid() )
	return d->sqlResult->isNull( field );
    return FALSE;
}

/*! Applies the SQL \a query.  The \a query string must use SQL syntax appropriate for the SQL database
    being queried.

    The QSql object is reset to an invalid state, and must be positioned to
    a valid record before data values can be retrieved.

    If this operation fails, the QSql object is reset to an inactive state.

    \sa isActive(), isValid(), next(), previous(), first(), last()

*/

bool QSql::setQuery ( const QString& query )
{
    if ( d->count > 1 )
	*this = driver()->createResult();
    d->sqlResult->setActive( FALSE );
    d->sqlResult->setAt( QSqlResult::BeforeFirst );
    d->sqlResult->setQuery( query.stripWhiteSpace() );
    if ( !driver()->isOpen() || driver()->isOpenError() )
	return FALSE;
    if ( query.isNull() || query.length() == 0 )
	return FALSE;
    qDebug( "\n### SQL: " + query ); // ###
    return d->sqlResult->reset( query );
}

/*! Returns the value of field \a i (zero based) in the result as reported by the database, or
    QVariant() if it cannot be determined.
    Note that the result must be active and positioned on a valid record.

    \sa previous(), next(), first(), last(), seek(), isActive(), isValid()

*/

QVariant QSql::value( int i )
{
    if ( isActive() && isValid() && ( i > QSqlResult::BeforeFirst ) ) {
	return d->sqlResult->data( i );
    }
    return QVariant();
}

/*! Retrieves current index of the result.  If the index is invalid, a
    QSqlResult::Location will be returned indicating the position.

    \sa isValid()

*/

int QSql::at() const
{
    return d->sqlResult->at();
}


/*! Returns the current query used, or QString::null if there is no current query.

*/

QString QSql::query() const
{
    return d->sqlResult->query();
}

/*! Returns the database driver associated with the result.

*/

const QSqlDriver* QSql::driver() const
{
    return d->sqlResult->driver();
}

/*! Positions the result to a random index \a i.  If \a relative is TRUE,
    the index is moved relative to the current index.  If successful,
    TRUE is returned. If the record is not available, FALSE is returned.

    If \a relative==FALSE, the following rules apply:
    If \a i is negative, the result is positioned before the first record and FALSE is returned.
    Otherwise, an attempt is made to move to the record at index \a i.

    If \a relative==TRUE, the following rules apply:
    If the result is currently located before the first record or on the first
    record, and \a i is  negative, there is no change, and FALSE is returned.
    If the result is currently located after the last record, and \a i is
    positive, there is no change, and FALSE is returned.
    If the result is currently located somewhere in the middle, and the relative
    offset \a i moves the result below zero, the result is positioned before the
    first record and FALSE is returned.
    Otherwise, an attempt is made to move the result.  If the attempt fails, the
    result is positioned after the last record, and FALSE is returned.

*/
bool QSql::seek( int i, bool relative )
{
    preSeek();
    checkDetach();
    if ( isActive() ) {
        int actualIdx;
	if ( !relative ) { // random seek
	    if ( i < 0 ) {
		d->sqlResult->setAt( QSqlResult::BeforeFirst );
		postSeek();
		return FALSE;
	    }
	    actualIdx = i;
	}
	else {
	    switch ( at() ) { // relative seek
	    	case QSqlResult::BeforeFirst:
		    if ( i > 0 )
		    	actualIdx = i;
		    else {
			postSeek();
			return FALSE;
		    }
		    break;
		case QSqlResult::AfterLast:
		    if ( i < 0 )
		    	actualIdx = i;
		    else {
			postSeek();
			return FALSE;
		    }
		    break;
		default:
		    if ( ( at() + i ) < 0  ) {
		    	d->sqlResult->setAt( QSqlResult::BeforeFirst );
			postSeek();
			return FALSE;
		    }
		    actualIdx = i;
		    break;
	    }
	}
	// let drivers optimize
	if ( actualIdx == ( at() + 1 ) ) {
	    if ( !d->sqlResult->fetchNext() ) {
	    	d->sqlResult->setAt( QSqlResult::AfterLast );
		postSeek();
		return FALSE;
	    }
	}
	if ( actualIdx == ( at() - 1 ) ) {
	    if ( !d->sqlResult->fetchPrevious() ) {
	    	d->sqlResult->setAt( QSqlResult::BeforeFirst );
		postSeek();
		return FALSE;
	    }
	}
	if ( !d->sqlResult->fetch( actualIdx ) ) {
	    d->sqlResult->setAt( QSqlResult::AfterLast );
	    postSeek();
	    return FALSE;
	}
	postSeek();
	return TRUE;
    } else {
	postSeek();
	return FALSE;
    }
}

/*! Retrieves the next record in the result, if available.  Note that the result must be in an active
    state before calling this method.  The following rules apply:

    If the result is currently located before the first record, an attempt is made to get the first record.
    If the result is currently located after the last record, there is no change and FALSE is returned.
    If the result is located somewhere in the middle, an attempt is made to get the next record.

    In any case, if the record could not be retrieved, the result is positioned after the last record
    and FALSE is returned. If the record is successfully retrieved, TRUE is returned.

    \sa at()

*/

bool QSql::next()
{
    preSeek();
    checkDetach();
    bool b = FALSE;
    if ( isActive() ) {
	switch ( at() ) {
	    case QSqlResult::BeforeFirst:
		b = d->sqlResult->fetchFirst();
		postSeek();
	    	return b;
	    case QSqlResult::AfterLast:
		postSeek();
		return FALSE;
	    default:
		if ( !d->sqlResult->fetchNext() ) {
		    d->sqlResult->setAt( QSqlResult::AfterLast );
		    postSeek();
		    return FALSE;
		}
		postSeek();
        	return TRUE;
	}
    }
    postSeek();
    return FALSE;
}

/*! Positions the result to the previous record in the result, if available.  Note that the result must
    be in an active state before calling this method.  The following rules apply:

    If the result is currently located before the first record, there is no change and FALSE is returned.
    If the result is currently located after the last record, an attempt is made to get the last record.
    If the result is somewhere in the middle, an attempt is made to get the previous record.

    In any case, is the record could not be retrieved, the result is positioned before the first record
    and FALSE is returned.  If the record is successfully retrieved, TRUE is returned.

    \sa at()

*/

bool QSql::previous()
{
    preSeek();
    checkDetach();
    bool b = FALSE;
    if ( isActive() ) {
	switch ( at() ) {
	    case QSqlResult::BeforeFirst:
		postSeek();
		return FALSE;
	    case QSqlResult::AfterLast:
		b = d->sqlResult->fetchLast();
		postSeek();
		return b;
	    default:
		if ( !d->sqlResult->fetchPrevious() ) {
		    d->sqlResult->setAt( QSqlResult::BeforeFirst );
		    postSeek();
		    return FALSE;
		}
		postSeek();
        	return TRUE;
	}
    }
    postSeek();
    return FALSE;
}

/*! Positions the result to the first record of an active result.  Returns TRUE on success, FALSE otherwise.
    Note that the result must be in an active state before calling this method.

*/

bool QSql::first()
{
    preSeek();
    checkDetach();
    bool b = FALSE;
    if ( isActive() ) {
	b = d->sqlResult->fetchFirst();
	postSeek();
	return b;
    }
    postSeek();
    return FALSE;
}

/*! Positions the result to the last record of an active result.  Returns TRUE on success, FALSE otherwise.
    Note that the result must be in an active state before calling this method.

*/

bool QSql::last()
{
    preSeek();
    checkDetach();
    bool b = FALSE;
    if ( isActive() ) {
	b = d->sqlResult->fetchLast();
	postSeek();
	return b;
    }
    return FALSE;
}

/*!
  Returns a list of fields used in the query.  Note that the fields which are
  returned may not contain complete QSqlField information.  For example:

  \code
  QSql q = database->query("select name, salary+bonus from employeee;");
  ...
  QSqlFieldList fl = q.fields();

  \endcode

  \sa QSqlField

*/

QSqlFieldList QSql::fields() const
{
    return d->sqlResult->fields();
}

/*!  Returns the size of the result, or -1 if it cannot be determined
  or the database does not support reporting information about query
  sizes.  Note that for non-SELECT statements, size() will return -1.
  To determine the number of rows affected by a non-SELECT statement,
  use affectedRows().

  \sa affectedRows() QSqlDatabase

*/
int QSql::size() const
{
    if ( d->sqlResult->driver()->hasQuerySizeSupport() )
	return d->sqlResult->size();
    return -1;
}

/*!
  Returns the number of rows affected by the result's SQL statement, or
  -1 if it cannot be determined.  Note that for SELECT statements, this
  value will be the same as size(),

  \sa size()

*/

int QSql::affectedRows() const
{
    return d->sqlResult->affectedRows();
}

/*!
  Returns a QSqlError object which contains information about the last error (if any) that occurred.

  \sa QSqlError

*/

QSqlError QSql::lastError() const
{
    return d->sqlResult->lastError();
}

bool QSql::isValid() const
{
    return d->sqlResult->isValid();
}

bool QSql::isActive() const
{
    return d->sqlResult->isActive();
}

/*!
  Returns TRUE is the current query is a SQL SELECT statement, otherwise false.

*/

bool QSql::isSelect() const
{
    return d->sqlResult->isSelect();
}

/*!
  \internal
*/

void QSql::deref()
{
    if ( d->deref() ) {
	delete d;
	d = 0;
    }
}

/*!
  \internal
*/

bool QSql::checkDetach()
{
    if ( d->count > 1 ) {
	QString sql = d->sqlResult->query();
	*this = driver()->createResult();
	setQuery( sql );
	return TRUE;
    }
    return FALSE;
}


/*!  Protected virtual called before the internal record pointer is
  moved.  The default implentation does nothing.

*/

void QSql::preSeek()
{

}


/*!  Protected virtual called after the internal record pointer is
  moved.  The default implentation does nothing.
*/

void QSql::postSeek()
{

}


#endif // QT_NO_SQL


