/*
** Definitions for the config selection widget
*/

#include <qscrollview.h>
#include <qstringlist.h>

class CConfigView : public QScrollView
{
	Q_OBJECT
public:
	CConfigView( QWidget* pParent = NULL, const char* pName = NULL, WFlags f = 0 );
	~CConfigView();

	QStringList* activeModules();
	QString mapOption( QString strOption );
	enum { NUM_MODULES = 14 };
private:
	static QString m_Modules[ NUM_MODULES ];
	static QString m_ConfigOpts[ NUM_MODULES ];
	QStringList m_activeModules;

public slots:
	void configToggled( const QString& modulename );
};
