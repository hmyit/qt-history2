/****************************************************************
**
** Qt tutorial 5
**
****************************************************************/

#include <qapplication.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qlcdnumber.h>
#include <qfont.h>

#include <qvbox.h>

class MyWidget : public QVBox
{
public:
    MyWidget( QWidget *parent=0, const char *name=0 );
};


MyWidget::MyWidget( QWidget *parent, const char *name )
        : QVBox( parent, name )
{
    QPushButton *quit = new QPushButton( "Quit", this, "quit" );
    quit->setFont( QFont( "Times", 18, QFont::Bold ) );

    connect( quit, SIGNAL(clicked()), qApp, SLOT(quit()) );

    QLCDNumber *lcd  = new QLCDNumber( 2, this, "lcd" );

    QSlider *sBar = new QSlider( 0, 99,		       	// range
			   10,				// page steps
			   0, 				// inital value
			   Horizontal,		 	// orientation
                           this, "slider" );

    connect( sBar, SIGNAL(valueChanged(int)), lcd, SLOT(display(int)) );
}

int main( int argc, char **argv )
{
    QApplication a( argc, argv );

    MyWidget w;
    a.setMainWidget( &w );
    w.show();
    return a.exec();
}
