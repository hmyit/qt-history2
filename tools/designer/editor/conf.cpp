#include "conf.h"
#include <qapplication.h>
#include <qfont.h>
#include <qcolor.h>
#include <qsettings.h>

QMap<QString, ConfigStyle> Config::defaultStyles()
{
    ConfigStyle s;
    QMap<QString, ConfigStyle> styles;
    int normalSize =  qApp->font().pointSize();
    QString normalFamily = qApp->font().family();
    QString commentFamily = "times";
    int normalWeight = qApp->font().weight();

    s.font = QFont( normalFamily, normalSize, normalWeight );
    s.color = Qt::black;
    styles.insert( "Standard", s );

    s.font = QFont( commentFamily, normalSize, normalWeight, TRUE );
    s.color = Qt::red;
    styles.insert( "Comment", s );

    s.font = QFont( normalFamily, normalSize, normalWeight );
    s.color = Qt::blue;
    styles.insert( "Number", s );

    s.font = QFont( normalFamily, normalSize, normalWeight );
    s.color = Qt::darkGreen;
    styles.insert( "String", s );

    s.font = QFont( normalFamily, normalSize, normalWeight );
    s.color = Qt::darkMagenta;
    styles.insert( "Type", s );

    s.font = QFont( normalFamily, normalSize, normalWeight );
    s.color = Qt::darkYellow;
    styles.insert( "Keyword", s );

    s.font = QFont( normalFamily, normalSize, normalWeight );
    s.color = Qt::darkBlue;
    styles.insert( "Preprocessor", s );

    s.font = QFont( normalFamily, normalSize, normalWeight );
    s.color = Qt::darkRed;
    styles.insert( "Label", s );

    return styles;
}

QMap<QString, ConfigStyle> Config::readStyles( const QString &path )
{
    QSettings settings;
    QMap<QString, ConfigStyle> styles;
    styles = defaultStyles();

    QString family;
    int size;
    bool bold, italic, underline;
    int red, green, blue;

    QString elements[] = {
	"Comment",
	"Number",
	"String",
	"Type",
	"Keyword",
	"Preprocessor",
	"Label",
	"Standard",
	QString::null
    };

    for ( int i = 0; elements[ i ] != QString::null; ++i ) {
	bool ok = TRUE;
	while ( 1 ) {
	    family = settings.readEntry( path + elements[ i ] + "/family", &ok );
	    if ( !ok )
		break;
	    size = settings.readNumEntry( path + elements[ i ] + "/size", &ok );
	    if ( !ok )
		break;	
	    bold = settings.readBoolEntry( path + elements[ i ] + "/bold", &ok );
	    if ( !ok )
		break;
	    italic = settings.readBoolEntry( path + elements[ i ] + "/italic", &ok );
	    if ( !ok )
		break;
	    underline = settings.readBoolEntry( path + elements[ i ] + "/underline", &ok );
	    if ( !ok )
		break;
	    red = settings.readNumEntry( path + elements[ i ] + "/red", &ok );
	    if ( !ok )
		break;
	    green = settings.readNumEntry( path + elements[ i ] + "/green", &ok );
	    if ( !ok )
		break;
	    blue = settings.readNumEntry( path + elements[ i ] + "/blue", &ok );
	    if ( !ok )
		break;
	    break;
	}
	if ( !ok )
	    continue;
	QFont f( family );
	f.setPointSize( size );
	f.setBold( bold );
	f.setItalic( italic );
	f.setUnderline( underline );
	QColor c( red, green, blue );
	ConfigStyle s;
	s.font = f;
	s.color = c;
	styles.remove( elements[ i ] );
	styles.insert( elements[ i ], s );
    }
    return styles;
}

void Config::saveStyles( const QMap<QString, ConfigStyle> &styles, const QString &path )
{
    QSettings settings;
    QString elements[] = {
	"Comment",
	"Number",
	"String",
	"Type",
	"Keyword",
	"Preprocessor",
	"Label",
	"Standard",
	QString::null
    };

    for ( int i = 0; elements[ i ] != QString::null; ++i ) {
	settings.writeEntry( path + elements[ i ] + "/family", styles[ elements[ i ] ].font.family() );
	settings.writeEntry( path + elements[ i ] + "/size", styles[ elements[ i ] ].font.pointSize() );
	settings.writeEntry( path + elements[ i ] + "/bold", styles[ elements[ i ] ].font.bold() );
	settings.writeEntry( path + elements[ i ] + "/italic", styles[ elements[ i ] ].font.italic() );
	settings.writeEntry( path + elements[ i ] + "/underline", styles[ elements[ i ] ].font.underline() );
	settings.writeEntry( path + elements[ i ] + "/red", styles[ elements[ i ] ].color.red() );
	settings.writeEntry( path + elements[ i ] + "/green", styles[ elements[ i ] ].color.green() );
	settings.writeEntry( path + elements[ i ] + "/blue", styles[ elements[ i ] ].color.blue() );
    }
    settings.sync();
}

bool Config::completion( const QString &path )
{
    QSettings settings;
    bool ok = FALSE;
    bool ret = settings.readBoolEntry( path + "completion", &ok );
    if ( ok )
	return ret;
    return TRUE;
}

bool Config::wordWrap( const QString &path )
{
    QSettings settings;
    bool ok = FALSE;
    bool ret = settings.readBoolEntry( path + "wordWrap", &ok );
    if ( ok )
	return ret;
    return TRUE;
}

bool Config::parenMatching( const QString &path )
{
    QSettings settings;
    bool ok = FALSE;
    bool ret = settings.readBoolEntry( path + "parenMatching", &ok );
    if ( ok )
	return ret;
    return TRUE;
}

void Config::setCompletion( bool b, const QString &path )
{
    QSettings settings;
    settings.writeEntry( path + "completion", b );
}

void Config::setWordWrap( bool b, const QString &path )
{
    QSettings settings;
    settings.writeEntry( path + "wordWrap", b );
}

void Config::setParenMatching( bool b,const QString &path )
{
    QSettings settings;
    settings.writeEntry( path + "parenMatching", b );
}
