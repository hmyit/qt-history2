#include <qapplication.h>
#include <qpainter.h>

#include "qrtstring.h"
#include "qtextlayout.h"

#include <private/qcomplextext_p.h>
#include <qdatetime.h>

#if 1
class MyWidget : public QWidget
{
public:
    MyWidget( QWidget *parent = 0,  const char *name = 0);

    QRTString string;
protected:
    void paintEvent( QPaintEvent *e);

};


MyWidget::MyWidget( QWidget *parent, const char *name )
    : QWidget( parent, name )
{
}


void MyWidget::paintEvent( QPaintEvent * )
{

    QPainter p( this );
	QRTFormat format = string.format(0);
	p.setFont( format.font() );
	p.setPen( format.color() );

#if 1
	ScriptItemArray items;
	TextLayout::instance()->itemize( items, string );
	qDebug("itemization: ");
	for ( int i = 0; i < items.size(); i++ ) {
	    qDebug("    (%d): start: %d, level: %d, script: %d", i, items[i].position, items[i].analysis.bidiLevel,
		   items[i].analysis.script );
	}

	unsigned char levels[256];
	int visualOrder[256];
	for ( int i = 0; i < items.size(); i++ )
	    levels[i] = items[i].analysis.bidiLevel;
	TextLayout::instance()->bidiReorder( items.size(), (unsigned char *)levels, (int *)visualOrder );

	int x = 0;
	int y = 30;
	QString str = string.str();
	for ( int i = 0; i < items.size(); i++ ) {
	    int current = visualOrder[i];
	    const ScriptItem &it = items[ current ];
	    int pos = it.position;
	    int length = (current == items.size() -1 ? str.length() : items[current+1].position ) - pos;
	    p.drawText( x, y, str, pos, length,
			it.analysis.bidiLevel % 2 ? QPainter::RTL : QPainter::LTR );
	    for ( int j = 0; j < length; j++ )
		x += p.fontMetrics().width( str[pos+j] );
	    qDebug("visual %d x=%d length=%d", visualOrder[i], x,  length);
	    y += 20;
	}

#else

    int x = 10, y = 50;
    // just for testing we draw char by char
    QRTFormat oldformat;
    for ( int i = 0; i < string.length(); i++ ) {
	QRTFormat format = string.format(i);
	if ( i == 0 || format != oldformat ) {
	    qDebug("format change at pos %d", i );
	}

	p.setFont( format.font() );
	p.setPen( format.color() );

	p.drawText( x, y, string.str(), i, 1 );
	x += p.fontMetrics().charWidth( string.str(), i );
	oldformat = format;
    }
#endif
}

#endif

//const char * s = "אי U יו";

//const char * s = "אירופה, תוכנה והאינטרנט: Unicode יוצא לשוק העולמי הירשמו כעת לכנס Unicode הבינלאומי העשירי, שייערך בין התאריכים 12־10 במרץ 1997, במיינץ שבגרמניה. בכנס ישתתפו מומחים מכל ענפי התעשייה בנושא האינטרנט העולמי וה־Unicode, בהתאמה לשוק הבינלאומי והמקומי, ביישום Unicode במערכות הפעלה וביישומים, בגופנים, בפריסת טקסט ובמחשוב רב־לשוני. some english inbetween כאשר העולם רוצה לדבר, הוא מדבר ב־Unicode";
//const char * s = "אירופה, תוכנה והאינטרנט: Unicode";


const char *s = "أوروبا, برمجيات الحاسوب + انترنيت : some english تصبح";



int main( int argc, char **argv )
{
    QApplication a(argc, argv);

    MyWidget *w = new MyWidget;
    w->resize( 600,  300 );
    w->show();
    a.setMainWidget ( w );


    {
	QRTString string = QString::fromUtf8( s );
	string.setFormat( QRTFormat( QFont( "Arial", 24 ), Qt::black ) );
	const TextLayout *textLayout = TextLayout::instance();
	w->string = string;
#if 0
	QTime t;
	t.start();
	ScriptItemArray items;
	for ( int i = 0; i < 1000; i++ ) {
	    textLayout->itemize( items, string );
	}
	qDebug("itemize: %dms", t.elapsed() );
	t.start();
	for ( int i = 0; i < 1000; i++ ) {
	    QString str = QComplexText::bidiReorderString( string.str() );
	}
	qDebug("itemize: %dms", t.elapsed() );
#endif
    }

    a.exec();
    delete w;

    qDebug("at exit:");
    QRTFormat::statistics();
}
