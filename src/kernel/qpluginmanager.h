#ifndef QPLUGINMANAGER_H
#define QPLUGINMANAGER_H

#include <qdict.h>
#include <qdir.h>

template<class Type>
class QPlugInManager
{
public:
    QPlugInManager( const QString& path = QString::null, const QString& filter = "*.dll; *.so", 
	QPlugIn::LibraryPolicy pol = QPlugIn::Default, const char* fn = 0 )
	: defPol( pol )
    {
	defFunction = fn;
	// Every library is unloaded on destruction of the manager
	libDict.setAutoDelete( TRUE );
	plugDict.setAutoDelete( FALSE );
	if ( !path.isEmpty() )
	    addPlugInPath( path, filter );
    }

    virtual ~QPlugInManager()
    {
    }

    virtual void addPlugInPath( const QString& path, const QString& filter = "*.dll; *.so" )
    {
	QStringList plugins = QDir(path).entryList( filter );

	for ( uint p = 0; p < plugins.count(); p++ ) {
	    QString lib = path + "/" + plugins[p];
	    addLibrary( lib );
	}
    }

    Type* addLibrary( const QString& file )
    {
	if ( libDict[file] )
	    return 0;

	Type* plugin = new Type( file, defPol, defFunction );

	bool useful = FALSE;
	QStringList al = ((QPlugIn*)plugin)->featureList();
	for ( QStringList::Iterator a = al.begin(); a != al.end(); a++ ) {
	    useful = TRUE;
#ifdef CHECK_RANGE
	    if ( plugDict[*a] )
		qWarning("%s: Action %s already defined!", plugin->library().latin1(), (*a).latin1() );
	    else
#endif
		plugDict.insert( *a, plugin );
	}

	if ( useful ) {
#ifdef CHECK_RANGE
	    if ( libDict[plugin->library()] )
		qWarning( "QPlugInManager: Can't manage library twice! (%s)", plugin->library().latin1() );
#endif
	    libDict.replace( plugin->library(), plugin );
	} else {
	    delete plugin;
	    return 0;
	}

	return plugin;
    }

    bool removeLibrary( const QString& file )
    {
	Type* plugin = libDict[ file ];
	if ( !plugin )
	    return FALSE;

	{
	    QStringList al = ((QPlugIn*)plugin)->featureList();
	    for ( QStringList::Iterator a = al.begin(); a != al.end(); a++ )
		plugDict.remove( *a );
	}

	if ( !libDict.remove( file ) ) {
	    delete plugin;
	    return FALSE;
	}

	return TRUE;
    }

    void setDefaultPolicy( QPlugIn::LibraryPolicy pol )
    {
	defPol = pol;
    }

    QPlugIn::LibraryPolicy defaultPolicy() const
    {
	return defPol;
    }

    void setDefaultFunction( const char* fn )
    {
	defFunction = fn;
    }

    const char* defaultFunction() const
    {
	return defFunction;
    }

    Type *plugIn( const QString &feature )
    {
	return (Type*)plugDict[feature];
    }

    Type* plugInFromFile( const QString& fileName )
    {
	return libDict[fileName];
    }

    QList<Type> plugInList()
    {
	QList<Type> list;
	QDictIterator<Type> it( libDict );

	while ( it.current() ) {
#ifdef CHECK_RANGE
	    if ( list.containsRef( it.current() ) )
		qWarning("QPlugInManager: Library %s added twice!", it.current()->library().latin1() );
#endif
	    list.append( it.current() );
	    ++it;
	}
	return list;
    }

    QStringList libraryList()
    {
	QStringList list;

	QDictIterator<Type> it( libDict );
	while ( it.current() ) {
	    list << it.currentKey();
	    ++it;
	}

	return list;
    }

    QStringList featureList()
    {
	QStringList list;
	QDictIterator<Type> it (plugDict);

	while( it.current() ) {
	    list << it.currentKey();
	    ++it;
	}

	return list;
    }

    bool selectFeature( const QString& feat )
    {
	Type* plugin = plugIn( feat );
	QDictIterator<Type> it( libDict );

	while ( it.current() ) {
	    if ( it.current() != plugin && it.current()->loaded() )
		it.current()->unload();
	    ++it;
	}

	return plugin != 0;
    }

    void unloadFeature( const QString& feat )
    {
	Type* plugin = plugIn( feat );
	if ( plugin && plugin->loaded() )
	    plugin->unload();
    }

private:
    QDict<Type> plugDict;	    // Dict to match requested feature with plugin
    QDict<Type> libDict;	    // Dict to match library file with plugin

    QPlugIn::LibraryPolicy defPol;
    QString defFunction;
};

#endif //QPLUGINMANAGER_H