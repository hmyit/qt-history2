#include <qstyleinterface.h>
#include <qwindowsstyle.h>

class WindowsStyle : public QStyleInterface, public QLibraryInterface
{
public:
    WindowsStyle();

    QUnknownInterface *queryInterface( const QUuid& );
    unsigned long addRef();
    unsigned long release();

    QStringList featureList() const;
    QStyle *create( const QString& );

    bool canUnload() const;

private:
    unsigned long ref;
};

WindowsStyle::WindowsStyle()
: ref( 0 )
{
}

QUnknownInterface *WindowsStyle::queryInterface( const QUuid &uuid )
{
    QUnknownInterface *iface = 0;
    if ( uuid == IID_QUnknownInterface )
	iface = (QUnknownInterface*)(QStyleInterface*)this;
    else if ( uuid == IID_QStyleInterface )
	iface = (QStyleInterface*)this;
    else if ( uuid == IID_QLibraryInterface )
	iface = (QLibraryInterface*)this;

    if ( iface )
	iface->addRef();
    return iface;
}

unsigned long WindowsStyle::addRef()
{
    return ref++;
}

unsigned long WindowsStyle::release()
{
    if ( !--ref ) {
	delete this;
	return 0;
    }

    return ref;
}

QStringList WindowsStyle::featureList() const
{
    QStringList list;
    list << "Windows";
    return list;
}

QStyle* WindowsStyle::create( const QString& style )
{
    if ( style.lower() == "windows" )
	return new QWindowsStyle();
    return 0;
}

bool WindowsStyle::canUnload() const
{
    return TRUE;
}

Q_EXPORT_INTERFACE()
{
    QUnknownInterface *iface = (QUnknownInterface*)(QStyleInterface*)new WindowsStyle();
    iface->addRef();
    return iface;
}
