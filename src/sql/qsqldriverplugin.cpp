#include "qsqldriverplugin.h"
#include "qsqldriverinterface_p.h"

class QSqlDriverPluginPrivate : public QSqlDriverFactoryInterface
{
public:
    QSqlDriverPluginPrivate( QSqlDriverPlugin *p )
	: plugin( p )
    {
    }
    virtual ~QSqlDriverPluginPrivate();

    QRESULT queryInterface( const QUuid &iid, QUnknownInterface **iface );
    Q_REFCOUNT;

    QStringList featureList() const;
    QSqlDriver *create( const QString &key );

private:
    QSqlDriverPlugin *plugin;
};

QSqlDriverPluginPrivate::~QSqlDriverPluginPrivate()
{
    delete plugin;
}

QRESULT QSqlDriverPluginPrivate::queryInterface( const QUuid &iid, QUnknownInterface **iface )
{
    *iface = 0;

    if ( iid == IID_QUnknown )
	*iface = this;
    else if ( iid == IID_QFeatureList )
	*iface = this;
    else if ( iid == IID_QSqlDriverFactory )
	*iface = this;
    else
	return QE_NOINTERFACE;

    (*iface)->addRef();
    return QS_OK;
}

QStringList QSqlDriverPluginPrivate::featureList() const
{
    return plugin->keys();
}

QSqlDriver *QSqlDriverPluginPrivate::create( const QString &key )
{
    return plugin->create( key );
}


QSqlDriverPlugin::QSqlDriverPlugin()
{
    d = new QSqlDriverPluginPrivate( this );
    _iface = d;
}

QSqlDriverPlugin::~QSqlDriverPlugin()
{
    // don't delete d, as this is deleted by d
}

