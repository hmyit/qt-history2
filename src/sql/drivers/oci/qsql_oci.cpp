/****************************************************************************
**
** Implementation of OCI driver classes.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the sql module of the Qt GUI Toolkit.
** EDITIONS: ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qsql_oci.h"

#include <qdatetime.h>
#include <qptrvector.h>
#include <qmemarray.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <private/qinternal_p.h>
#include <stdlib.h>

#define QOCI_DYNAMIC_CHUNK_SIZE  255
static const ub2 CSID_UTF8 = 871; // UTF-8 not defined in Oracle 8 libraries
static const ub1 CSID_NCHAR = SQLCS_NCHAR;

#ifdef OCI_UTF16ID
static const ub2 CSID_UTF16 = OCI_UTF16ID;
#else
static const ub2 CSID_UTF16 = 0;
#endif

QByteArray qMakeOraDate( const QDateTime& dt );
QDateTime qMakeDate( const char* oraDate );
QString qOraWarn( const QOCIPrivate* d );
void qOraWarning( const char* msg, const QOCIPrivate* d );

class QOCIPrivate
{
public:
    QOCIPrivate();
    ~QOCIPrivate();

    OCIEnv *env;
    OCIError *err;
    OCISvcCtx *svc;
    OCIStmt *sql;
    bool transaction;
    int serverVersion;
    bool utf8;  // db charset
    bool nutf8; // national charset
    bool utf16bind;
    QString user;

    typedef QVector<QVariant> RowCache;
    typedef QVector<RowCache*> RowsetCache;
    RowsetCache rowCache;
    void clearCache();
    
    text* oraText( const QString& str ) const;    
    sb4 oraTextLength( const QString& str ) const;    
    sb4 oraByteLength( const QString& str ) const;
    void setCharset( OCIBind* hbnd );    
    int bindValues( QVector<QVariant>& values, QList<QVirtualDestructor*> & tmpStorage );    
    void outValues( QVector<QVariant> &values, QList<QVirtualDestructor*> & tmpStorage );
};

QOCIPrivate::QOCIPrivate(): env(0), err(0), svc(0), sql(0), transaction( FALSE ), serverVersion(-1),
    utf8( FALSE ), nutf8( FALSE ), utf16bind( FALSE )
{
}

QOCIPrivate::~QOCIPrivate()
{
    clearCache();
}

void QOCIPrivate::clearCache()
{
    for (int i = 0; i < rowCache.count(); ++i)
	delete rowCache.at(i);
    rowCache.clear();
}

text* QOCIPrivate::oraText( const QString& str ) const
{
    if ( utf16bind )
	return (text*)str.ucs2();
    return (text*)str.ascii();
}

sb4 QOCIPrivate::oraTextLength( const QString& str ) const
{
    if ( utf16bind )
	return (sb4)((str.length() + 1) * sizeof( QChar ));
    return (sb4)(str.length() + 1);
}

sb4 QOCIPrivate::oraByteLength( const QString& str ) const
{
    if ( utf16bind )
	return (sb4)((str.length()) * sizeof( QChar ));
    return (sb4)str.length();
}

void QOCIPrivate::setCharset( OCIBind* hbnd )
{
    int r = 0;
    
#ifdef QOCI_USES_VERSION_9
    if ( serverVersion > 8 && !CSID_UTF16 ) {
	
	r = OCIAttrSet( (void*)hbnd,
			OCI_HTYPE_BIND,
			(void*) &CSID_NCHAR,
			(ub4) 0,
			(ub4) OCI_ATTR_CHARSET_FORM,
			err );
	
#ifdef QT_CHECK_RANGE
	if ( r != 0 )
	    qOraWarning( "QOCIPrivate::setCharset: Couldn't set OCI_ATTR_CHARSET_FORM: ", this );
#endif
    }
#endif //QOCI_USES_VERSION_9
    
    const ub2* csid = utf16bind ? &CSID_UTF16 : &CSID_UTF8;
    r = OCIAttrSet( (void*)hbnd,
		    OCI_HTYPE_BIND,
		    (void*) &csid,
		    (ub4) 0,
		    (ub4) OCI_ATTR_CHARSET_ID,
		    err );
#ifdef QT_CHECK_RANGE
    if ( r != 0 )
	qOraWarning( "QOCIPrivate::setCharset: Couldn't set OCI_ATTR_CHARSET_ID: ", this );
#endif
}

int QOCIPrivate::bindValues( QVector<QVariant>& values, QList<QVirtualDestructor*> & tmpStorage )
{
    int r = OCI_SUCCESS;	
    int i;
    for ( i = 0; i < values.count(); ++i ) {
	QVariant val( values.at( i ) );
	//qDebug( "binding values: %d, %s", i, values.at(i).toString().ascii() );
	OCIBind * hbnd = 0; // Oracle handles these automatically
	sb2 * indPtr = new sb2(0);
	tmpStorage.append( qAutoDeleter(indPtr) );
	if ( val.isNull() )
	    *indPtr = -1;
	//	    qDebug( "Binding: type: %s utf16: %d holder: %i value: %s", QVariant::typeToName( val.type() ), utf16bind, i, val.toString().ascii() );
	switch ( val.type() ) {
	    case QVariant::ByteArray:
		r = OCIBindByPos( sql, &hbnd, err,
				  i + 1,
				  (dvoid *) val.asByteArray().data(),
				  val.asByteArray().size(),
				  SQLT_BIN, (dvoid *) indPtr, (ub2 *) 0, (ub2*) 0,
				  (ub4) 0, (ub4 *) 0, OCI_DEFAULT );
	    break;
	    case QVariant::Time:
		case QVariant::Date:
		case QVariant::DateTime: {
		QByteArray * ba = new QByteArray( qMakeOraDate( val.toDateTime() ) );
		tmpStorage.append( qAutoDeleter(ba) );
		r = OCIBindByPos( sql, &hbnd, err,
				  i + 1,
				  (dvoid *) ba->data(),
				  ba->size(),
				  SQLT_DAT, (dvoid *) indPtr, (ub2 *) 0, (ub2*) 0,
				  (ub4) 0, (ub4 *) 0, OCI_DEFAULT );
		break; }
	    case QVariant::Int:
		r = OCIBindByPos( sql, &hbnd, err,
				  i + 1,
				  (dvoid *) &values[ i ].asInt(), // avoid deep cpy
				  sizeof(int),
				  SQLT_INT, (dvoid *) indPtr, (ub2 *) 0, (ub2*) 0,
				  (ub4) 0, (ub4 *) 0, OCI_DEFAULT );
	    break;
	    case QVariant::Double:
		r = OCIBindByPos( sql, &hbnd, err,
				  i + 1,
				  (dvoid *) &values[ i ].asDouble(), // avoid deep cpy
				  sizeof(double),
				  SQLT_FLT, (dvoid *) indPtr, (ub2 *) 0, (ub2*) 0,
				  (ub4) 0, (ub4 *) 0, OCI_DEFAULT );
	    break;
	    default: {
		QString* str = new QString( val.toString() );
		tmpStorage.append( qAutoDeleter(str) );
		r = OCIBindByPos( sql, &hbnd, err,
				  i + 1,
				  (dvoid *)str->ucs2(),
				  (str->length() + 1) * sizeof(QChar), // number of bytes + 0 term. scan limit
				  SQLT_STR, (dvoid *) indPtr, (ub2 *) 0, (ub2*) 0,
				  (ub4) 0, (ub4 *) 0, OCI_DEFAULT );
		setCharset( hbnd );
		break; }
	}
	if (r != OCI_SUCCESS)
	    qOraWarning( "QOCIPrivate::bindValues:", this);
    }
    return r;
}

void QOCIPrivate::outValues( QVector<QVariant> &values, QList<QVirtualDestructor*> & tmpStorage )
{
    int i;
    for ( i = 0; i < values.count(); ++i ) {
	QVariant::Type typ = values[i].type();
	sb2 *indPtr;
	if (tmpStorage.isEmpty() || !(indPtr = qAutoDeleterData((QAutoDeleter<sb2>*)tmpStorage.at(0))) )
	    return;
	bool isNull = (*indPtr == -1);
	tmpStorage.removeFirst();
	
	if ( isNull ) {
	    values[ i ] = QVariant(typ);
	    if ( !tmpStorage.isEmpty() && typ != QVariant::ByteArray && typ != QVariant::Int && typ != QVariant::Double )
		tmpStorage.removeFirst();
	    continue;
	}	    
	
	if (tmpStorage.isEmpty())
	    return;
    
	switch ( typ ) {
	    case QVariant::ByteArray:
	    case QVariant::Int:
	    case QVariant::Double:
		break;
	    case QVariant::Time:
		case QVariant::Date:
		case QVariant::DateTime: {
		    QByteArray *ba = qAutoDeleterData((QAutoDeleter<QByteArray>*)tmpStorage.at(0));
		    QDateTime dt = qMakeDate( ba->data() );
		    if ( typ == QVariant::DateTime )
			values[ i ] = dt;
		    else if ( typ == QVariant::Date )
			values[ i ] = dt.date();
		    else if ( typ == QVariant::Time )
			values[ i ] = dt.time();
		    tmpStorage.removeFirst();
		}
	    break;
	    default: {
		QString *str = qAutoDeleterData((QAutoDeleter<QString>*)tmpStorage.at(0));
		if (!str)
		    return;
		//		    qDebug( "received: %d, '%s'", str->length(), QString::fromUcs2( str->ucs2() ).ascii() );
		values[ i ] = QString::fromUcs2( str->ucs2() );
		tmpStorage.removeFirst();
		break; }
	}			
    }
}

struct OraFieldInfo
{
    QString	   name;
    QVariant::Type type;
    ub1		   oraIsNull;
    ub4		   oraType;
    sb1		   oraScale;
    ub4		   oraLength; // size in bytes
    ub4		   oraFieldLength; // amount of characters
    sb2		   oraPrecision;
};

QString qOraWarn( const QOCIPrivate* d )
{
    sb4 errcode;
    text errbuf[1024];
    
    QString errBuf;
    errBuf.setLength( 512 );
    OCIErrorGet( (dvoid *)d->err,
		(ub4) 1,
		(text *)NULL,
		&errcode,
		errbuf,
		(ub4)( sizeof(errbuf) ),
		OCI_HTYPE_ERROR);
    if ( d->utf16bind )
	return QString::fromUcs2( (const unsigned short*)errbuf );
    return QString::fromLocal8Bit( (const char*)errbuf );
}

void qOraWarning( const char* msg, const QOCIPrivate* d )
{
    unsigned char   errbuf[512];
    sb4             errcode;
    OCIErrorGet((dvoid *)d->err,
                (ub4) 1,
                (text *) NULL,
                &errcode,
                errbuf,
                (ub4) sizeof(errbuf),
                OCI_HTYPE_ERROR);
    qWarning( "%s %s", msg, errbuf );
}

int qOraErrorNumber( const QOCIPrivate* d )
{
    sb4 errcode;
    OCIErrorGet((dvoid *)d->err,
		(ub4) 1,
		(text *) NULL,
		&errcode,
		NULL,
		0,
		OCI_HTYPE_ERROR);
    return errcode;
}

QSqlError qMakeError( const QString& err, int type, const QOCIPrivate* p )
{
    return QSqlError("QOCI: " + err, qOraWarn(p), type );
}

QVariant::Type qDecodeOCIType( const QString& ocitype, int ocilen, int ociprec, int ociscale )
{
    QVariant::Type type = QVariant::Invalid;
    if ( ocitype == "VARCHAR2" || ocitype == "VARCHAR" || ocitype.startsWith( "INTERVAL" ) ||
	 ocitype == "CHAR" || ocitype == "NVARCHAR2" || ocitype == "NCHAR" )
	type = QVariant::String;
    else if ( ocitype == "NUMBER" )
	type = QVariant::Int;
    else if ( ocitype == "FLOAT" )
	type = QVariant::Double;
//    else if ( ocitype == "LONG" || ocitype == "NCLOB" || ocitype == "CLOB" )
//	type = QVariant::CString;
    else if ( ocitype == "RAW" || ocitype == "LONG RAW" || ocitype == "ROWID" || ocitype == "CFILE" || ocitype == "BFILE" || ocitype == "BLOB" )
	type = QVariant::ByteArray;
    else if ( ocitype == "DATE" ||  ocitype.startsWith( "TIME" ) )
	type = QVariant::DateTime;
    else if ( ocitype == "UNDEFINED" )
	type = QVariant::Invalid;
    if ( type == QVariant::Int ) {
	if ( ocilen == 22 && ociprec == 0 && ociscale == 0 )
	    type = QVariant::Double;
	if ( ociscale > 0 )
	    type = QVariant::Double;
    }
    if ( type == QVariant::Invalid )
	qWarning("qDecodeOCIType: unknown type: %s", ocitype.local8Bit() );
    return type;
}

QVariant::Type qDecodeOCIType( int ocitype )
{
    QVariant::Type type = QVariant::Invalid;
    switch ( ocitype ) {
    case SQLT_STR:
    case SQLT_VST:
    case SQLT_CHR:
    case SQLT_AFC:
    case SQLT_VCS:
    case SQLT_AVC:
    case SQLT_LNG: //???
#ifdef SQLT_INTERVAL_YM
    case SQLT_INTERVAL_YM:
#endif
#ifdef SQLT_INTERVAL_DS
    case SQLT_INTERVAL_DS:
#endif
	type = QVariant::String;
	break;
    case SQLT_INT:
	type = QVariant::Int;
	break;
    case SQLT_FLT:
    case SQLT_NUM:
    case SQLT_VNU:
    case SQLT_UIN:
	type = QVariant::Double;
	break;
    case SQLT_CLOB:
//    case SQLT_LNG:
//	type = QVariant::CString;
//	break;
    case SQLT_VBI:
    case SQLT_BIN:
    case SQLT_LBI:
    case SQLT_LVC:
    case SQLT_LVB:
    case SQLT_BLOB:
    case SQLT_FILE:
    case SQLT_RDD:
    case SQLT_NTY:
    case SQLT_REF:
    case SQLT_RID:
	type = QVariant::ByteArray;
	break;
    case SQLT_DAT:
    case SQLT_ODT:
#ifdef SQLT_TIMESTAMP
    case SQLT_TIMESTAMP:
    case SQLT_TIMESTAMP_TZ:
    case SQLT_TIMESTAMP_LTZ:
#endif
	type = QVariant::DateTime;
	break;
    default:
	type = QVariant::Invalid;
	qWarning( "qDecodeOCIType: unknown OCI datatype: %d", ocitype );
	break;
    }
	return type;
}

OraFieldInfo qMakeOraField( const QOCIPrivate* p, OCIParam* param )
{
    OraFieldInfo ofi;
    ub2		colType(0);
    text		*colName = 0;
    ub4		colNameLen(0);
    sb1		colScale(0);
    ub2		colLength(0);
    ub2		colFieldLength(0);
    sb2		colPrecision(0);
    ub1		colIsNull(0);
    int		r(0);
    QVariant::Type type( QVariant::Invalid );

    r = OCIAttrGet( (dvoid*)param,
		    OCI_DTYPE_PARAM,
		    &colType,
		    0,
		    OCI_ATTR_DATA_TYPE,
		    p->err);
    if ( r != 0 )
	qOraWarning( "qMakeOraField:", p );

    r = OCIAttrGet( (dvoid*) param,
		    OCI_DTYPE_PARAM,
		    (dvoid**) &colName,
		    (ub4 *) &colNameLen,
		    (ub4) OCI_ATTR_NAME,
		    p->err );
    if ( r != 0 )
	qOraWarning( "qMakeOraField:", p );

    r = OCIAttrGet( (dvoid*) param,
    OCI_DTYPE_PARAM,
		    &colLength,
		    0,
		    OCI_ATTR_DATA_SIZE, /* in bytes */
		    p->err );
    if ( r != 0 )
	qOraWarning( "qMakeOraField:", p );

#ifdef OCI_ATTR_CHAR_SIZE
    r = OCIAttrGet( (dvoid*) param,
		    OCI_DTYPE_PARAM,
		    &colFieldLength,
		    0,
		    OCI_ATTR_CHAR_SIZE,
		    p->err );
    if ( r != 0 )
	qOraWarning( "qMakeOraField:", p );
#else
    // for Oracle8.
    colFieldLength = colLength;
#endif

    r = OCIAttrGet( (dvoid*) param,
		    OCI_DTYPE_PARAM,
		    &colPrecision,
		    0,
		    OCI_ATTR_PRECISION,
		    p->err );
    if ( r != 0 )
	qOraWarning( "qMakeOraField:", p );

    r = OCIAttrGet( (dvoid*) param,
		    OCI_DTYPE_PARAM,
		    &colScale,
		    0,
		    OCI_ATTR_SCALE,
		    p->err );
    if ( r != 0 )
	qOraWarning( "qMakeOraField:", p );
    r = OCIAttrGet( (dvoid*)param,
		    OCI_DTYPE_PARAM,
		    (dvoid*)&colType,
		    0,
		    OCI_ATTR_DATA_TYPE,
		    p->err);
    if ( r != 0 )
	qOraWarning( "qMakeOraField:", p );
    r = OCIAttrGet( (dvoid*)param,
		    OCI_DTYPE_PARAM,
		    (dvoid*)&colIsNull,
		    0,
		    OCI_ATTR_IS_NULL,
		    p->err);
    if ( r != 0 )
	qOraWarning( "qMakeOraField:", p );

    type = qDecodeOCIType( colType );
    if ( type == QVariant::Int ) {
	if ( colLength == 22 && colPrecision == 0 && colScale == 0 )
	    type = QVariant::Double;
	if ( colScale > 0 )
	    type = QVariant::Double;
    }
    if ( colType == SQLT_BLOB )
	colLength = 0;

    // colNameLen is length in bytes!!!!
    ofi.name = QString( (const QChar*)colName, colNameLen / 2 );
    ofi.type = type;
    ofi.oraType = colType;
    ofi.oraFieldLength = colFieldLength;
    ofi.oraLength = colLength;
    ofi.oraScale = colScale;
    ofi.oraPrecision = colPrecision;
    ofi.oraIsNull = colIsNull;

    return ofi;
}


/*! \internal Convert QDateTime to the internal Oracle DATE format 
  NB! It does not handle BCE dates.
*/
QByteArray qMakeOraDate( const QDateTime& dt )
{
    QByteArray ba( 7 );
    int year = dt.date().year();
    ba[0]= (year / 100) + 100; // century
    ba[1]= (year % 100) + 100; // year
    ba[2]= dt.date().month();
    ba[3]= dt.date().day();
    ba[4]= dt.time().hour() + 1;
    ba[5]= dt.time().minute() + 1;
    ba[6]= dt.time().second() + 1;
    return ba;
}

QDateTime qMakeDate( const char* oraDate )
{
    int century = oraDate[0];
    if( century >= 100 ){
	int year    = (unsigned char)oraDate[1];
	year = ( (century-100)*100 ) + (year-100);
	int month = oraDate[2];
	int day   = oraDate[3];
	int hour  = oraDate[4] - 1;
	int min   = oraDate[5] - 1;
	int sec   = oraDate[6] - 1;
	return QDateTime( QDate(year,month,day), QTime(hour,min,sec) );
    }
    return QDateTime();
}

class QOCIResultPrivate
{
public:    
    QOCIResultPrivate( int size, QOCIPrivate* dp );
    ~QOCIResultPrivate();
    void setCharset( OCIDefine* dfn );
    int readPiecewise( QSqlRecord& res );
    int readLOBs( QSqlRecord& res );
    void getOraFields( QSqlRecordInfo &rinf );
    char* at( int i );
    int size();
    bool isNull( int i );
    QVariant::Type type( int i );
    int fieldFromDefine( OCIDefine* d );
    OCILobLocator* lobLocator( int i );
    int length( int i );
    QVariant value( int i );
    
    QSqlRecord fs;
    
private:
    char* create( int position, int size );
    OCILobLocator** createLobLocator( int position, OCIEnv* env );

    class OraFieldInf
    {
    public:
	OraFieldInf(): data(0), len(0), ind(0), typ(QVariant::Invalid), def(0), lob(0) {}
	~OraFieldInf();
	char *data;
	int len;
	sb2 ind;
	QVariant::Type typ;
	OCIDefine *def;
	OCILobLocator **lob;
    };
    
    QVector<OraFieldInf> fieldInf;    
    QOCIPrivate* d;
};

QOCIResultPrivate::OraFieldInf::~OraFieldInf()
{
    delete [] data;
    if (lob) {
	int r = OCIDescriptorFree( (dvoid *)*lob, (ub4) OCI_DTYPE_LOB );
	if ( r != 0 )
	    qWarning( "QOCIResultPrivate: Cannot free LOB descriptor" );
    }
}

QOCIResultPrivate::QOCIResultPrivate( int size, QOCIPrivate* dp )
    : fieldInf(size), d(dp)
//    : data( size ), len( size ), ind( size ), typ( size ), def( size ), lobs( size ), d( dp )
{   
    ub4 dataSize(0);
    OCIDefine* dfn = 0;
    int r;
    
    OCIParam* param = 0;
    sb4 parmStatus = 0;
    ub4 count = 1;
    int idx = 0;
    parmStatus = OCIParamGet( d->sql,
			      OCI_HTYPE_STMT,
			      d->err,
			      (void**)&param,
			      count );
    
    while ( parmStatus == OCI_SUCCESS ) {
	OraFieldInfo ofi = qMakeOraField( d, param );
#ifdef SQLT_INTERVAL_YM
#ifdef SQLT_INTERVAL_DS
	if ( ofi.oraType == SQLT_INTERVAL_YM || ofi.oraType == SQLT_INTERVAL_DS )
	    // since we are binding interval datatype as string,
	    // we are not interested in the number of bytes but characters.
	    dataSize = 50;  // magic number
	else
#endif //SQLT_INTERVAL_DS
#endif //SQLT_INTERVAL_YM
	    dataSize = ofi.oraLength;
	QVariant::Type type = ofi.type;
	fieldInf[count-1].typ = type;
	switch ( type ) {
	    case QVariant::DateTime:
		r = OCIDefineByPos( d->sql,
				    &dfn,
				    d->err,
				    count,
				    create(idx, dataSize+1),
				    dataSize+1,
				    SQLT_DAT,
				    (dvoid *) &(fieldInf[idx].ind),
				    0, 0, OCI_DEFAULT );
	    break;
	    /*
	    case QVariant::CString:
		// LONG fields can't be bound to LOB locators
		if ( ofi.oraType == SQLT_LNG ) {
		    r = OCIDefineByPos( d->sql,
					&dfn,
					d->err,
					count,
					0,
					SB4MAXVAL, // really big
					SQLT_LNG,
					(dvoid *) createInd( count-1 ),
					(ub2 *) 0,
					(ub2 *) 0,
					OCI_DYNAMIC_FETCH ); // piecewise
		} else {
		    r = OCIDefineByPos( d->sql,
					&dfn,
					d->err,
					count,
					createLobLocator( count-1, d->env ),
					(sb4) -1,
					ofi.oraType,
					(dvoid *) createInd( count-1 ),
					(ub2 *) 0,
					(ub2 *) 0,
					OCI_DEFAULT ); // piecewise 
		    if ( r == 0 )
			setCharset( dfn );
		}
		break;
*/
	    case QVariant::ByteArray:
		// RAW and LONG RAW fields can't be bound to LOB locators
		if ( ofi.oraType == SQLT_BIN ) {
		//		    qDebug( "binding SQLT_BIN" );
		r = OCIDefineByPos( d->sql,
				    &dfn,
				    d->err,
				    count,
				    create(idx, dataSize),
				    dataSize,
				    SQLT_BIN,
				    (dvoid *) &(fieldInf[idx].ind),
				    0, 0, OCI_DYNAMIC_FETCH );
	    } else if ( ofi.oraType == SQLT_LBI ) {
		//		    qDebug( "binding SQLT_LBI" );
		r = OCIDefineByPos( d->sql,
				    &dfn,
				    d->err,
				    count,
				    0,
				    SB4MAXVAL,
				    SQLT_LBI,
				    (dvoid *) &(fieldInf[idx].ind),
				    0, 0, OCI_DYNAMIC_FETCH );
	    } else {
		//		    qDebug( "binding SQLT_BLOB" );
		r = OCIDefineByPos( d->sql,
				    &dfn,
				    d->err,
				    count,
				    createLobLocator(idx, d->env),
				    (sb4)-1,
				    SQLT_BLOB,
				    (dvoid *) &(fieldInf[idx].ind),
				    0, 0, OCI_DEFAULT );
	    }
	    break;
	    case QVariant::String:
		dataSize += dataSize + sizeof( QChar );
	    //qDebug( "OCIDefineByPosStr: %d", dataSize );
	    r = OCIDefineByPos( d->sql,
				&dfn,
				d->err,
				count,
				create(idx, dataSize),
				dataSize,
				SQLT_STR,
				(dvoid *) &(fieldInf[idx].ind),
				0, 0, OCI_DEFAULT );
	    if ( r == 0 )
		setCharset( dfn );
	    break;
	    default:
	    dataSize += ++dataSize; // REMOVE ME
	    //qDebug( "OCIDefineByPosDef: %d", dataSize );
	    r = OCIDefineByPos( d->sql,
				&dfn,
				d->err,
				count,
				create(idx, dataSize+1),
				dataSize+1,
				SQLT_STR,
				(dvoid *) &(fieldInf[idx].ind),
				0, 0, OCI_DEFAULT );
	    break;
	}
	if ( r != 0 )
	    qOraWarning( "QOCIResultPrivate::bind:", d );
	fieldInf[idx].def = dfn;
	++count;
	++idx;
	parmStatus = OCIParamGet( d->sql,
				  OCI_HTYPE_STMT,
				  d->err,
				  (void**)&param,
				  count );
    }
}

QOCIResultPrivate::~QOCIResultPrivate()
{
}

char* QOCIResultPrivate::create( int position, int size )
{
    char* c = new char[ size+1 ];
    // Oracle may not fill fixed width fields
    memset( c, 0, size+1 );
    fieldInf[position].data = c;
    fieldInf[position].len = size;
    return c;
}

OCILobLocator** QOCIResultPrivate::createLobLocator( int position, OCIEnv* env )
{
    OCILobLocator** lob = new OCILobLocator*;
    int r = OCIDescriptorAlloc( (dvoid *)env,
				(dvoid **)lob,
				(ub4)OCI_DTYPE_LOB,
				(size_t) 0,
				(dvoid **) 0 );
    if ( r != 0 )
	qWarning( "QOCIResultPrivate: Cannot create LOB locator" );
    fieldInf[position].lob = lob;
    return lob;
}

void QOCIResultPrivate::setCharset( OCIDefine* dfn )
{
    int r = 0;
    
#ifdef QOCI_USES_VERSION_9
    if ( d->serverVersion > 8 && !CSID_UTF16 ) {
	
	r = OCIAttrSet( (void*)dfn,
			OCI_HTYPE_DEFINE,
			(void*) &CSID_NCHAR,
			(ub4) 0,
			(ub4) OCI_ATTR_CHARSET_FORM,
			d->err );
	
	if ( r != 0 )
	    qOraWarning( "QOCIResultPrivate::setCharset: Couldn't set OCI_ATTR_CHARSET_FORM: ", d );
    }
#endif //QOCI_USES_VERSION_9
    
    if (d->serverVersion > 8) {
	const ub2* csid = d->utf16bind ? &CSID_UTF16 : &CSID_UTF8;
	r = OCIAttrSet( (void*)dfn,
			OCI_HTYPE_DEFINE,
			(void*) &csid,
			(ub4) 0,
			(ub4) OCI_ATTR_CHARSET_ID,
			d->err );
	if ( r != 0 )
	    qOraWarning( "QOCIResultPrivate::setCharset: Couldn't set OCI_ATTR_CHARSET_ID: ", d );
    }
}

int QOCIResultPrivate::readPiecewise( QSqlRecord& res )
{
    OCIDefine*     dfn;
    ub4            typep;
    ub1            in_outp;
    ub4            iterp;
    ub4            idxp;
    ub1            piecep;
    sword          status;
    text           col [QOCI_DYNAMIC_CHUNK_SIZE+1];
    int            fieldNum = -1;
    int            r = 0;
    bool           nullField;
    for ( ; ; ) {
	r = OCIStmtGetPieceInfo( d->sql, d->err, (dvoid**) &dfn, &typep,
				 &in_outp, &iterp, &idxp, &piecep );
	if ( r != OCI_SUCCESS )
	    qOraWarning( "OCIResultPrivate::readPiecewise: unable to get piece info:", d );
	fieldNum = fieldFromDefine( dfn );
	int chunkSize = QOCI_DYNAMIC_CHUNK_SIZE;
	nullField = FALSE;
	r  = OCIStmtSetPieceInfo( dfn, OCI_HTYPE_DEFINE,
				  d->err, (void *)col,
				  (ub4 *)&chunkSize, piecep, NULL, NULL);
	if ( r != OCI_SUCCESS )
	    qOraWarning( "OCIResultPrivate::readPiecewise: unable to set piece info:", d );
	status = OCIStmtFetch (  d->sql, d->err, 1, OCI_FETCH_NEXT, OCI_DEFAULT );
	if ( status == -1 ) {
	    sb4 errcode;
	    OCIErrorGet((dvoid *)d->err, (ub4) 1, (text *) NULL, &errcode, NULL, 0, OCI_HTYPE_ERROR);
	    switch ( errcode ) {
		case 1405: /* NULL */
		nullField = TRUE;
		break;
		default:
		qOraWarning( "OCIResultPrivate::readPiecewise: unable to fetch next:", d );
		break;
	    }
	}
	if ( status == OCI_NO_DATA ) {
	    break;
	}
	if ( nullField || !chunkSize ) {
	    //		if ( res.value( fieldNum ).type() == QVariant::CString ) {
	    //		    res.setValue( fieldNum, QCString() );
	    //		} else {
	    res.setValue( fieldNum, QByteArray() );
	    //		}
	} else {
	    //		QByteArray * ba;
	    //		if ( res.value( fieldNum ).type() == QVariant::CString ) {
	    //		    ba = new QCString();
	    //		    *ba = res.value( fieldNum ).toCString();
	    //		} else {
	    //		    ba = new QByteArray();
	    //		    *ba = res.value( fieldNum ).toByteArray();
	    //		}
	    // NB! not a leak - tmp is deleted by QByteArray/QCString later on
	    //		char * tmp = (char *)malloc( chunkSize + ba->size() );
	    //		memcpy( tmp, ba->data(), ba->size() );
	    //		memcpy( tmp + ba->size(), col, chunkSize );
	    //		*ba = ba->assign( tmp, chunkSize + ba->size() );
	    
	    res.value( fieldNum ).asByteArray().append( QByteArray( (char*)col, chunkSize ) );
	    
	    //		if ( res.value( fieldNum ).type() == QVariant::CString ) {
	    //		    res.setValue( fieldNum, *((QCString *) ba) );
	    //		} else {
	    //		    res.setValue( fieldNum, *ba );
	    //		}
	    //		delete ba;		
	}
	if ( status == OCI_SUCCESS_WITH_INFO ||
	     status == OCI_NEED_DATA ) {
	} else
	    break;
    }
    return r;
}

int QOCIResultPrivate::readLOBs( QSqlRecord& res )
{
    int r = 0;
    OCILobLocator* lob;
    ub4 amount;
    for ( int i = 0; i < size(); ++i ) {
	lob = lobLocator( i );
	if ( !lob || isNull( i ) )
	    continue;
	r = OCILobGetLength( d->svc, d->err, lob, &amount );
	if ( r != 0 ) {
	    qOraWarning( "OCIResultPrivate::readLOBs: Can't get size of LOB:", d );
	    amount = 0;
	}
	if ( amount > 0 ) {
	    QByteArray * buf;
	    //		if ( res.value( i ).type() == QVariant::CString ) {
	    //		    buf = new QCString( amount + 1 ); // including terminating zero
	    //		} else {
	    buf = new QByteArray( amount );
	    //		}
	    
	    // get lob charset ID and tell oracle to transform it into UTF-8
	    ub1 csfrm = 0;
	    r = OCILobCharSetForm( d->env, d->err, lob, &csfrm );
	    if ( r != 0 ) {
		qOraWarning( "OCIResultPrivate::readLOBs: Can't get encoding of LOB: ", d );
		csfrm = 0;
	    }
	    
	    r = OCILobRead( d->svc,
			    d->err,
			    lob,
			    &amount,
			    1,
			    (void*) buf->data(),
			    (ub4) buf->size(),
			    0, 0,
			    0,
			    csfrm );
	    if ( r != 0 ) {
		qOraWarning( "OCIResultPrivate::readLOBs: Cannot read LOB:", d );
	    } else {
		//		    if ( res.value( i ).type() == QVariant::CString ) {
		//			res.setValue( i, *((QCString *) buf) );
		//		    } else {
		res.setValue( i, *buf );
		//		    }
	    }
	    delete buf;
	}
	if ( r != 0 || !amount ) {
	    //		if ( res.value( i ).type() == QVariant::CString ) {
	    //		    res.setValue( i, QCString() );
	    //		} else {
	    res.setValue( i, QByteArray() );
	    //		}
	    r = 0; // non-fatal error
	}
    }
    return r;
}

void QOCIResultPrivate::getOraFields( QSqlRecordInfo &rinf )
{
    OCIParam* param = 0;
    ub4 count = 1;
    sb4 parmStatus = OCIParamGet( d->sql,
				  OCI_HTYPE_STMT,
				  d->err,
				  (void**)&param,
				  count );
    
    while ( parmStatus == OCI_SUCCESS ) {
	OraFieldInfo ofi = qMakeOraField( d, param );
	QSqlFieldInfo inf( ofi.name, ofi.type, (int)ofi.oraIsNull == 0 ? 1 : 0, (int)ofi.oraFieldLength,
			   (int)ofi.oraPrecision, QVariant(), (int)ofi.oraType );
	rinf.append( inf );
	count++;
	parmStatus = OCIParamGet( d->sql,
				  OCI_HTYPE_STMT,
				  d->err,
				  (void**)&param,
				  count );
    }
}

inline char* QOCIResultPrivate::at( int i )
{
    return fieldInf.at(i).data;
}
inline int QOCIResultPrivate::size()
{
    return fieldInf.size();
}
inline bool QOCIResultPrivate::isNull( int i )
{
    return ( fieldInf.at( i ).ind == -1 );
}
inline QVariant::Type QOCIResultPrivate::type( int i )
{
    return fieldInf.at( i ).typ;
}
inline int QOCIResultPrivate::fieldFromDefine( OCIDefine* d )
{
    for (int i = 0; i < fieldInf.count(); ++i) {
	if (fieldInf.at(i).def == d)
	    return i;
    }
    return -1;
}
OCILobLocator* QOCIResultPrivate::lobLocator( int i )
{
    OCILobLocator** lob = fieldInf.at(i).lob;
    if ( !lob )
	return 0;
    return *lob;
}
inline int QOCIResultPrivate::length( int i )
{
    return fieldInf.at(i).len;
}
QVariant QOCIResultPrivate::value( int i )
{
    QVariant v;
    switch ( type(i) ) {
	case QVariant::DateTime:
	    v = QVariant( qMakeDate( at(i) ) );
	break;
	//	case QVariant::CString:
	//	    v = QVariant( QCString( at(i), length(i)+1 ) );
	//	    break;
	case QVariant::String:
	    case QVariant::Double: // when converted to strings
	    case QVariant::Int:    // keep these as strings so that we do not lose precision
	    //	    qDebug( "::value(%d): " + QString::fromUcs2( (const short unsigned int*)at(i) ), i );
	    if ( d->utf16bind )
		v = QVariant( QString::fromUcs2( (const short unsigned int*)at(i) ) );
	else
	    v = QVariant( QString::fromUtf8( at(i) ) );
	break;
	case QVariant::ByteArray: {
	    QByteArray ba;
	    int len = length(i);
	    if (len > 0)
		ba.duplicate(at(i), len);
	    return QVariant( ba );
	    break;
	}
	default:
	qWarning( "QOCIResultPrivate::value: unknown data type" );
	break;
    }
    return v;
}


////////////////////////////////////////////////////////////////////////////

QOCIResult::QOCIResult( const QOCIDriver * db, QOCIPrivate* p )
: QSqlResult(db),
  cols(0)
{
    d = new QOCIPrivate();
    (*d) = (*p);
}

QOCIResult::~QOCIResult()
{
    if ( d->sql ) {
	int r = OCIHandleFree( d->sql, OCI_HTYPE_STMT );
	if ( r != 0 )
	    qOraWarning( "~QOCIResult: unable to free statement handle:", d );
    }
    delete d;
    delete cols;
}

OCIStmt* QOCIResult::statement()
{
    return d->sql;
}

bool QOCIResult::reset ( const QString& query )
{
    if ( !prepare( query ) )
	return FALSE;
    return exec();
}

bool QOCIResult::cacheNext()
{
    if ( at() == QSql::AfterLast )
	return FALSE;
    cols->fs.clearValues();
    int currentRecord = at() + 1;
    int r = 0;
    r = OCIStmtFetch (d->sql, d->err, 1, OCI_FETCH_NEXT, OCI_DEFAULT);

    if ( r == OCI_SUCCESS_WITH_INFO ) {
	qOraWarning( "QOCIResult::cacheNext: ", d );
	r = 0; //ignore it
    } else if ( r == OCI_NEED_DATA ) { /* piecewise */
	r = cols->readPiecewise( cols->fs );
    }
    if( r == OCI_ERROR ) {
	switch ( qOraErrorNumber( d ) ) {
	case 1406:
	    qWarning( "QOCI Warning: data truncated for %s", lastQuery().local8Bit() );
	    r = 0; /* ignore it */
	    break;
	default:
	    qOraWarning( "QOCIResult::cacheNext: ", d );
	}
    }
    // fetch LOBs
    if ( r == 0 ) {
	r = cols->readLOBs( cols->fs );
    }
    if ( r == 0 ) {
	QOCIPrivate::RowCache* cache = 0;
	if (!isForwardOnly()) {
	    //resize the cache if necessary
	    if (d->rowCache.capacity() <= currentRecord) {
		if ( d->rowCache.isEmpty())
		    d->rowCache.reserve(QOCI_DYNAMIC_CHUNK_SIZE);
		else
		    d->rowCache.reserve(d->rowCache.capacity() << 1);
	    }
	    cache = new QOCIPrivate::RowCache(cols->size());
//	    qDebug("Appending record %d to cache %d, count %d", cache->size(), d->rowCache.size(), d->rowCache.count());
	}

	for ( int i = 0; i < cols->size(); ++i ) {
	    if ( cols->fs.isNull( i ) && !cols->isNull( i ) ) {
		QVariant v = QVariant( cols->value( i ) );
		cols->fs.setValue( i, v );
	    }
	    if ( cols->isNull( i ) )
		cols->fs.setNull( i );
	    if (!isForwardOnly() && cache) {
//		qDebug("adding to cache%p, %d, '%s'", cache, i, cols->fs.value(i).toString().ascii());
		(*cache)[i] = cols->fs.value(i);
	    }
	    //qDebug( "got Value %d: '%s'", i, cols->fs.value(i).toString().ascii() );
	}
	if (!isForwardOnly() && cache)
	    d->rowCache.append(cache);
    } else {
	setAt( QSql::AfterLast );
    }
    return r == 0;
}

bool QOCIResult::fetchNext()
{
//    qDebug( "fetchNext: count %d, at %d", d->rowCache.count(), at() );
    if ( !isForwardOnly() && (d->rowCache.count() - 1 >= at() + 1) ) {
	setAt( at() + 1 );
	return TRUE;
    }
    if ( cacheNext() ) {
	setAt( at() + 1 );
	return TRUE;
    }
    return FALSE;
}

bool QOCIResult::fetch( int i )
{
    if ( !isForwardOnly() && (d->rowCache.count() - 1 >= i) ) {
	setAt( i );
	return TRUE;
    }
    if ( isForwardOnly() && at() > i )
	return FALSE;
    setAt( d->rowCache.count() - 1 );
    while ( at() < i ) {
	if ( !cacheNext() )
	    return FALSE;
	setAt( at() + 1 );
    }
    if ( at() == i ) {
	return TRUE;
    }
    return FALSE;
}

bool QOCIResult::fetchFirst()
{
//    qDebug("fetchFirst at %d count %d forwardOnly %d", at(), d->rowCache.count(), isForwardOnly());
    if ( isForwardOnly() && at() != QSql::BeforeFirst )
	return FALSE;
    if ( !isForwardOnly() && (d->rowCache.count() > 0) ) {
	setAt( 0 );
	return TRUE;
    }
    if ( cacheNext() ) {
	setAt( 0 );
	return TRUE;
    }
    return FALSE;
}

bool QOCIResult::fetchLast()
{
    if ( !isForwardOnly() && at() == QSql::AfterLast && d->rowCache.count() > 0 ) {
	setAt( d->rowCache.count() - 1 );
	return TRUE;
    }
    if ( at() >= QSql::BeforeFirst ) {
	int i = at();
	while ( fetchNext() )
	    i++; /* brute force */
	if ( isForwardOnly() && at() == QSql::AfterLast ) {
	    setAt( i );
	    return TRUE;
	} else
	    return fetch( d->rowCache.count() - 1 );
    }
    return FALSE;
}

QVariant QOCIResult::data( int field )
{
//   qDebug("data: count %d, size %d, at %d, field %d", d->rowCache.count(), d->rowCache.size(), at(), field);
    if (field >= cols->fs.count()) {
	qWarning( "QOCIResult::data: column %d out of range", field );
	return QVariant();
    }
    if (isForwardOnly())
	return cols->fs.value(field);
    if (d->rowCache.count() > at())
	return d->rowCache.at(at())->at(field);
    return QVariant();
}

bool QOCIResult::isNull( int field )
{
    if (field >= cols->fs.count()) {
	qWarning( "QOCIResult::isNull: column %d out of range", field );
	return TRUE;
    }
    if ( isForwardOnly() )
	return cols->fs.isNull(field);
    if (d->rowCache.count() > at())
	return d->rowCache.at(at())->at(field).isNull();
    return TRUE;
}

int QOCIResult::size()
{
    return -1;
}

int QOCIResult::numRowsAffected()
{
    int rowCount;
    OCIAttrGet( d->sql,
		OCI_HTYPE_STMT,
		&rowCount,
		NULL,
		OCI_ATTR_ROW_COUNT,
		d->err);
    return rowCount;
}

bool QOCIResult::prepare( const QString& query )
{
    int r = 0;

    delete cols;
    cols = 0;

    d->clearCache();
    if ( d->sql ) {
	r = OCIHandleFree( d->sql, OCI_HTYPE_STMT );
	if ( r != 0 )
	    qOraWarning( "QOCIResult::prepare: unable to free statement handle:", d );
    }
    if ( query.isEmpty() )
	return FALSE;
    r = OCIHandleAlloc( (dvoid *) d->env,
			(dvoid **) &d->sql,
			OCI_HTYPE_STMT,
			0,
			0);
    if ( r != 0 ) {
	qOraWarning( "QOCIResult::prepare: unable to alloc statement:", d );
	setLastError( qMakeError( "Unable to alloc statement", QSqlError::Statement, d ) );
	return FALSE;
    }
    r = OCIStmtPrepare( d->sql,
			d->err,
			(OraText*)query.unicode(),
			query.length() * sizeof( QChar ),
			OCI_NTV_SYNTAX,
			OCI_DEFAULT );
    if ( r != 0 ) {
	qOraWarning( "QOCIResult::prepare: unable to prepare statement:", d );
	setLastError( qMakeError( "Unable to prepare statement", QSqlError::Statement, d ) );
	return FALSE;
    }
    // do something with the placeholders? into a map?    
    return TRUE;
}

bool QOCIResult::exec()
{
    int r = 0;
    ub2 stmtType;
    QList<QVirtualDestructor*> tmpStorage;
    tmpStorage.setAutoDelete( TRUE );
    d->clearCache();
    
    // bind placeholders
    if ( boundValueCount() > 0
	 && d->bindValues( boundValues(), tmpStorage ) != OCI_SUCCESS ) {
	qOraWarning( "QOCIResult::exec: unable to bind value: ", d );
	setLastError( qMakeError( "Unable to bind value", QSqlError::Statement, d ) );
	return FALSE;
    }
    
    r = OCIAttrGet( d->sql,
		    OCI_HTYPE_STMT,
		    (dvoid*)&stmtType,
		    NULL,
		    OCI_ATTR_STMT_TYPE,
		    d->err );
    // execute
    if ( stmtType == OCI_STMT_SELECT )
    {
	r = OCIStmtExecute( d->svc,
			    d->sql,
			    d->err,
			    0,
			    0,
			    (CONST OCISnapshot *) NULL,
			    (OCISnapshot *) NULL,
			    OCI_DEFAULT );
	if ( r != 0 ) {
	    qOraWarning( "QOCIResult::exec: unable to execute select statement:", d );
	    setLastError( qMakeError( "Unable to execute select statement", QSqlError::Statement, d ) );
	    return FALSE;
	}
	ub4 parmCount = 0;
	int r = OCIAttrGet( d->sql, OCI_HTYPE_STMT, (dvoid*)&parmCount, NULL, OCI_ATTR_PARAM_COUNT, d->err );
	if ( r == 0 && !cols )
	    cols = new QOCIResultPrivate( parmCount, d );
	OCIParam* param = 0;
	sb4 parmStatus = 0;
	ub4 count = 1;
	parmStatus = OCIParamGet( d->sql,
				  OCI_HTYPE_STMT,
				  d->err,
				  (void**)&param,
				  count );
	while ( parmStatus == OCI_SUCCESS ) {
	    OraFieldInfo ofi = qMakeOraField( d, param );
	    QSqlField fi( ofi.name, ofi.type );
	    cols->fs.append( fi );
	    count++;
	    parmStatus = OCIParamGet( d->sql,
				      OCI_HTYPE_STMT,
				      d->err,
				      (void**)&param,
				      count );
	}
	setSelect( TRUE );
    } else { /* non-SELECT */
	r = OCIStmtExecute( d->svc, d->sql, d->err, 1,0,
				(CONST OCISnapshot *) NULL,
				(OCISnapshot *) NULL,
				d->transaction ? OCI_DEFAULT : OCI_COMMIT_ON_SUCCESS  );
	if ( r != 0 ) {
	    qOraWarning( "QOCIResult::exec: unable to execute statement:", d );
	    setLastError( qMakeError( "Unable to execute statement", QSqlError::Statement, d ) );
	    return FALSE;
	}
	setSelect( FALSE );
    }
    setAt( QSql::BeforeFirst );
    setActive( TRUE );
    
    if ( hasOutValues() )
	d->outValues( boundValues(), tmpStorage );
    
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////

#ifdef QOCI_USES_VERSION_9
QOCI9Result::QOCI9Result( const QOCIDriver * db, QOCIPrivate* p )
: QSqlResult(db),
  cols(0)
{
    d = new QOCIPrivate();
    (*d) = (*p);
}

QOCI9Result::~QOCI9Result()
{
    if ( d->sql ) {
	int r = OCIHandleFree( d->sql,OCI_HTYPE_STMT );
	if ( r != 0 )
	    qOraWarning( "~QOCI9Result: unable to free statement handle: ", d );
    }
    delete d;
    delete cols;
}

OCIStmt* QOCI9Result::statement()
{
    return d->sql;
}

bool QOCI9Result::reset ( const QString& query )
{
    if ( !prepare( query ) )
	return FALSE;
    return exec();
}

bool QOCI9Result::cacheNext( int r )
{
    cols->fs.clearValues();
    if ( r == OCI_SUCCESS_WITH_INFO ) {
        qOraWarning( "QOCI9Result::cacheNext:", d );
	r = 0; //ignore it
    } else if ( r == OCI_NEED_DATA ) { /* piecewise */
	r = cols->readPiecewise( cols->fs );
    }
    if( r == OCI_ERROR ) {
	switch ( qOraErrorNumber( d ) ) {
	case 1406:
	    qWarning( "QOCI Warning: data truncated for %s", lastQuery().local8Bit() );
	    r = 0; /* ignore it */
	    break;
	default:
	    qOraWarning( "QOCI9Result::cacheNext: ", d );
	}
    }
    // fetch LOBs
    if ( r == 0 ) {
	r = cols->readLOBs( cols->fs );
    }
    if ( r == 0 ) {
	for ( int i = 0; i < cols->size(); ++i ) {
	    if ( cols->fs.isNull( i ) && !cols->isNull( i ) ) {
		QVariant v = QVariant( cols->value( i ) );
		cols->fs.setValue( i, v );
	    }
	    if ( cols->isNull( i ) ) {
		cols->fs.setNull( i );
	    }
	}
    } else {
	setAt( QSql::AfterLast );
    }
    return r == 0;
}

bool QOCI9Result::fetchNext()
{
    int r;
    if ( !isForwardOnly() ) {
	r = OCIStmtFetch2 ( d->sql, d->err, 1, OCI_FETCH_NEXT, (sb4) 1, OCI_DEFAULT );
    } else {
	r = OCIStmtFetch ( d->sql, d->err, 1, OCI_FETCH_NEXT, OCI_DEFAULT );
    }
    if ( cacheNext( r ) ) {
	setAt( at() + 1 );
	return TRUE;
    }
    return FALSE;
}

bool QOCI9Result::fetch( int i )
{
    if ( !isForwardOnly() ) {
	int r = OCIStmtFetch2( d->sql, d->err, 1, OCI_FETCH_ABSOLUTE, (sb4) i + 1, OCI_DEFAULT );
	if ( cacheNext( r ) ) {
	    setAt( i );
	    return TRUE;
	}
	return FALSE;
    } else {
	while ( at() < i ) {
	    if ( !fetchNext() )
		return FALSE;
	}
	if ( at() == i ) {
	    return TRUE;
	}
    }
    return FALSE;
}

bool QOCI9Result::fetchFirst()
{
    if ( isForwardOnly() ) {
	if ( at() == QSql::BeforeFirst )
	    return fetchNext();
    } else {
	int r = OCIStmtFetch2( d->sql, d->err, 1, OCI_FETCH_FIRST, (sb4) 1, OCI_DEFAULT );
	if ( cacheNext( r ) ) {
	    setAt( 0 );
	    return TRUE;
	}
    }
    return FALSE;
}

bool QOCI9Result::fetchLast()
{
    if ( isForwardOnly() ) {
	int i = at();
	while ( fetchNext() )
	    i++; /* brute force */
	if ( at() == QSql::AfterLast ) {
	    setAt( i );
	    return TRUE;
	}
    } else {
	int r = OCIStmtFetch2( d->sql, d->err, 1, OCI_FETCH_LAST, (sb4) 0, OCI_DEFAULT );
	if ( cacheNext( r ) ) {
	    ub4 currentPos;
	    ub4 sz = sizeof( currentPos );
	    r = OCIAttrGet( (CONST void *) d->sql,
			    OCI_HTYPE_STMT,
			    (void *) &currentPos,
			    (ub4 *) &sz,
			    OCI_ATTR_CURRENT_POSITION,
			    d->err );
	    if ( r != 0 ) {
		qWarning( "QOCI9Result::fetchLast(): Cannot get current position" );
		setAt( QSql::AfterLast );
		return FALSE;
	    }
	    setAt( currentPos - 1 );
	    return TRUE;
	}
    }
    return FALSE;
}

bool QOCI9Result::fetchPrev()
{
    if ( !isForwardOnly() ) {
	int r = OCIStmtFetch2 ( d->sql, d->err, 1, OCI_FETCH_PRIOR, (sb4) 1, OCI_DEFAULT );
	if ( cacheNext( r ) ) {
	    setAt( at() - 1 );
	    return TRUE;
	}
    }
    return FALSE;
}

QVariant QOCI9Result::data( int field )
{
    if ( field < (int) cols->fs.count() )
	return cols->fs.value( field );
    qWarning( "QOCIResult::data: column %d out of range", field );
    return QVariant();
}

bool QOCI9Result::isNull( int field )
{
    return cols->fs.field( field )->isNull();
}

int QOCI9Result::size()
{
    return -1;
}

int QOCI9Result::numRowsAffected()
{
    int rowCount;
    OCIAttrGet( d->sql,
		OCI_HTYPE_STMT,
		&rowCount,
		NULL,
		OCI_ATTR_ROW_COUNT,
		d->err);
    return rowCount;
}

bool QOCI9Result::prepare( const QString& query )
{
//    qDebug( "QOCI9Result::prepare: " + query );
    
    int r = 0;
    delete cols;
    cols = 0;

    if ( d->sql ) {
	r = OCIHandleFree( d->sql, OCI_HTYPE_STMT );
	if ( r != 0 )
	    qOraWarning( "QOCI9Result::reset: unable to free statement handle: ", d );
    }
    if ( query.isNull() || query.length() == 0 )
	return FALSE;
    r = OCIHandleAlloc( (dvoid *) d->env,
			(dvoid **) &d->sql,
			OCI_HTYPE_STMT,
			0,
			0);
    if ( r != 0 ) {
	qOraWarning( "QOCI9Result::reset: unable to alloc statement: ", d );
	return FALSE;
    }
    r = OCIStmtPrepare( d->sql,
			d->err,
			(const OraText*)query.unicode(),
			query.length() * sizeof( QChar ),
			OCI_NTV_SYNTAX,
			OCI_DEFAULT );
    if ( r != 0 ) {
	qOraWarning( "QOCI9Result::reset: unable to prepare statement: ", d );
	return FALSE;
    }
    return TRUE;
}

bool QOCI9Result::exec()
{
    int r = 0;
    ub2 stmtType;
    QList<QVirtualDestructor*> tmpStorage;
    tmpStorage.setAutoDelete( TRUE );
    d->clearCache();
    
//    qDebug( "QOCI9Result::exec: %s", executedQuery().ascii() );
    
    // bind placeholders
    if ( boundValueCount() > 0
	 && d->bindValues( boundValues(), tmpStorage ) != OCI_SUCCESS ) {
	qOraWarning( "QOCIResult::exec: unable to bind value: ", d );
	setLastError( qMakeError( "Unable to bind value", QSqlError::Statement, d ) );
	return FALSE;
    }
    
    r = OCIAttrGet( d->sql,
		    OCI_HTYPE_STMT,
		    (dvoid*)&stmtType,
		    NULL,
		    OCI_ATTR_STMT_TYPE,
		    d->err );
    if ( stmtType == OCI_STMT_SELECT )
    {
	ub4 mode = OCI_STMT_SCROLLABLE_READONLY;
	if ( isForwardOnly() ) {
	    mode = OCI_DEFAULT;
	}
	r = OCIStmtExecute( d->svc,
			    d->sql,
			    d->err,
			    0,
			    0,
			    (CONST OCISnapshot *) NULL,
			    (OCISnapshot *) NULL,
			    mode );
	if ( r != 0 ) {
	    qOraWarning( "QOCI9Result::reset: unable to execute select statement: ", d );
	    setLastError( qMakeError( "Unable to execute select statement", QSqlError::Statement, d ) );
	    return FALSE;
	}
	ub4 parmCount = 0;
	int r = OCIAttrGet( d->sql, OCI_HTYPE_STMT, (dvoid*)&parmCount, NULL, OCI_ATTR_PARAM_COUNT, d->err );
	if ( r == 0 && !cols )
	    cols = new QOCIResultPrivate( parmCount, d );
	OCIParam* param = 0;
	sb4 parmStatus = 0;
	ub4 count = 1;
	parmStatus = OCIParamGet( d->sql,
				  OCI_HTYPE_STMT,
				  d->err,
				  (void**)&param,
				  count );
	while ( parmStatus == OCI_SUCCESS ) {
	    OraFieldInfo ofi = qMakeOraField( d, param );
	    QSqlField fi( ofi.name, ofi.type );
	    cols->fs.append( fi );
	    count++;
	    parmStatus = OCIParamGet( d->sql,
				      OCI_HTYPE_STMT,
				      d->err,
				      (void**)&param,
				      count );
	}
	setSelect( TRUE );
    } else { /* non-SELECT */
	r = OCIStmtExecute( d->svc, d->sql, d->err, 1, 0,
				(CONST OCISnapshot *) NULL,
				(OCISnapshot *) NULL,
				d->transaction ? OCI_DEFAULT : OCI_COMMIT_ON_SUCCESS );
	if ( r != 0 ) {
	    qOraWarning( "QOCI9Result::reset: unable to execute statement: ", d );
	    setLastError( qMakeError( "Unable to execute statement", QSqlError::Statement, d ) );
	    return FALSE;
	}
	setSelect( FALSE );
    }
    setAt( QSql::BeforeFirst );
    setActive( TRUE);
    
    if ( hasOutValues() )
	d->outValues( boundValues(), tmpStorage );
    
    return TRUE;
}

#endif //QOCI_USES_VERSION_9
////////////////////////////////////////////////////////////////////////////


QOCIDriver::QOCIDriver(QObject* parent)
    : QSqlDriver(parent)
{
    init();
}

QOCIDriver::QOCIDriver(OCIEnv* env, OCIError* err, OCISvcCtx* ctx, QObject* parent)
    : QSqlDriver(parent)
{
    d = new QOCIPrivate();
    d->env = env;
    d->err = err;
    d->svc = ctx;
    if ( env && err && ctx ) {
	setOpen( TRUE );
	setOpenError( FALSE );
    }
}

void QOCIDriver::init()
{
    d = new QOCIPrivate();
#ifdef QOCI_USES_VERSION_9
    int r = OCIEnvCreate( &d->env,
			    OCI_UTF16 | OCI_OBJECT,
			    NULL,
			    NULL,
			    NULL,
			    NULL,
			    0,
			    NULL );
    d->utf16bind = TRUE;
#else
    // this call is deprecated in Oracle >= 8.1.x, but still works
    int r = OCIInitialize( OCI_DEFAULT | OCI_OBJECT,
			    NULL,
			    NULL,
			    NULL,
			    NULL );
    if ( r != 0 )
	qOraWarning( "QOCIDriver: unable to initialize environment:", d );
    r = OCIEnvInit( &d->env,
		    OCI_DEFAULT,
		    0,
		    NULL );
    d->utf16bind = FALSE;
#endif  //QOCI_USES_VERSION_9
    if ( r != 0 )
	qOraWarning( "QOCIDriver: unable to create environment:", d );
    r = OCIHandleAlloc( (dvoid *) d->env,
			(dvoid **) &d->err,
			OCI_HTYPE_ERROR,
			(size_t) 0,
			(dvoid **) 0);
    if ( r != 0 )
	qOraWarning( "QOCIDriver: unable to alloc error handle:", d );
    r = OCIHandleAlloc( (dvoid *) d->env,
			(dvoid **) &d->svc,
			OCI_HTYPE_SVCCTX,
			(size_t) 0,
			(dvoid **) 0);
    if ( r != 0 )
	qOraWarning( "QOCIDriver: unable to alloc service context:", d );
    if ( r != 0 )
	setLastError( qMakeError( "Unable to initialize", QSqlError::Connection, d ) );
}

QOCIDriver::~QOCIDriver()
{
    cleanup();
    delete d;
}

bool QOCIDriver::hasFeature( DriverFeature f ) const
{
    switch ( f ) {
    case Transactions:
	return TRUE;
    case QuerySize:
	return FALSE;
    case BLOB:
	return TRUE;
    case Unicode:
	return d->serverVersion >= 9;
    case PreparedQueries:
	return TRUE;
    case NamedPlaceholders:
	return TRUE;
    default:
	return FALSE;
    }
}

bool QOCIDriver::open( const QString & db,
		       const QString & user,
		       const QString & password,
		       const QString & ,
		       int,
		       const QString &)
{
    if ( isOpen() )
	close();
    int r = OCILogon(	d->env,
			d->err,
			&d->svc,
			(OraText*) user.unicode(),
			user.length() * sizeof( QChar ),
			(OraText*)password.unicode(),
			password.length() * sizeof( QChar ),
			(OraText*)db.unicode(),
			db.length() * sizeof( QChar ) );
    if ( r != 0 ) {
	setLastError( qMakeError("Unable to logon", QSqlError::Connection, d ) );
	setOpenError( TRUE );
	return FALSE;
    }

    // get server version
    text vertxt[512];
    r = OCIServerVersion( d->svc,
			  d->err,
			  vertxt,
			  sizeof( vertxt ),
			  OCI_HTYPE_SVCCTX );
    if ( r != 0 ) {
#ifdef QT_CHECKRANGE
	qWarning( "QOCIDriver::open: could not get Oracle server version." );
#endif
    } else {
	QString versionStr = d->utf16bind ? QString::fromUcs2( (unsigned short*)vertxt ) : QString::fromUtf8( (char*)vertxt, sizeof( vertxt ) );
	QRegExp vers("([0-9]+)\\.[0-9\\.]+[0-9]");
	if ( vers.search( versionStr ) >= 0 )
	    d->serverVersion = vers.cap( 1 ).toInt();
	if ( d->serverVersion == 0 )
	    d->serverVersion = -1;
}
    setOpen( TRUE );
    setOpenError( FALSE );
    d->user = user.upper();
    
    QSqlQuery q = createQuery();
    q.setForwardOnly( TRUE );
    if ( q.exec( "select parameter, value from nls_database_parameters "
		 "where parameter = 'NLS_CHARACTERSET' "
		 "or parameter = 'NLS_NCHAR_CHARACTERSET'" ) ) {
	while ( q.next() ) {
	    // qDebug( "NLS: " + q.value( 0 ).toString() ); // ###
	    if ( q.value( 0 ).toString() == "NLS_CHARACTERSET" &&
		 q.value( 1 ).toString().upper().startsWith( "UTF8" ) ) {
		d->utf8 = TRUE;
	    } else if ( q.value( 0 ).toString() == "NLS_NCHAR_CHARACTERSET" &&
		 q.value( 1 ).toString().upper().startsWith( "UTF8" ) ) {
		d->nutf8 = TRUE;
	    }
	}
    } else {
#ifdef QT_CHECKRANGE
	qWarning( "QOCIDriver::open: could not get Oracle server character set." );
#endif
    }
        
    return TRUE;
}

void QOCIDriver::close()
{
    OCILogoff( d->svc, d->err );
    setOpen( FALSE );
    setOpenError( FALSE );
}

void QOCIDriver::cleanup()
{
    if ( isOpen() ) {
	OCILogoff( d->svc, d->err );
    }
    OCIHandleFree( (dvoid *) d->svc, OCI_HTYPE_SVCCTX );
    OCIHandleFree( (dvoid *) d->err, OCI_HTYPE_ERROR );
    OCIHandleFree( (dvoid *) d->env, OCI_HTYPE_ENV );
}

QSqlQuery QOCIDriver::createQuery() const
{
#ifdef QOCI_USES_VERSION_9
    if ( d->serverVersion >= 9 )
	return QSqlQuery( new QOCI9Result( this, d ) );
#endif
    return QSqlQuery( new QOCIResult( this, d ) );
}

bool QOCIDriver::beginTransaction()
{
    if ( !isOpen() ) {
	qWarning( "QOCIDriver::beginTransaction: Database not open" );
	return FALSE;
    }
    d->transaction = TRUE;
    int r = OCITransStart ( d->svc,
			    d->err,
			    2,
			    OCI_TRANS_READWRITE );
    if ( r == OCI_ERROR ) {
	qOraWarning( "QOCIDriver::beginTransaction: ", d );
	return FALSE;
    }
    return TRUE;
}

bool QOCIDriver::commitTransaction()
{
    if ( !isOpen() ) {
	qWarning( "QOCIDriver::commitTransaction: Database not open" );
	return FALSE;
    }
    d->transaction = FALSE;
    int r = OCITransCommit ( d->svc,
			     d->err,
			     0 );
    if ( r == OCI_ERROR ) {
	qOraWarning( "QOCIDriver::commitTransaction:", d );
	return FALSE;
    }
    return TRUE;
}

bool QOCIDriver::rollbackTransaction()
{
    if ( !isOpen() ) {
	qWarning( "QOCIDriver::rollbackTransaction: Database not open" );
	return FALSE;
    }
    d->transaction = FALSE;
    int r = OCITransRollback ( d->svc,
			       d->err,
			       0 );
    if ( r == OCI_ERROR ) {
	qOraWarning( "QOCIDriver::rollbackTransaction:", d );
	return FALSE;
    }
    return TRUE;
}

QStringList QOCIDriver::tables( const QString& typeName ) const
{
    QStringList tl;
    if ( !isOpen() )
	return tl;

    QSqlQuery t = createQuery();
    t.setForwardOnly( TRUE );
    int type = typeName.toInt();
    if ( typeName.isEmpty() || ((type & (int)QSql::Tables) == (int)QSql::Tables) ) {
        t.exec( "select owner, table_name from all_tables "
		"where owner != 'MDSYS' "
		"and owner != 'LBACSYS' "
		"and owner != 'SYS' "
		"and owner != 'SYSTEM' "
		"and owner != 'WKSYS'"
		"and owner != 'CTXSYS'"
		"and owner != 'WMSYS'" );
        while ( t.next() ) {
	    if ( t.value(0).toString() != d->user )
		tl.append( t.value(0).toString() + "." + t.value(1).toString() );
	    else
		tl.append( t.value(1).toString() );
	}
    }
    if ( (type & (int)QSql::Views) == (int)QSql::Views ) {
        t.exec( "select owner, view_name from all_views "
                "where owner != 'MDSYS' "
                "and owner != 'LBACSYS' "
                "and owner != 'SYS' "
                "and owner != 'SYSTEM' "
                "and owner != 'WKSYS'"
                "and owner != 'CTXSYS'"
                "and owner != 'WMSYS'" );
        while ( t.next() ) {
            if ( t.value(0).toString() != d->user )
                tl.append( t.value(0).toString() + "." + t.value(1).toString() );
            else
                tl.append( t.value(1).toString() );
        }
    }
    if ( (type & (int)QSql::SystemTables) == (int)QSql::SystemTables ) {
	t.exec( "select table_name from dictionary" );
	while ( t.next() ) {
	    tl.append( t.value(0).toString() );
	}
    }
    return tl;
}

void qSplitTableAndOwner( const QString & tname, QString * tbl,
			  QString * owner )
{
    int i = tname.find('.'); // prefixed with owner?
    if ( i != -1 ) {
	*tbl = tname.right( tname.length() - i - 1 ).upper();
	*owner = tname.left( i ).upper();
    } else {
	*tbl = tname.upper();
    }
}

QSqlRecord QOCIDriver::record( const QString& tablename ) const
{
//    qDebug( "*** record QString" );
    QSqlRecord fil;
    if ( !isOpen() )
	return fil;
    QSqlQuery t = createQuery();
    // using two separate queries for this is A LOT faster than using
    // eg. a sub-query on the sys.synonyms table
    QString stmt( "select column_name, data_type, data_length, "
		  "data_precision, data_scale from all_tab_columns "
		  "where table_name=%1" );
    bool buildRecord = FALSE;
    QString table, owner, tmpStmt;
    qSplitTableAndOwner( tablename, &table, &owner );
    tmpStmt = stmt.arg( "'" + table + "'" );
    if ( owner.isEmpty() ) {
	owner = d->user;
    }
    tmpStmt += " and owner='" + owner + "'";
    t.setForwardOnly( TRUE );
    t.exec( tmpStmt );
    if ( !t.next() ) { // check to see if this is a synonym instead
	stmt = stmt.arg( "(select tname from sys.synonyms where sname='"
			 + table +"' and creator=owner)" );
	t.setForwardOnly( TRUE );
	t.exec( stmt );
	if ( t.next() )
	    buildRecord = TRUE;
    } else {
	buildRecord = TRUE;
    }
    
    if ( buildRecord ) {
	do {
	    QVariant::Type ty = qDecodeOCIType( t.value(1).toString(), t.value(2).toInt(),
						t.value(3).toInt(), t.value(4).toInt() );
	    QSqlField f( t.value(0).toString(), ty );
	    fil.append( f );
	} while ( t.next() );
    }
    return fil;
}

QSqlRecord QOCIDriver::record( const QSqlQuery& query ) const
{
//    qDebug( "*** record Query" );
    QSqlRecord fil;
    if ( !isOpen() )
	return fil;
    if ( !query.isActive() )
	return fil;
    if ( query.isActive() && query.driver() == this ) {
#ifdef QOCI_USES_VERSION_9
	if ( d->serverVersion >= 9 ) {
	    QOCI9Result* result = (QOCI9Result*)query.result();
	    if ( result && result->cols )
		fil = result->cols->fs;
	    return fil;
	}
#endif
	QOCIResult* result = (QOCIResult*)query.result();
	if ( result && result->cols )
	    fil = result->cols->fs;
    }
    return fil;
}

QSqlRecordInfo QOCIDriver::recordInfo( const QString& tablename ) const
{
//    qDebug( "*** recordInfo QString" );
    QSqlRecordInfo fil;
    if ( !isOpen() )
	return fil;
    
    QSqlQuery t = createQuery();
    // using two separate queries for this is A LOT faster than using
    // eg. a sub-query on the sys.synonyms table
    QString stmt( "select column_name, data_type, data_length, "
		  "data_precision, data_scale, nullable, data_default%1"
		  "from all_tab_columns "
		  "where table_name=%2" );
    if ( d->serverVersion >= 9 ) {
	stmt = stmt.arg( ", char_length " );
    } else {
	stmt = stmt.arg( " " );
    }
    bool buildRecordInfo = FALSE;
    QString table, owner, tmpStmt;
    qSplitTableAndOwner( tablename, &table, &owner );
    tmpStmt = stmt.arg( "'" + table + "'" );
    if ( owner.isEmpty() ) {
	owner = d->user;
    }
    tmpStmt += " and owner='" + owner + "'";
    t.setForwardOnly( TRUE );
    t.exec( tmpStmt );
    if ( !t.next() ) { // try and see if the tablename is a synonym
	stmt= stmt.arg( "(select tname from sys.synonyms where sname='"
			+ table + "' and creator=owner)" );
	t.setForwardOnly( TRUE );
	t.exec( stmt );
	if ( t.next() )
	    buildRecordInfo = TRUE;
    } else {
	buildRecordInfo = TRUE;
    }
    if ( buildRecordInfo ) {
	do {
	    QVariant::Type ty = qDecodeOCIType( t.value(1).toString(), t.value(2).toInt(), t.value(3).toInt(), t.value(4).toInt() );
	    bool required = t.value( 5 ).toString() == "N";
	    int prec = -1;
	    if ( !t.isNull( 3 ) ) {
		prec = t.value( 3 ).toInt();
	    }
	    int size = t.value( 2 ).toInt();
	    if ( d->serverVersion >= 9 && ( ty == QVariant::String ) ) {
		// Oracle9: data_length == size in bytes, char_length == amount of characters
		size = t.value( 7 ).toInt();
	    }
	    QSqlFieldInfo f( t.value(0).toString(), ty, required, size, prec, t.value( 6 ) );
	    fil.append( f );
	} while (t.next() );
    }
    return fil;
}

QSqlRecordInfo QOCIDriver::recordInfo( const QSqlQuery& query ) const
{
///    qDebug( "*** recordInfo Query" );
    QSqlRecordInfo inf;
    if ( query.isActive() && query.driver() == this ) {
#ifdef QOCI_USES_VERSION_9
	if ( d->serverVersion >= 9 ) {
	    QOCI9Result* result = (QOCI9Result*)query.result();
	    if ( result && result->cols )
		result->cols->getOraFields( inf );
	    return inf;
	}
#endif
	QOCIResult* result = (QOCIResult*)query.result();
	if ( result && result->cols )
	    result->cols->getOraFields( inf );
    }
    return inf;
}

QSqlIndex QOCIDriver::primaryIndex( const QString& tablename ) const
{
    QSqlIndex idx( tablename );
    if ( !isOpen() )
	return idx;
    QSqlQuery t = createQuery();
    QString stmt( "select b.column_name, b.index_name, a.table_name, a.owner "
		  "from all_constraints a, all_ind_columns b "
		  "where a.constraint_type='P' "
		  "and b.index_name = a.constraint_name "
                  "and b.index_owner = a.owner" );

    bool buildIndex = FALSE;
    QString table, owner, tmpStmt;
    qSplitTableAndOwner( tablename, &table, &owner );
    tmpStmt = stmt + " and a.table_name='" + table + "'";
    if ( owner.isEmpty() ) {
	owner = d->user;
    }
    tmpStmt += " and a.owner='" + owner + "'";
    t.setForwardOnly( TRUE );
    t.exec( tmpStmt );
    
    if ( !t.next() ) {
	stmt += " and a.table_name=(select tname from sys.synonyms "
                "where sname='" + table + "' and creator=a.owner)";
	t.setForwardOnly( TRUE );
	t.exec( stmt );
	if ( t.next() ) {
	    owner = t.value(3).toString();
	    buildIndex = TRUE;
	}
    } else {
	buildIndex = TRUE;
    }
    if ( buildIndex ) {	
	QSqlQuery tt = createQuery();
	tt.setForwardOnly( TRUE );
	idx.setName( t.value(1).toString() );
	do {
	    tt.exec( "select data_type from all_tab_columns where table_name='" + 
		     t.value(2).toString() + "' and column_name='" + 
		     t.value(0).toString() + "' and owner='" + owner + "'" );
	    if ( !tt.next() ) {
		return QSqlIndex();
	    }
	    QSqlField f( t.value(0).toString(), qDecodeOCIType( tt.value(0).toString(), 0, 0, 0 ) );
	    idx.append(f);
	} while ( t.next() );
	return idx;
    }
    return QSqlIndex();
}

QString QOCIDriver::formatValue( const QSqlField* field, bool ) const
{
    switch ( field->type() ) {
    case QVariant::String: {
	if ( d->serverVersion >= 9 ) {
	    QString encStr = "UNISTR('";
	    const QString srcStr = field->value().toString();
	    for ( int i = 0; i < srcStr.length(); ++i ) {
		encStr += '\\' + QString::number( srcStr.at( i ).unicode(), 16 ).rightJustify( 4, '0' );
	    }
	    encStr += "')";
	    return encStr;
	} else {
	    return QSqlDriver::formatValue( field );
	}
	break;
    }
    case QVariant::DateTime: {
	QDateTime datetime = field->value().toDateTime();
	QString datestring;
	if ( datetime.isValid() ) {
	    datestring = "TO_DATE('" + QString::number(datetime.date().year()) + "-" + \
				 QString::number(datetime.date().month()) + "-" + \
				 QString::number(datetime.date().day()) + " " + \
				 QString::number(datetime.time().hour()) + ":" + \
				 QString::number(datetime.time().minute()) + ":" + \
				 QString::number(datetime.time().second()) + "',"
				 "'YYYY-MM-DD HH24:MI:SS')";
	} else {
	    datestring = "NULL";
	}
	return datestring;
	break;
    }
    case QVariant::Date: {
	QDate date = field->value().toDate();
	QString datestring;
	if ( date.isValid() ) {
	    datestring = "TO_DATE('" + QString::number(date.year()) + "-" + \
				 QString::number(date.month()) + "-" + \
				 QString::number(date.day()) + "',"
				 "'YYYY-MM-DD')";
	} else {
	    datestring = "NULL";
	}
	return datestring;
	break;
    }
    default:
	break;
    }
    return QSqlDriver::formatValue( field );
}

OCIEnv* QOCIDriver::environment()
{
    return d->env;
}

OCISvcCtx* QOCIDriver::serviceContext()
{
    return d->svc;
}
