/**********************************************************************
** Copyright (C) 2000 Trolltech AS.  All rights reserved.
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

#include "project.h"
#include "formwindow.h"
#include "config.h"
#include "designerappiface.h"
#include "pixmapcollection.h"
#include "dbconnectionimpl.h"

#include <qfile.h>
#include <qtextstream.h>
#include <qurl.h>
#include <qobjectlist.h>
#include <qfeatures.h>
#include <qtextcodec.h>
#include <qdom.h>
#include <qmessagebox.h>
#include <qapplication.h>

#ifndef QT_NO_SQL
#include <qsqlrecord.h>
#include <qdatatable.h>
#endif


#ifndef QT_NO_SQL
DatabaseConnection::~DatabaseConnection()
{
    delete iface;
}

bool DatabaseConnection::refreshCatalog()
{
#ifndef QT_NO_SQL
    if ( loaded )
	return TRUE;
    if ( !open() )
	return FALSE;
    tbls = conn->tables();
    flds.clear();
    for ( QStringList::Iterator it = tbls.begin(); it != tbls.end(); ++it ) {
	QSqlRecord fil = conn->record( *it );
	QStringList lst;
	for ( uint j = 0; j < fil.count(); ++j )
	    lst << fil.field( j )->name();
	flds.insert( *it, lst );
    }
    loaded = TRUE;
    conn->close();
    return loaded;
#else
    return FALSE;
#endif
}

#ifndef QT_NO_SQL
void DatabaseConnection::remove()
{
    if ( nm == "(default)" )
	QSqlDatabase::removeDatabase( QSqlDatabase::defaultConnection );
    else
	QSqlDatabase::removeDatabase( nm );
}
#endif

bool DatabaseConnection::open( bool suppressDialog )
{
#ifndef QT_NO_SQL
    // register our name, if nec
    if ( nm == "(default)" ) {
	if ( !QSqlDatabase::contains() ) // default doesn't exists?
	    conn = QSqlDatabase::addDatabase( drv );
	else
	    conn = QSqlDatabase::database();
    } else {
	if ( !QSqlDatabase::contains( nm ) )
	    conn = QSqlDatabase::addDatabase( drv, nm );
	else
	    conn = QSqlDatabase::database( nm );
    }
    conn->setDatabaseName( dbName );
    conn->setUserName( uname );
    conn->setPassword( pword );
    conn->setHostName( hname );
    bool success = conn->open();
    for( ; suppressDialog == FALSE ; ) {
	bool done = FALSE;
	if ( !success ) {
	    DatabaseConnectionEditor dia( this, 0  , 0 , TRUE );
	    switch( dia.exec() ) {
	    case QDialog::Accepted:
		done = FALSE;
		break;
	    case QDialog::Rejected:
		done = TRUE;
		break;
	    }
	}
	if ( done )
	    break;
	conn->setUserName( uname );
	conn->setPassword( pword );
	conn->setHostName( hname );
	success = conn->open();
	if ( !success ) {
	    switch( QMessageBox::warning( 0, QApplication::tr( "Connection" ),
					  QApplication::tr( "Could not connect to the database.\n"
							    "Press 'OK' to continue or 'Cancel' to "
							    "specify different\nconnection information.\n" )
					  + QString( "[" + conn->lastError().driverText() + "\n" +
						     conn->lastError().databaseText() + "]\n" ),
					  QApplication::tr( "&OK" ),
					  QApplication::tr( "&Cancel" ), 0, 0, 1 ) ) {
	    case 0: // OK or Enter
		continue;
		break;
	    case 1: // Cancel or Escape
		done = TRUE;
		break;
	    }
	} else
	    break;
	if ( done )
	    break;
    }
    if ( !success ) {
	if ( nm == "(default)" )
	    QSqlDatabase::removeDatabase( QSqlDatabase::defaultConnection );
	else
	    QSqlDatabase::removeDatabase( nm );
    }
    return success;
#else
    return FALSE;
#endif
}

void DatabaseConnection::close()
{
    if ( !loaded )
	return;
#ifndef QT_NO_SQL
    if ( conn ) {
	conn->close();
    }
#endif
}

DesignerDatabase *DatabaseConnection::iFace()
{
    if ( !iface )
	iface = new DesignerDatabaseImpl( this );
    return iface;
}

#endif

////////

Project::Project( const QString &fn, const QString &pName, QInterfaceManager<ProjectSettingsInterface> *pm )
    : proName( pName ), projectSettingsPluginManager( pm )
{
    pixCollection = new PixmapCollection( this );
    iface = 0;
    lang = "C++";
    setFileName( fn );
    if ( !pName.isEmpty() )
	proName = pName;
}

Project::~Project()
{
    delete iface;
}

#ifndef QT_NO_SQL
DatabaseConnection *Project::databaseConnection( const QString &name )
{
    for ( DatabaseConnection *conn = dbConnections.first();
	  conn;
	  conn = dbConnections.next() ) {
	if ( conn->name() == name )
	    return conn;
    }
    return 0;
}
#endif

void Project::setFileName( const QString &fn, bool doClear )
{
    filename = fn;
    if ( !doClear )
	return;
    clear();
    if ( QFile::exists( fn ) )
	parse();
}

QString Project::fileName() const
{
    return filename;
}

QStringList Project::uiFiles() const
{
    return uifiles;
}

QString Project::imageFile() const
{
    return imgFile;
}


QString Project::databaseDescription() const
{
    return dbFile;
}

void Project::setProjectName( const QString &n )
{
    proName = n;
    save();
}

QString Project::projectName() const
{
    return proName;
}

QString Project::fixedProjectName() const
{
    QString s = proName;
    for ( int i = 0; i < (int)s.length(); ++i ) {
	uchar r = s[i].row();
	uchar c = s[i].cell();
	if ( r == 0 && ( ( c >= '0' && c <= '9' ) ||
			 ( c >= 'a' && c <= 'z' ) ||
			 ( c >= 'A' && c <= 'Z' ) ) )
	    continue;
	s[i] = '_';
    }
    return s;
}

static QString parse_part( const QString &part )
{
    QString res;
    bool inName = FALSE;
    QString currName;
    for ( int i = 0; i < (int)part.length(); ++i ) {
	QChar c = part[ i ];
	if ( !inName ) {
	    if ( c != ' ' && c != '\t' && c != '\n' && c != '=' && c != '\\' )
		inName = TRUE;
	    else
		continue;
	}
	if ( inName ) {
	    if ( c == '\n' )
		break;
	    res += c;
	}
    }
    return res;
}

void Project::parse()
{
    QFile f( filename );
    if ( !f.exists() || !f.open( IO_ReadOnly ) )
	return;
    QTextStream ts( &f );
    QString contents = ts.read();
    f.close();

    QString fl( QFileInfo( filename ).baseName() );
    proName = fl[ 0 ].upper() + fl.mid( 1 );

    int i = contents.find( "INTERFACES" );
    if ( i != -1 ) {
	QString part = contents.mid( i + QString( "INTERFACES" ).length() );
	QStringList lst;
	bool inName = FALSE;
	QString currName;
	for ( i = 0; i < (int)part.length(); ++i ) {
	    QChar c = part[ i ];
	    if ( ( c.isLetter() || c.isDigit() || c == '.' || c == '/' ) &&
		 c != ' ' && c != '\t' && c != '\n' && c != '=' && c != '\\' ) {
		if ( !inName )
		    currName = QString::null;
		currName += c;
		inName = TRUE;
	    } else {
		if ( inName ) {
		    inName = FALSE;
		    if ( currName.right( 3 ).lower() == ".ui" )
			lst.append( currName );
		}
	    }
	}

	uifiles = lst;
    }

    i = contents.find( "DBFILE" );
    if ( i != -1 ) {
	dbFile = "";
	QString part = contents.mid( i + QString( "DBFILE" ).length() );
	dbFile = parse_part( part );
    }

    i = contents.find( "IMAGEFILE" );
    if ( i != -1 ) {
	imgFile = "";
	QString part = contents.mid( i + QString( "IMAGEFILE" ).length() );
	imgFile = parse_part( part );
    }

    i = contents.find( "PROJECTNAME" );
    if ( i != -1 ) {
	proName = "";
	QString part = contents.mid( i + QString( "PROJECTNAME" ).length() );
	proName = parse_part( part );
    }

    i = contents.find( "LANGUAGE" );
    if ( i != -1 ) {
	lang = "";
	QString part = contents.mid( i + QString( "LANGUAGE" ).length() );
	lang = parse_part( part );
    }

    i = contents.find( "DESIGNER_SOURCES" );
    if ( i != -1 ) {
	QString part = contents.mid( i + QString( "DESIGNER_SOURCES" ).length() );
	QString s = parse_part( part );
	QStringList lst = QStringList::split( ' ', s );
	for ( QStringList::Iterator it = lst.begin(); it != lst.end(); ++it ) {
	    SourceFile *f = new SourceFile( makeAbsolute( *it ) );
	    sources.append( f );
	    MetaDataBase::addEntry( f );
	}
    }

    updateCustomSettings();

    for ( QStringList::Iterator it = csList.begin(); it != csList.end(); ++it ) {
	i = contents.find( *it );
	if ( i != -1 ) {
	    QString val = "";
	    QString part = contents.mid( i + QString( *it ).length() );
	    val = parse_part( part );
	    setCustomSetting( *it, val );
	}
    }

    loadConnections();
    loadImages();
}

void Project::clear()
{
    uifiles.clear();
    dbFile = "";
    imgFile = "";
    proName = "unnamed";
    loadedForms.clear();
    desc = "";
}

void Project::addUiFile( const QString &f, FormWindow *fw )
{
    uifiles << f;
    if ( fw )
	formWindows.insert( fw, f );
    save();
}

void Project::removeUiFile( const QString &f, FormWindow *fw )
{
    formWindows.remove( fw );
    uifiles.remove( f );
    save();
}

void Project::setDatabaseDescription( const QString &db )
{
    dbFile = db;
}

void Project::setImageFile( const QString &f )
{
    imgFile = f;
    pixCollection->createCppFile();
}

void Project::setDescription( const QString &s )
{
    desc = s;
}

QString Project::description() const
{
    return desc;
}

void Project::setUiFiles( const QStringList &lst )
{
    uifiles = lst;
    save();
}

bool Project::isValid() const
{
     // #### do more checking here?
    if ( filename.isEmpty() || proName.isEmpty() )
	return FALSE;

    return TRUE;
}

bool Project::hasFormWindow( FormWindow* fw ) const
{
    return formWindows.contains( fw );
}

void Project::setFormWindow( const QString &f, FormWindow *fw )
{
    formWindows.remove( fw );
    formWindows.insert( fw, f );
    save();
}

void Project::setFormWindowFileName( FormWindow *fw, const QString &f )
{
    QString s = *formWindows.find( fw );
    uifiles.remove( s );
    uifiles << f;
    formWindows.remove( fw );
    formWindows.insert( fw, f );
    save();
}

QString Project::makeAbsolute( const QString &f )
{
    QUrl u( QFileInfo( filename ).dirPath( TRUE ), f );
    return u.path();
}

QString Project::makeRelative( const QString &f )
{
    QString p = QFileInfo( filename ).dirPath( TRUE );
    QString f2 = f;
    if ( f2.left( p.length() ) == p )
	f2.remove( 0, p.length() + 1 );
    return f2;
}

static void remove_contents( QString &contents, const QString &s )
{
    int i = contents.find( s );
    if ( i != -1 ) {
	int start = i;
	int end = contents.find( '\n', i );
	if ( end == -1 )
	    end = contents.length() - 1;
	contents.remove( start, end - start + 1 );
    }
}

void Project::save()
{
    if ( proName == "<No Project>" || filename.isEmpty() )
	return;

    QFile f( filename );
    QString contents;
    if ( f.open( IO_ReadOnly ) ) {
	QTextStream ts( &f );
	contents = ts.read();
	f.close();
    } else {
	contents += "TEMPLATE\t= app\nCONFIG\t= qt warn_on release\nTARGET\t= " + fixedProjectName() + "\n";
    }

    int i = contents.find( "INTERFACES" );
    if ( i != -1 ) {
	int start = i;
	int end = i;
	i = contents.find( '\n', i );
	if ( i == -1 ) {
	    end = contents.length() - 1;
	} else {
	    end = i;
	    int lastNl = i;
	    for ( ; i < (int)contents.length(); ++i ) {
		int j = contents.find( '\n', lastNl + 1 );
		if ( i == -1 ) {
		    end = contents.length() - 1;
		    break;
		} else {
		    if ( contents.mid( lastNl, j - lastNl + 1 ).find( '=' ) == -1 )
			lastNl = j;
		    else
			break;
		}
	    }
	}
	contents.remove( start, end - start + 1 );
    }

    if ( !uifiles.isEmpty() ) {
	contents += "INTERFACES\t= ";
	for ( QStringList::Iterator it = uifiles.begin(); it != uifiles.end(); ++it )
	    contents += *it + " ";
	contents += "\n";
    }

    remove_contents( contents, "DBFILE" );
    remove_contents( contents, "IMAGEFILE" );
    remove_contents( contents, "PROJECTNAME" );
    remove_contents( contents, "LANGUAGE" );
    remove_contents( contents, "DESIGNER_SOURCES" );
    remove_contents( contents, "{SOURCES+=" );
    if ( !dbFile.isEmpty() )
	contents += "DBFILE\t= " + dbFile + "\n";
    if ( !imgFile.isEmpty() )
	contents += "IMAGEFILE\t= " + imgFile + "\n";

    if ( !proName.isEmpty() )
	contents += "PROJECTNAME\t= " + proName + "\n";

    contents += "LANGUAGE\t= " + lang + "\n";

    if ( !sources.isEmpty() ) {
	contents += "DESIGNER_SOURCES\t=";
	for ( SourceFile *f = sources.first(); f; f = sources.next() )
	    contents += makeRelative( f->fileName() ) + " ";
	contents += "\n";
    }

    if ( !imageFile().isEmpty() ) {
	contents += "{SOURCES+=" + imageFile();
	if ( !sources.isEmpty() )
	    contents += " $$DESIGNER_SOURCES}\n";
	else
	    contents += "}\n";
    }

    for ( QStringList::Iterator it = csList.begin(); it != csList.end(); ++it ) {
	remove_contents( contents, *it );
	QString val = *customSettings.find( *it );
	contents += *it + "\t= " + val + "\n";
    }

    if ( !f.open( IO_WriteOnly ) ) {
	//## more of a warning here? mbox?
	qWarning( "Couldn't write project file " + filename );
	return;
    }

    QTextStream os( &f );
    os << contents;

    f.close();

    saveConnections();
    saveImages();
}

#ifndef QT_NO_SQL
QList<DatabaseConnection> Project::databaseConnections() const
{
    return dbConnections;
}
#endif

#ifndef QT_NO_SQL
void Project::setDatabaseConnections( const QList<DatabaseConnection> &lst )
{
    dbConnections = lst;
}
#endif

#ifndef QT_NO_SQL
void Project::addDatabaseConnection( DatabaseConnection *conn )
{
    dbConnections.append( conn );
    saveConnections();
}
#endif

#ifndef QT_NO_SQL
void Project::removeDatabaseConnection( const QString &c )
{
    for ( DatabaseConnection *conn = dbConnections.first(); conn; conn = dbConnections.next() ) {
	if ( conn->name() == c ) {
	    conn->remove();
	    dbConnections.removeRef( conn );
	    delete conn;
	    return;
	}
    }
}
#endif

#ifndef QT_NO_SQL
QStringList Project::databaseConnectionList()
{
    QStringList lst;
    for ( DatabaseConnection *conn = dbConnections.first(); conn; conn = dbConnections.next() )
	lst << conn->name();
    return lst;
}
#endif

#ifndef QT_NO_SQL
QStringList Project::databaseTableList( const QString &connection )
{
    DatabaseConnection *conn = databaseConnection( connection );
    if ( !conn ) {
	return QStringList();
    }
    return conn->tables();
}
#endif

#ifndef QT_NO_SQL
QStringList Project::databaseFieldList( const QString &connection, const QString &table )
{
    DatabaseConnection *conn = databaseConnection( connection );
    if ( !conn )
	return QStringList();
    return conn->fields( table );
}
#endif

#ifndef QT_NO_SQL
static QString makeIndent( int indent )
{
    QString s;
    s.fill( ' ', indent * 4 );
    return s;
}
#endif

#ifndef QT_NO_SQL
static void saveSingleProperty( QTextStream &ts, const QString& name, const QString& value, int indent )
{
    ts << makeIndent( indent ) << "<property name=\"" << name << "\">" << endl;
    ++indent;
    ts << makeIndent( indent ) << "<string>" << value << "</string>" << endl;
    --indent;
    ts << makeIndent( indent ) << "</property>" << endl;
}
#endif

#ifndef QT_NO_SQL
static bool inSaveConnections = FALSE;
#endif
void Project::saveConnections()
{
#ifndef QT_NO_SQL
    if ( inSaveConnections )
	return;
    inSaveConnections = TRUE;
    if ( !QFile::exists( makeAbsolute( dbFile ) ) ) {
	QFileInfo fi( fileName() );
	setDatabaseDescription( fi.baseName() + ".db" );
    }

    /* .db xml */
    QFile f( makeAbsolute( dbFile ) );
    if ( f.open( IO_WriteOnly ) ) {
	QTextStream ts( &f );
	ts.setCodec( QTextCodec::codecForName( "UTF-8" ) );
	ts << "<!DOCTYPE DB><DB version=\"1.0\">" << endl;

	/* db connections */
	int indent = 0;
	for ( DatabaseConnection *conn = dbConnections.first(); conn; conn = dbConnections.next() ) {
	    ts << makeIndent( indent ) << "<connection>" << endl;
	    ++indent;
	    saveSingleProperty( ts, "name", conn->name(), indent );
	    saveSingleProperty( ts, "driver", conn->driver(), indent );
	    saveSingleProperty( ts, "database", conn->database(), indent );
	    saveSingleProperty( ts, "username", conn->username(), indent );
	    saveSingleProperty( ts, "hostname", conn->hostname(), indent );

	    /* connection tables */
	    QStringList tables = conn->tables();
	    for ( QStringList::Iterator it = tables.begin();
		  it != tables.end(); ++it ) {
		ts << makeIndent( indent ) << "<table>" << endl;
		++indent;
		saveSingleProperty( ts, "name", (*it), indent );

		/* tables fields */
		QStringList fields = conn->fields( *it );
		for ( QStringList::Iterator it2 = fields.begin();
		      it2 != fields.end(); ++it2 ) {
		    ts << makeIndent( indent ) << "<field>" << endl;
		    ++indent;
		    saveSingleProperty( ts, "name", (*it2), indent );
		    --indent;
		    ts << makeIndent( indent ) << "</field>" << endl;
		}

		--indent;
		ts << makeIndent( indent ) << "</table>" << endl;
	    }

	    --indent;
	    ts << makeIndent( indent ) << "</connection>" << endl;
	}

	ts << "</DB>" << endl;
	f.close();
    }

    inSaveConnections = FALSE;
#endif
}

#ifndef QT_NO_SQL
static QDomElement loadSingleProperty( QDomElement e, const QString& name )
{
    QDomElement n;
    for ( n = e.firstChild().toElement();
	  !n.isNull();
	  n = n.nextSibling().toElement() ) {
	if ( n.tagName() == "property" && n.toElement().attribute("name") == name )
	    return n;
    }
    return n;
}
#endif

void Project::loadConnections()
{
#ifndef QT_NO_SQL
    if ( !QFile::exists( makeAbsolute( dbFile ) ) )
	return;

    QFile f( makeAbsolute( dbFile ) );
    if ( f.open( IO_ReadOnly ) ) {
	QDomDocument doc;
	if ( doc.setContent( &f ) ) {
	    QDomElement e;
	    e = doc.firstChild().toElement();

	    /* connections */
	    QDomNodeList connections = e.toElement().elementsByTagName( "connection" );
	    for ( uint i = 0; i <  connections.length(); i++ ) {
		QDomElement connection = connections.item(i).toElement();
		QDomElement connectionName = loadSingleProperty( connection, "name" );
		QDomElement connectionDriver = loadSingleProperty( connection, "driver" );
		QDomElement connectionDatabase = loadSingleProperty( connection,
								     "database" );
		QDomElement connectionUsername = loadSingleProperty( connection,
								     "username" );
		QDomElement connectionHostname = loadSingleProperty( connection,
								     "hostname" );
		DatabaseConnection *conn = new DatabaseConnection( this );
		conn->setName( connectionName.firstChild().firstChild().toText().data() );
		conn->setDriver( connectionDriver.firstChild().firstChild().toText().data() );
		conn->setDatabase( connectionDatabase.firstChild().firstChild().toText().data() );
		conn->setUsername( connectionUsername.firstChild().firstChild().toText().data() );
		conn->setHostname( connectionHostname.firstChild().firstChild().toText().data() );

		/* connection tables */
		QDomNodeList tables = connection.toElement().elementsByTagName( "table" );
		for ( uint j = 0; j <  tables.length(); j++ ) {
		    QDomElement table = tables.item(j).toElement();
		    QDomElement tableName = loadSingleProperty( table, "name" );
		    conn->addTable( tableName.firstChild().firstChild().toText().data() );

		    /* table fields */
		    QStringList fieldList;
		    QDomNodeList fields = table.toElement().elementsByTagName( "field" );
		    for ( uint k = 0; k <  fields.length(); k++ ) {
			QDomElement field = fields.item(k).toElement();
			QDomElement fieldName = loadSingleProperty( field, "name" );
			fieldList.append( fieldName.firstChild().firstChild().toText().data() );
		    }
		    conn->setFields( tableName.firstChild().firstChild().toText().data(),
					 fieldList );
		}

		dbConnections.append( conn );
	    }
	}
	f.close();
    }
#endif
}

/*! Opens the database \a connection.  The connection remains open and
can be closed again with closeDatabase().
*/

bool Project::openDatabase( const QString &connection, bool suppressDialog )
{
#ifndef QT_NO_SQL
    DatabaseConnection *conn = databaseConnection( connection );
    if ( connection.isEmpty() && !conn )
	conn = databaseConnection( "(default)" );
    if ( !conn )
	return FALSE;
    bool b = conn->open( suppressDialog );
    return b;
#else
    return FALSE;
#endif
}

/*! Closes the database \a connection.
*/
void Project::closeDatabase( const QString &connection )
{
#ifndef QT_NO_SQL
    DatabaseConnection *conn = databaseConnection( connection );
    if ( connection.isEmpty() && !conn )
	conn = databaseConnection( "(default)" );
    if ( !conn )
	return;
    conn->close();
#endif
}

void Project::formClosed( FormWindow *fw )
{
    formWindows.remove( fw );
}

QObjectList *Project::formList() const
{
    QObjectList *l = new QObjectList;
    for ( QMap<FormWindow*, QString>::ConstIterator it = formWindows.begin(); it != formWindows.end(); ++it )
	l->append( it.key()->child( 0, "QWidget" ) );
    return l;
}

DesignerProject *Project::iFace()
{
    if ( !iface )
	iface = new DesignerProjectImpl( this );
    return iface;
}

void Project::setLanguage( const QString &l )
{
    lang = l;
    updateCustomSettings();
    save();
}

QString Project::language() const
{
    return lang;
}

QString Project::formName( const QString &uifile )
{
    QFile f( makeAbsolute( uifile ) );
    if ( f.open( IO_ReadOnly ) ) {
	QTextStream ts( &f );
	QString line;
	QString className;
	while ( !ts.eof() ) {
	    line = ts.readLine();
	    if ( !className.isEmpty() ) {
		int end = line.find( "</class>" );
		if ( end == -1 ) {
		    className += line;
		} else {
		    className += line.left( end );
		    break;
		}
		continue;
	    }
	    int start;
	    if ( ( start = line.find( "<class>" ) ) != -1 ) {
		int end = line.find( "</class>" );
		if ( end == -1 ) {
		    className = line.mid( start + 7 );
		} else {
		    className = line.mid( start + 7, end - ( start + 7 ) );
		    break;
		}
	    }
	}
	if ( !className.isEmpty() )
	    return className;
    }
    return uifile;
}

void Project::setCustomSetting( const QString &key, const QString &value )
{
    customSettings.remove( key );
    customSettings.insert( key, value );
    save();
}

QString Project::customSetting( const QString &key ) const
{
    return *customSettings.find( key );
}

void Project::updateCustomSettings()
{
    if ( !projectSettingsPluginManager )
	return;
    ProjectSettingsInterface *iface = (ProjectSettingsInterface*)projectSettingsPluginManager->queryInterface( lang );
    if ( !iface )
	return;
    csList = iface->projectSettings();
    customSettings.clear();
    iface->release();
}

void Project::saveImages()
{
}

void Project::loadImages()
{
    pixCollection->load();
}

void Project::setActive( bool b )
{
    pixCollection->setActive( b );
}

void Project::addSourceFile( SourceFile *sf )
{
    sources.append( sf );
    save();
}
