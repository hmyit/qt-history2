#include "qplugin.h"
#include <qapplication.h>
#include <qapplicationinterface.h>

#ifdef _WS_WIN_
#include <qt_windows.h>
#else
#include <dlfcn.h>
#endif

/*!
  \class QPlugInInterface qplugininterface.h

  \brief An abstract class to provide a common interface to functionality a plugin provides.
*/

/*!
  \fn QPlugInInterface::QPlugInInterface()
  
  Creates a QPlugInInterface and initializes the object.
*/

/*!
  \fn QPlugInInterface::~QPlugInInterface()
  
  Destroys the QPlugInInterface and disconnects the plugin from the application.
  Reimplement the destructor in the plugin to provide cleanup for object created 
  in your plugin.

  \sa QCleanUpHandler
*/

/*!
  \fn bool QPlugInInterface::connectNotify( QApplication* theApp )

  This function gets called by the plugin loader as soon as the interface is validated.
  Reimplement this function to provide post constructor initialization.

  If the function returns FALSE, the interface gets deleted immediately. The default
  implementation returns TRUE.
*/

/*!
  \fn bool QPlugInInterface::disconnectNotify()

  This function gets called directly before the interface gets destroyed.
  Reimplement this function to provide pre destructor cleanup, or to prevent
  the unloading of the library.

  If the function returns FALSE, the interface remains undestroyed, and the corresponding
  library is not unloaded.
  The default implementation returns TRUE.
*/

/*!
  \fn QString QPlugInInterface::name()

  Reimplement this function to return the name of the plugin.
  The default implementation returns QString::null.
*/

/*!
  \fn QString QPlugInInterface::description()

  Reimplement this function to return a description of the plugin.
  The default implementation returns QString::null.
*/

/*!
  \fn QString QPlugInInterface::author()

  Reimplement this function to return information about the author of 
  the plugin.
  The default implementation returns QString::null.
*/

/*!
  \fn QStringList QPlugInInterface::featureList()

  Reimplement this function to provide a list of features your plugin
  provides. This function is used by a QPlugInManager to locate the
  plugin that provides a requested feature.

  The default implementation returns an empty list.

  \sa QPlugInManager
*/

/*!
  \fn QCString QPlugInInterface::queryPlugInInterface() const
  \overload

  Returns the name of the plugin's interface. You have to overwrite this 
  method in your subclass for the plugin loader to identify the library.
*/

/*!
  \class QCleanUpHandler

  \brief Provides a save class for memory cleanup.
*/

/*!
  \fn QCleanUpHandler::~QCleanUpHandler()
  
  This destructor will delete all handled objects.
*/

/*!
  \fn void QCleanUpHandler::addCleanUp( Type* object )

  Adds an object to the list that will be destroyed upon
  destruction of the cleanup handler itself.
*/

/*!
  \fn bool QCleanUpHandler::clean()

  Return TRUE if there are any undeleted objects this handler
  has to care about.
  This may be useful for plugins that do not want to be unloaded
  until all memory allocated in the library's scope has been freed.
*/

/*!
  \class QPlugIn qplugin.h

  \brief This class provides a wrapper for library loading and unloading.
*/

/*!
  \enum QPlugIn::LibraryPolicy

  This enum type is used to set and read the plugin's library
  policy.
  Defined values are:
  <ul>
  <li> \c Default - The library get's loaded on first need and never unloaded
  <li> \c OptimizeSpeed - The library is loaded as soon as possible at the cost of memory
  <li> \c Manual - The library has to be loaded and unloaded manually
  </ul>
*/

/*!
  Creates a plugin using the shared library \a filename.

  The library get's loaded immediately if \a pol is OptimizeSpeed,
  as soon as necessary if \a pol is Default, or not automatically
  if \a pol is Manual.

  When the library get's loaded, the function \a fn will be executed to
  retrieve the interface object. If \a fn is not specified, the function
  "loadInterface" will be used.

  \sa setPolicy(), load()
*/
QPlugIn::QPlugIn( const QString& filename, LibraryPolicy pol, const char* fn )
    : pHnd( 0 ), libfile( filename ), libPol( pol )
{
    if ( fn )
	function = fn;
    else
	function = "loadInterface";
    ifc = 0;
    if ( pol == OptimizeSpeed )
	load();
}

/*!
  Deletes the plugin.

  When the library policy is not Manual, the plugin will try to unload the library.

  \sa unload()
*/
QPlugIn::~QPlugIn()
{
    if ( libPol != Manual )
	unload();
}

/*!
  Loads the shared library and initializes the connection to the interface. 
  Returns TRUE if the library was loaded successfully, otherwise does nothing 
  and returns FALSE.

  This function gets called automatically if the policy is not Manual. 
  Otherwise you have to make sure that the library has been loaded before usage.

  \sa setPolicy()
*/
bool QPlugIn::load()
{
    if ( libfile.isEmpty() )
	return FALSE;

    if ( !pHnd ) {
#if defined(_WS_WIN_)
	pHnd = LoadLibraryA( libfile );  //### use LoadLibrary for NT_based systems
#elif defined(_WS_X11_)
	pHnd = dlopen( libfile, RTLD_NOW );
#endif
	if ( !pHnd )
	    return FALSE;
    }

    if ( pHnd )
	return ifc ? TRUE : loadInterface();
    return FALSE;
}

/*!
  Calls the interface's disconnectNotify method and unloads the library.
  When disconnectNotify returns FALSE, the library will not be unloaded
  and the function returns FALSE. Thus the plugin interface can prevent
  the unloading of the library, e.g. when the application still uses
  an object that would be unsafe to delete.
  When disconnectNotify returns TRUE, the interface gets deleted and the 
  library unloaded, and the function returns TRUE.

  \warning
  If \a force is set to TRUE, the library gets unloaded
  at any cost, which is in most cases a segmentation fault,
  so you should know what you're doing!

  \sa load
*/
bool QPlugIn::unload( bool force )
{
    if ( pHnd ) {
	if ( ifc ) {
	    if ( !ifc->disconnectNotify( qApp ) && !force )
		return FALSE;

	    delete ifc;
	    ifc = 0;

#if defined(_WS_WIN_)
	    FreeLibrary( pHnd );
#elif defined(_WS_X11_)
	    dlclose( pHnd );
#endif	
	}
    }
    pHnd = 0;
    return TRUE;
}

/*!
  Sets the current policy to \a pol.
  Forces the library to load if \a pol is set to
  OptimizeSpeed.

  \sa LibraryPolicy
*/
void QPlugIn::setPolicy( LibraryPolicy pol )
{
    libPol = pol;

    if ( libPol == OptimizeSpeed )
	load();
}

/*!
  Returns the current policy.
*/
QPlugIn::LibraryPolicy QPlugIn::policy() const
{
    return libPol;
}

/*!
  Returns the filename of the shared library this QPlugIn object handles.
*/
QString QPlugIn::library() const
{
    return libfile;
}

/*!
  Makes sure the library is loaded unless policy
  is ManualPolicy. Call this method before accessing
  the library functions.

  \sa load, setPolicy
*/
bool QPlugIn::use()
{
    if ( !pHnd ) {
	if ( libPol != Manual )
	    return load();
#ifdef CHECK_RANGE
	else
	    qWarning( "Tried to use library %s without loading!", libfile.latin1() );
#endif
	return FALSE;
    }
    return TRUE;
}

/*!
  Loads the interface of the shared library and calls the connectNotify function.
  Returns TRUE if successful, or FALSE if the interface could not be loaded.

  \sa QApplicationInterface
*/
bool QPlugIn::loadInterface()
{
    if ( !pHnd ) {
	qWarning("QPlugIn::loadInterface(): Failed to load library - no handle!");
	return FALSE;
    }

    LoadInterfaceProc proc;
#ifdef _WS_WIN_
    proc = (LoadInterfaceProc) GetProcAddress( pHnd, function );
#else
    proc = (LoadInterfaceProc) dlsym( pHnd, function );
#endif
    if ( !proc )
	return FALSE;
    ifc = proc();

    if ( !ifc )
	return FALSE;

    if ( ifc->queryPlugInInterface() != queryPlugInInterface() ) {
	delete ifc;
	ifc = 0;
	return FALSE;
    } 
    if ( !ifc->connectNotify( qApp ) ) {
	delete ifc;
	ifc = 0;
	return FALSE;
    }

    return TRUE;
}

/*!
  Calls the library's name() function and returns the result.
*/
QString QPlugIn::name()
{
    if ( !use() )
	return QString::null;

    QString str = plugInterface()->name();

    return str;
}

/*!
  Calls the library's description() function and returns the result.
*/
QString QPlugIn::description()
{
    if ( !use() )
	return QString::null;

    QString str = plugInterface()->description();

    return str;
}

/*!
  Calls the library's author() function and returns the result.
*/
QString QPlugIn::author()
{
    if ( !use() )
	return QString::null;

    QString str = plugInterface()->author();

    return str;
}

/*!
  Calls the library's featureList() function and returns the result.
*/
QStringList QPlugIn::featureList()
{
    if ( !use() )
	return QStringList();

    QStringList list = plugInterface()->featureList();

    return list;
}

/*!
  \class QPlugInManager qpluginmanager.h
  \brief Template class for plugin management.

  The QPlugInManager provides basic support for plugins.
*/

/*!
  \fn QPlugInManager::QPlugInManager( const QString &path, const QString &filter, QPlugIn::LibraryPolicy pol, const char* fn )

  Creates a plugin manager.
  The manager looks up and loads all shared libraries in \a path that match the \a filter. The default policy \a pol and 
  the library function name \a fn will be passed to the QPlugIn constructor.

  \sa QPlugIn::QPlugIn(), addPlugInPath(), addPlugIn()
*/

/*!
  \fn QPlugInManager::~QPlugInManager()
  Deletes the plugin manager.

  Calls the destructor of all managed plugins, too.
*/

/*!
  \fn void QPlugInManager::setDefaultPolicy( QPlugIn::LibraryPolicy pol )

  Sets the current default policy to \a pol, which will be used for all
  libraries that are added to this manager.
  The default policy does not affect plugins already registered to
  the manager.

  \sa QPlugIn::setPolicy
*/

/*!
  \fn QPlugIn::LibraryPolicy QPlugInManager::defaultPolicy() const

  Returns the current default policy.

  \sa setDefaultPolicy
*/

/*!
  \fn void QPlugInManager::setDefaultFunction( const char* fn ) const

  Sets the default function that will be used to look up the QPlugInInterface.
  The default lookup function does not affect plugins already registered to
  the manager.
*/

/*!
  \fn const char* QPlugInManager::defaultFunction() const

  Returns the current default lookup function.

  \sa setDefaultFunction
*/

/*!
  \fn QPlugIn* QPlugInManager::addLibrary( const QString& file )

  Loads the shared library \a file and registers the provided
  features. Returns the QPlugIn created when successful, 
  otherwise null.

  The default library policy and the lookup function name will be 
  passed to the created QPlugIn object.
  
  \sa setDefaultPolicy, setDefaultFunction
*/

/*!
  \fn bool QPlugInManager::removeLibrary( const QString& file )

  Tries to unload the library and removes the library from management. All features
  provided by the specified library will no longer be available using this plugin
  manager, and the corresponding QPlugIn object will be deleted in any case.
  
  Returns TRUE when successful, otherwise returns FALSE.
*/

/*!
  \fn void QPlugInManager::addPlugInPath( const QString& path, const QString& filter )

  Tries to add all shared libraries matching \a filter in \a path.
*/

/*!
  \fn QPlugIn* QPlugInManager::plugIn( const QString &feature )

  Returns the QPlugIn object that provides \a feature, or null if the feature is
  not know to this manager.
*/

/*!
  \fn QPlugIn* QPlugInManager::plugInFromFile( const QString& file )
  
  Returns the QPlugIn object that provides acces to the library \a file, or null
  if the library is not know to this manager.
*/

/*!
  \fn QList<QPlugIn> QPlugInManager::plugInList()

  Returns a list of all QPlugIn object known to this manager.
*/

/*!
  \fn QStringList QPlugInManager::libraryList()

  Returns a list of all library files this manager handles.
*/

/*!
  \fn QStringList QPlugInManager::features()

  Returns a list of all features known to this manager.
  The method will call the features() method for all QPlugIn objects handled by this
  plugin manager and return the results, so calling this function may be quite ineffective. 
  
  Use plugIn() to get access to the plugin object providing the specified feature.

  \sa plugIn, plugInFromFile
*/