/*
** Implementation of the config selection view
*/

#include <qlayout.h>
#include <qcheckbox.h>
#include <qsignalmapper.h>

#include "configview.h"

CConfigView::CConfigView( QWidget* pParent, const char* pName, WFlags w ) : QScrollView( pParent, pName, w )
{
	int i;
	QVBoxLayout* pConfigsLayout;
	QCheckBox* pConfigCheck;
	QSignalMapper* pMapper;
	QFrame* pLine;

	pMapper = new QSignalMapper( this );
	connect( pMapper, SIGNAL( mapped( const QString& ) ), this, SLOT( configToggled( const QString& ) ) );

	pConfigsLayout = new QVBoxLayout( this );

	setMargins( 1, 1, 1, 1 );

/*
** - Create the new checkbox object
** - Add it to the layout manager
** - Add it to the signal mapper
** - Connect the toggle button to the signal mapper
*/
	for( i = 0; i < NUM_MODULES; i++ )
	{
		if ( m_Modules[ i ].length() )
		{
			pConfigCheck = new QCheckBox( m_Modules[ i ], this );
			pConfigsLayout->addWidget( pConfigCheck );
			pMapper->setMapping( pConfigCheck, m_Modules[ i ] );
			connect( pConfigCheck, SIGNAL( clicked() ), pMapper, SLOT( map() ) );
		}
		else
		{
			pLine = new QFrame( this );
			pLine->setFrameShape( QFrame::HLine );
			pLine->setFrameStyle( QFrame::HLine | QFrame::Sunken );
			pConfigsLayout->addWidget( pLine );
		}
	}
	pLine = new QFrame( this );
	pLine->setFrameShape( QFrame::HLine );
	pLine->setFrameStyle( QFrame::HLine | QFrame::Sunken );
	pConfigsLayout->addWidget( pLine );
}

CConfigView::~CConfigView()
{
}

QStringList* CConfigView::activeModules()
{
	return &m_activeModules;
}

QString CConfigView::m_Modules[ NUM_MODULES ] =
{
	"internal", "base", "network", "canvas", "table", "xml", "opengl", "sql",
	"", "gif", "jpeg", "libpng", "mng", "zlib"
};

QString CConfigView::m_ConfigOpts[ NUM_MODULES ] =
{
	"internal", "tools kernel widgets dialogs iconview workspace", "network", "canvas", "table", "xml", "opengl", "sql",
	"", "gif", "jpeg", "png", "mng", "zlib"
};

void CConfigView::configToggled( const QString& moduleName )
{
	/*
	** We receive the module name as parameter.
	** If this already exists in the list of active modules, it should
	** be removed, otherwise, we'll add it.
	*/
	if ( m_activeModules.findIndex( moduleName ) != -1 )
	{
		m_activeModules.remove( moduleName );
	}
	else
	{
		m_activeModules.append( moduleName );
	}
}

QString CConfigView::mapOption( QString strOption )
{
	int i;

	for ( i = 0; i < NUM_MODULES; i++ )
	{
		if ( m_Modules[ i ] == strOption )
		{
			return m_ConfigOpts[ i ];
		}
	}
	return QString( "" );
}