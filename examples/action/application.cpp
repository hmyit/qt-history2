/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of an example program for Qt.
** EDITIONS: NOLIMITS
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "application.h"

#include <qimage.h>
#include <qpixmap.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qpopupmenu.h>
#include <qmenubar.h>
#include <qtextedit.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <qstatusbar.h>
#include <qmessagebox.h>
#include <qprinter.h>
#include <qapplication.h>
#include <qaccel.h>
#include <qtextstream.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>
#include <qwhatsthis.h>
#include <q3action.h>
#include <qsimplerichtext.h>
#include <qevent.h>
#include <qmime.h>
#include <qmimefactory.h>
#include <qtextdocumentfragment.h>
#include <qabstracttextdocumentlayout.h>

#include "filesave.xpm"
#include "fileopen.xpm"
#include "fileprint.xpm"

using namespace Qt;

using namespace Qt;

ApplicationWindow::ApplicationWindow()
    : Q3MainWindow( 0, "example application main window", WDestructiveClose )
{
    // ### change back to highres, when it works
    printer = new QPrinter( QPrinter::ScreenResolution );

    Q3Action * fileNewAction;
    Q3Action * fileOpenAction;
    Q3Action * fileSaveAction, * fileSaveAsAction, * filePrintAction;
    Q3Action * fileCloseAction, * fileQuitAction;

    fileNewAction = new Q3Action( "&New", CTRL+Key_N, this, "new" );
    connect( fileNewAction, SIGNAL( activated() ) , this,
             SLOT( newDoc() ) );

    fileOpenAction = new Q3Action( QPixmap( fileopen ), "&Open...",
                                  CTRL+Key_O, this, "open" );
    connect( fileOpenAction, SIGNAL( activated() ) , this, SLOT( choose() ) );

    const char * fileOpenText = "<p><img source=\"fileopen\"> "
                     "Click this button to open a <em>new file</em>. <br>"
                     "You can also select the <b>Open</b> command "
                     "from the <b>File</b> menu.</p>";
    QMimeSourceFactory::defaultFactory()->setPixmap( "fileopen",
                          fileOpenAction->iconSet().pixmap() );
    fileOpenAction->setWhatsThis( fileOpenText );

    fileSaveAction = new Q3Action( QPixmap( filesave ),
                                  "&Save", CTRL+Key_S, this, "save" );
    connect( fileSaveAction, SIGNAL( activated() ) , this, SLOT( save() ) );

    const char * fileSaveText = "<p>Click this button to save the file you "
                     "are editing. You will be prompted for a file name.\n"
                     "You can also select the <b>Save</b> command "
                     "from the <b>File</b> menu.</p>";
    fileSaveAction->setWhatsThis( fileSaveText );

    fileSaveAsAction = new Q3Action( "Save &As...", 0,  this,
                                    "save as" );
    connect( fileSaveAsAction, SIGNAL( activated() ) , this,
             SLOT( saveAs() ) );
    fileSaveAsAction->setWhatsThis( fileSaveText );

    filePrintAction = new Q3Action( QPixmap( fileprint ), "Print file",
                                   CTRL+Key_P, this, "print" );
    connect( filePrintAction, SIGNAL( activated() ) , this,
             SLOT( print() ) );

    const char * filePrintText = "Click this button to print the file you "
                     "are editing.\n You can also select the Print "
                     "command from the File menu.";
    filePrintAction->setWhatsThis( filePrintText );

    fileCloseAction = new Q3Action( "&Close", CTRL+Key_W, this,
                                   "close" );
    connect( fileCloseAction, SIGNAL( activated() ) , this,
             SLOT( close() ) );

    fileQuitAction = new Q3Action( "&Quit", CTRL+Key_Q, this,
                                  "quit" );
    connect( fileQuitAction, SIGNAL( activated() ) , qApp,
             SLOT( closeAllWindows() ) );

    // populate a tool bar with some actions

    Q3ToolBar * fileTools = new Q3ToolBar( this, "file operations" );
    fileTools->setLabel( "File Operations" );
    fileOpenAction->addTo( fileTools );
    fileSaveAction->addTo( fileTools );
    filePrintAction->addTo( fileTools );
    (void)QWhatsThis::whatsThisButton( fileTools );


    // populate a menu with all actions

    QPopupMenu * file = new QPopupMenu( this );
    menuBar()->insertItem( "&File", file );
    fileNewAction->addTo( file );
    fileOpenAction->addTo( file );
    fileSaveAction->addTo( file );
    fileSaveAsAction->addTo( file );
    file->insertSeparator();
    filePrintAction->addTo( file );
    file->insertSeparator();
    fileCloseAction->addTo( file );
    fileQuitAction->addTo( file );


    menuBar()->insertSeparator();

    // add a help menu

    QPopupMenu * help = new QPopupMenu( this );
    menuBar()->insertItem( "&Help", help );
    help->insertItem( "&About", this, SLOT(about()), Key_F1 );
    help->insertItem( "About &Qt", this, SLOT(aboutQt()) );
    help->insertSeparator();
    help->insertItem( "What's &This", this, SLOT(whatsThis()),
                      SHIFT+Key_F1 );


    // create and define the central widget

    e = new QTextEdit( this, "editor" );
    e->setFocus();
    setCentralWidget( e );
    statusBar()->message( "Ready", 2000 );

    resize( 450, 600 );
}


ApplicationWindow::~ApplicationWindow()
{
    delete printer;
}



void ApplicationWindow::newDoc()
{
    ApplicationWindow *ed = new ApplicationWindow;
    ed->show();
}

void ApplicationWindow::choose()
{
    QString fn = QFileDialog::getOpenFileName( QString::null, QString::null,
					       this);
    if ( !fn.isEmpty() )
	load( fn );
    else
	statusBar()->message( "Loading aborted", 2000 );
}


void ApplicationWindow::load( const QString &fileName )
{
    QFile f( fileName );
    if ( !f.open( IO_ReadOnly ) )
	return;

    QTextStream ts( &f );
    e->setText( ts.read() );
    e->setModified( FALSE );
    setWindowTitle( fileName );
    statusBar()->message( "Loaded document " + fileName, 2000 );
}


void ApplicationWindow::save()
{
    if ( filename.isEmpty() ) {
	saveAs();
	return;
    }

    QString text = e->text();
    QFile f( filename );
    if ( !f.open( IO_WriteOnly ) ) {
	statusBar()->message( QString("Could not write to %1").arg(filename),
			      2000 );
	return;
    }

    QTextStream t( &f );
    t << text;
    f.close();

    e->setModified( FALSE );

    setWindowTitle( filename );

    statusBar()->message( QString( "File %1 saved" ).arg( filename ), 2000 );
}


void ApplicationWindow::saveAs()
{
    QString fn = QFileDialog::getSaveFileName( QString::null, QString::null,
					       this );
    if ( !fn.isEmpty() ) {
	filename = fn;
	save();
    } else {
	statusBar()->message( "Saving aborted", 2000 );
    }
}


void ApplicationWindow::print()
{
    printer->setFullPage( TRUE );
    if ( printer->setup(this) ) {		// printer dialog
	statusBar()->message( "Printing..." );
	QPainter p;
	if( !p.begin( printer ) ) {               // paint on printer
	    statusBar()->message( "Printing aborted", 2000 );
	    return;
	}

	QPaintDeviceMetrics metrics( p.device() );
	int dpiy = metrics.logicalDpiY();
	int margin = (int) ( (2/2.54)*dpiy ); // 2 cm margins
	QRect body( margin, margin, metrics.width() - 2*margin, metrics.height() - 2*margin );

        QTextDocument doc;
        QTextCursor(&doc).insertFragment(QTextDocumentFragment(e->document()));
        QAbstractTextDocumentLayout *layout = doc.documentLayout();
        layout->setPageSize(QSize(body.width(), INT_MAX));

        QRect view(0, 0, body.width(), body.height());
        p.translate(body.left(), body.top());

        QFont font = e->font();
        font.setPointSize(10); // we define 10pt to be a nice base size for printing

	int page = 1;
	do {
            QAbstractTextDocumentLayout::PaintContext ctx;
            ctx.palette = palette();
            p.setClipRect(view);
            layout->draw(&p, ctx);

            p.setClipping(false);
            p.setFont(font);
            p.drawText(view.right() - p.fontMetrics().width(QString::number(page)),
                       view.bottom() + p.fontMetrics().ascent() + 5, QString::number(page));

            view.moveBy(0, body.height());
            p.translate(0, -body.height());

	    if ( view.top() >= layout->sizeUsed().height() )
		break;
	    printer->newPage();
	    page++;
	} while (TRUE);

	statusBar()->message( "Printing completed", 2000 );
    } else {
	statusBar()->message( "Printing aborted", 2000 );
    }
}

void ApplicationWindow::closeEvent( QCloseEvent* ce )
{
    if ( !e->isModified() ) {
	ce->accept();
	return;
    }

    switch( QMessageBox::information( this, "Qt Application Example",
				      "The document has been changed since "
				      "the last save.",
				      "Save Now", "Cancel", "Leave Anyway",
				      0, 1 ) ) {
    case 0:
	save();
	ce->accept();
	break;
    case 1:
    default: // just for sanity
	ce->ignore();
	break;
    case 2:
	ce->accept();
	break;
    }
}


void ApplicationWindow::about()
{
    QMessageBox::about( this, "Qt Application Example",
			"This example demonstrates simple use of "
			"QMainWindow,\nQMenuBar and Q3ToolBar.");
}


void ApplicationWindow::aboutQt()
{
    QMessageBox::aboutQt( this, "Qt Application Example" );
}
