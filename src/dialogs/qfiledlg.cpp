/****************************************************************************
** $Id: //depot/qt/main/src/dialogs/qfiledlg.cpp#68 $
**
** Implementation of QFileDialog class
**
** Created : 950429
**
** Copyright (C) 1995-1997 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#include "qfiledlg.h"
#include "qlined.h"
#include "qcombo.h"
#include "qlabel.h"
#include "qpushbt.h"
#include "qmsgbox.h"
#include "qlistview.h"
#include "qapp.h"
#include "qlayout.h"
#include "qlistview.h"
#include "qpixmap.h"
#include "qpopmenu.h"
#include "qwidgetstack.h"
#include "qbttngrp.h"
#include "qvector.h"
#include "qkeycode.h"

#if defined(_WS_WIN_)
#if defined(_CC_BOOL_DEF_)
#undef	bool
#include <windows.h>
#define bool int
#else
#include <windows.h>
#endif
#endif

RCSTAG("$Id: //depot/qt/main/src/dialogs/qfiledlg.cpp#68 $");


/* Generated by qembed */
static const unsigned int  open_gif_len = 120;
static const unsigned char open_gif_data[] = {
    0x47,0x49,0x46,0x38,0x39,0x61,0x10,0x00,0x0d,0x00,0xc2,0x00,0x00,0x00,
    0x00,0x00,0x99,0x99,0x99,0xcc,0xcc,0xcc,0xff,0xff,0xff,0xff,0xff,0x00,
    0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x21,0xf9,0x04,0x01,0x00,
    0x00,0x03,0x00,0x2c,0x00,0x00,0x00,0x00,0x10,0x00,0x0d,0x00,0x00,0x03,
    0x3d,0x38,0x1a,0xcc,0xfa,0x70,0x84,0x42,0x69,0x88,0x6f,0x12,0xb1,0x49,
    0x69,0xcd,0x52,0x70,0x64,0x55,0x05,0x80,0x54,0x74,0x24,0x4b,0xa0,0x60,
    0x0c,0xbe,0xc0,0x64,0xde,0x8c,0x00,0xaf,0x6d,0x4b,0xd4,0x28,0x95,0xab,
    0x57,0x4b,0x4d,0x7a,0xae,0xa2,0x08,0xc9,0xa1,0xa5,0x24,0x32,0xd9,0x73,
    0x00,0xa8,0x5a,0xaf,0xd6,0x04,0x00,0x3b
};

static const unsigned int  closed_gif_len = 110;
static const unsigned char closed_gif_data[] = {
    0x47,0x49,0x46,0x38,0x39,0x61,0x0f,0x00,0x0d,0x00,0xc2,0x00,0x00,0x00,
    0x00,0x00,0x99,0x99,0x99,0xcc,0xcc,0xcc,0xff,0xff,0xff,0xff,0xff,0x00,
    0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x21,0xf9,0x04,0x01,0x00,
    0x00,0x03,0x00,0x2c,0x00,0x00,0x00,0x00,0x0f,0x00,0x0d,0x00,0x00,0x03,
    0x33,0x38,0x13,0xcc,0xfa,0x30,0x08,0x32,0x45,0x80,0x4f,0xd2,0x6d,0x9b,
    0x0b,0x45,0x28,0x8e,0x61,0x00,0x80,0x5c,0x3a,0x99,0x60,0xa5,0x52,0x6c,
    0xf1,0xba,0xb1,0x6b,0xc3,0xa7,0x7c,0xd3,0xf9,0xce,0xc5,0xb3,0x0d,0xcb,
    0x43,0xf4,0x00,0x06,0x80,0xa4,0x72,0xb9,0x4c,0x00,0x00,0x3b
};


static const unsigned int  cdtoparent_gif_len = 83;
static const unsigned char cdtoparent_gif_data[] = {
    0x47,0x49,0x46,0x38,0x39,0x61,0x0f,0x00,0x0d,0x00,0xa1,0x00,0x00,0x00,
    0x00,0x00,0xff,0xff,0x99,0xcc,0xcc,0xcc,0x00,0x00,0x00,0x21,0xf9,0x04,
    0x01,0x00,0x00,0x02,0x00,0x2c,0x00,0x00,0x00,0x00,0x0f,0x00,0x0d,0x00,
    0x00,0x02,0x24,0x94,0x80,0x68,0xcb,0x10,0xff,0x00,0x5b,0xa9,0xda,0x0a,
    0xb3,0x0e,0x28,0xbb,0xcd,0x7d,0xa1,0xe8,0x7d,0xd6,0xd6,0x41,0x64,0xe9,
    0x81,0x61,0x89,0x45,0xab,0x9b,0xba,0xea,0x85,0x23,0x05,0x00,0x3b
};


static const unsigned int  detailedview_gif_len = 74;
static const unsigned char detailedview_gif_data[] = {
    0x47,0x49,0x46,0x38,0x39,0x61,0x0e,0x00,0x0b,0x00,0xa1,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x99,0xcc,0xcc,0xcc,0x00,0x00,0x00,0x21,0xf9,0x04,
    0x01,0x00,0x00,0x02,0x00,0x2c,0x00,0x00,0x00,0x00,0x0e,0x00,0x0b,0x00,
    0x00,0x02,0x1b,0x14,0x0e,0x76,0x9a,0xe2,0x0f,0x61,0x98,0xb4,0xd6,0x88,
    0x21,0x4a,0x88,0xe5,0xcf,0x70,0x4b,0x07,0x62,0x9b,0x47,0x96,0x5a,0xc7,
    0x2e,0x05,0x00,0x3b
};


static const unsigned int  mclistview_gif_len = 84;
static const unsigned char mclistview_gif_data[] = {
    0x47,0x49,0x46,0x38,0x39,0x61,0x0f,0x00,0x0b,0x00,0xa1,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x99,0xcc,0xcc,0xcc,0xff,0xff,0xff,0x21,0xf9,0x04,
    0x01,0x00,0x00,0x02,0x00,0x2c,0x00,0x00,0x00,0x00,0x0f,0x00,0x0b,0x00,
    0x00,0x02,0x25,0x8c,0x85,0x68,0x91,0x12,0xc3,0x80,0x74,0x50,0x48,0xb0,
    0x9a,0x3b,0xba,0xfb,0xdc,0x80,0x4a,0x75,0x51,0x91,0x24,0x2a,0x9c,0xc7,
    0xaa,0x4c,0xb8,0x9a,0xd6,0xf4,0x9c,0x58,0xbc,0xbd,0x48,0x01,0x00,0x3b
};


static QPixmap * openFolderIcon = 0;
static QPixmap * closedFolderIcon = 0;
static QPixmap * detailViewIcon = 0;
static QPixmap * multiColumnListViewIcon = 0;
static QPixmap * cdToParentIcon = 0;


static void makeIcons()
{
    if ( !openFolderIcon ) {
	openFolderIcon = new QPixmap();
	openFolderIcon->loadFromData( open_gif_data, open_gif_len );
	closedFolderIcon = new QPixmap();
	closedFolderIcon->loadFromData( closed_gif_data, closed_gif_len );
	detailViewIcon = new QPixmap();
	detailViewIcon->loadFromData( detailedview_gif_data,
				     detailedview_gif_len );
	multiColumnListViewIcon = new QPixmap();
	multiColumnListViewIcon->loadFromData( mclistview_gif_data,
					       mclistview_gif_len );
	cdToParentIcon = new QPixmap();
	cdToParentIcon->loadFromData( cdtoparent_gif_data,
				      cdtoparent_gif_len );
    }
}


struct QFileDialogPrivate {
    bool geometryDirty;
    QComboBox * paths;
    QComboBox * types;
    QLabel * pathL;
    QLabel * fileL;
    QLabel * typeL;

    QVBoxLayout * topLevelLayout;

    QWidgetStack * stack;

    QPushButton * cdToParent, * detailView, * mcView;
    QButtonGroup * modeButtons;

    QString currentFileName;

    struct File: public QListViewItem {
	File( const QFileInfo * fi, QListViewItem * parent, int h )
	    : QListViewItem( parent ), info( *fi ) { setHeight( h ); }
	File( const QFileInfo * fi, QListView * parent, int h  )
	    : QListViewItem( parent ), info( *fi ) { setHeight( h ); }

	const char * text( int column ) const;
	const char * key( int column ) const;

	QFileInfo info;
    };

    class MCList: public QTableView {
    public:
	MCList( QListView *, QWidget * );
	~MCList();
	void paintCell( QPainter *, int row, int col );

	void clear();
    protected:
	void mousePressEvent( QMouseEvent * );
	void mouseMoveEvent( QMouseEvent * );
	void mouseReleaseEvent( QMouseEvent * );
	void paintEvent( QPaintEvent * );
	void resizeEvent( QResizeEvent * );
	void keyPressEvent( QKeyEvent * );
	int cellWidth ( int );
    private:
	void setFocusToPoint( const QPoint & );
	void setUpContents();
	void updateItem( QListViewItem * );
	QListView * lv;
	QVector<QListViewItem> * items;
	QArray<int> * widths;
    };

    MCList * moreFiles;

    QFileDialog::Mode mode;
};


const char * QFileDialogPrivate::File::text( int column ) const
{
    static QString r;

    switch( column ) {
    case 0:
	r = info.fileName();
	break;
    case 1:
	r.sprintf( "%d", info.size() );
	break;
    case 2:
	if ( info.isFile() )
	    r = "File";
	else if ( info.isDir() )
	    r = "Directory";
	else
	    r = "Special File";
	if ( info.isSymLink() )
	    r.prepend( "Link to " );
	break;
    case 3:
	r = info.lastModified().toString();
	break;
    case 4:
	r = "ashr";
	break;
    default:
	r = "<--->";
    }

    r.detach();
    return r;
}


const char * QFileDialogPrivate::File::key( int column ) const
{
    static QString r;
    static QDateTime epoch( QDate( 1968, 6, 19 ) );

    if ( column == 1 ) {
	r.sprintf( "%08d", info.size() );
	return r;
    } else if ( column == 3 ) {
	r.sprintf( "%08d", epoch.secsTo( info.lastModified() ) );
	return r;
    } else {
	return text( column );
    }
}



QFileDialogPrivate::MCList::MCList( QListView * files, QWidget * parent )
    : QTableView( parent, "multi-column list box" )
{
    lv = files;
    items = 0;
    widths = 0;
    setCellHeight( fontMetrics().lineSpacing() + 1 );
    setCellWidth( 0 );
    setBackgroundMode( PaletteBase ); // NoBackground for really cool bugs
}



QFileDialogPrivate::MCList::~MCList()
{
    delete items;
    delete widths;
}


int QFileDialogPrivate::MCList::cellWidth ( int c )
{
    if ( !widths )
	setUpContents();
    if ( !widths || c >= (int)(widths->size()) )
	return 25;
    return (*widths)[c];
}


void QFileDialogPrivate::MCList::paintCell( QPainter *p, int row, int col )
{
    if ( (uint)(col * numRows() + row) >= items->count() )
	return;

    const QListViewItem * file = (*items)[col*numRows() + row];
    if ( file ) {
	file->paintCell( p, colorGroup(), 0, cellWidth(col) );
	if ( lv->currentItem() == file )
	    file->paintFocus( p, colorGroup(),
			      QRect( 0, 0,
				     cellWidth( col ), cellHeight( row ) ) );
    }
}


void QFileDialogPrivate::MCList::clear()
{
    delete items;
    items = 0;
}

void QFileDialogPrivate::MCList::setFocusToPoint( const QPoint & p )
{
    if ( !items )
	setUpContents();

    int row = findRow( p.y() ), col = findCol( p.x() );
    if ( row < 0 || col < 0 )
	return;

    int i = col * numRows() + row;
    if ( i >= (int)items->count() )
	return;

    const QListViewItem * file = (*items)[col*numRows() + row];
    if ( !file )
	return;

    QListViewItem * prev = lv->currentItem();
    if ( prev != file ) {
	lv->setSelected( (QListViewItem *)file, TRUE );
	lv->setCurrentItem( (QListViewItem *)file );
	updateCell( row, col );
	updateItem( prev );
    }
}


void QFileDialogPrivate::MCList::setUpContents()
{
    bool oldAutoUpdate = autoUpdate();
    setAutoUpdate( FALSE );
    const QListViewItem * file;
    if ( !items ) {
	setNumRows( QMAX( 1, (height()+2) / cellHeight() ) );

	file = lv->firstChild();
	int i = 0;
	int w, maxw, totalw;
	maxw = 40;
	totalw = 0;
	
	int count = 0;
	while( file ) {
	    file = file->nextSibling();
	    count++;
	}
	
	file = lv->firstChild();

	delete widths;
	widths = new QArray<int>( (count+numRows()-1)/numRows() );

	while( file ) {
	    if ( file->height() ) {
		w = fontMetrics().width( file->text( 0 ) ) + 20;
		if ( w > maxw )
		    maxw = w;
		file = file->nextSibling();
		if ( (i+1) % numRows() == 0 || !file ) {
		    (*widths)[ i / numRows() ] = maxw;
		    totalw += maxw;
		    maxw = 40;
		}
		i++;
	    } else {
		file = file->nextSibling();
	    }
	}

	if ( totalw > width() ) {
	    i = numRows();
	    setNumRows( (height() + 2 - horizontalScrollBar()->height())
			/ cellHeight() );
	    if ( i != numRows() ) {
		// ### uglehack! repeated code.
		file = lv->firstChild();
		i = 0;
		maxw = 40;
		totalw = 0;

		delete widths;
		widths = new QArray<int>( (count+numRows()-1)/numRows() );

		while( file ) {
		    if ( file->height() ) {
			w = fontMetrics().width( file->text( 0 ) ) + 20;
			if ( w > maxw )
			    maxw = w;
			file = file->nextSibling();
			if ( (i+1) % numRows() == 0 || !file ) {
			    (*widths)[ i / numRows() ] = maxw;
			    totalw += maxw;
			    maxw = 40;
			}
			i++;
		    } else {
			file = file->nextSibling();
		    }
		}
	    }
	    setTableFlags( Tbl_hScrollBar
			   + Tbl_snapToHGrid + Tbl_clipCellPainting
			   + Tbl_cutCellsV + Tbl_smoothHScrolling );
	} else {
	    setTableFlags( Tbl_snapToHGrid + Tbl_clipCellPainting
			   + Tbl_cutCellsV );
	}
	setNumCols( widths->size() );

	items = new QVector<QListViewItem>( i );
	i = 0;
	file = lv->firstChild();
	// may have the wrong order.  fix that later.
	while( file ) {
	    if ( file->height() )
		items->insert( i++, file );
	    file = file->nextSibling();
	}
    }
    setAutoUpdate( oldAutoUpdate );
}


void QFileDialogPrivate::MCList::updateItem( QListViewItem * file )
{
    setUpContents();
    int i = items->count() -1 ;
    while( i >= 0 ) {
	if ( (*items)[i] == file ) {
	    // and all this because it's too much work to
	    // add a proper QListBox::setMultiColumn()? hm...

	    int col, row;
	    col = i / numRows();
	    row = i - ( col * numRows() );
	    updateCell( row, col );
	    return;
	}
	i--;
    }
}


void QFileDialogPrivate::MCList::mousePressEvent( QMouseEvent * e )
{
    if ( e )
	setFocusToPoint( e->pos() );
}


void QFileDialogPrivate::MCList::mouseMoveEvent( QMouseEvent * e )
{
    if ( e )
	setFocusToPoint( e->pos() );
}


void QFileDialogPrivate::MCList::mouseReleaseEvent( QMouseEvent * e )
{
    if ( e )
	setFocusToPoint( e->pos() );
}


void QFileDialogPrivate::MCList::paintEvent( QPaintEvent * e )
{
    if ( !items )
	setUpContents();
    QTableView::paintEvent( e );
}


void QFileDialogPrivate::MCList::resizeEvent( QResizeEvent * e )
{
    clear();
    setNumRows( (height()+2) / cellHeight() );
    QTableView::resizeEvent( e );
}


void QFileDialogPrivate::MCList::keyPressEvent( QKeyEvent * e )
{
    if ( !e )
	return;

    int col=0, row=0;
    setUpContents();
    int i = items->count() -1 ;
    while( i >= 0 ) {
	if ( (*items)[i] == lv->currentItem() ) {
	    col = i / numRows();
	    row = i - ( col * numRows() );
	    break;
	}
	i--;
    }

    switch( e->key() ) {
    case Key_Down:
	e->accept();
	if ( row < numRows() - 1 )
	    row++;
	break;
    case Key_Up:
	e->accept();
	if ( row )
	    row--;
	break;
    case Key_Left:
	e->accept();
	if ( col )
	    col--;
	break;
    case Key_Right:
	e->accept();
	if ( col < numCols() - 1 )
	    col++;
	break;
    case Key_Enter:
	QApplication::sendEvent( lv, e );
	clear();
	update();
	return;
    default:
	if ( e->ascii() ) {
	    QListViewItem * oldFile = lv->currentItem();
	    QApplication::sendEvent( lv, e );
	    QListViewItem * newFile = lv->currentItem();
	    if ( oldFile != newFile ) {
		updateItem( oldFile );
		updateItem( newFile );
	    }
	}
	return;
    }

    if ( col * numRows() + row >= (int)(items->count()) )
	return;

    QListViewItem * prev = lv->currentItem();
    QListViewItem * file = (*items)[col * numRows() + row];
    if ( prev != file ) {
	lv->setSelected( file, TRUE );
	lv->setCurrentItem( file );
	updateCell( row, col );
	updateItem( prev );
    }
}


/*!
  \class QFileDialog qfiledlg.h
  \brief The QFileDialog provides a dialog widget for inputting file names.

  Example:
  \code
    QString fileName = QFileDialog::getOpenFileName();
    if ( !fileName.isNull() ) {			// got a file name
	...
    }
  \endcode

  There are two ready-made convenience functions, getOpenFileName()
  and getSaveFileName(), which may be used like this:

  \code
    QString s( QFileDialog::getOpenFileName() );
    if ( s.isNull() )
	return;

    open( s ); // open() being your function to read the file
  \endcode

  <img src=qfiledlg-m.gif> <img src=qfiledlg-w.gif>

  http://www.iarchitect.com/file95.htm

  \sa QPrintDialog
*/


/*!
  Constructs a file dialog with a \e parent, \e name and \e modal flag.

  The dialog becomes modal if \e modal is TRUE, otherwise modeless.
*/

QFileDialog::QFileDialog( QWidget *parent, const char *name, bool modal )
    : QDialog( parent, name, modal )
{
    init();
    cwd.convertToAbs();
    rereadDir();
    QSize s( files->sizeHint() );
    s = QSize( s.width() + 50, s.height() + 80 );
    if ( s.width() * 3 > QApplication::desktop()->width() * 2 )
	s.setWidth( QApplication::desktop()->width() * 2 / 3 );
    if ( s.height() * 3 > QApplication::desktop()->height() * 2 )
	s.setHeight( QApplication::desktop()->height() * 2 / 3 );
    else if ( s.height() * 2 < QApplication::desktop()->height() )
	s.setHeight( QApplication::desktop()->height() / 2 );
    resize( s );
}


/*!
  Constructs a file dialog with a \e parent, \e name and \e modal flag.

  The dialog becomes modal if \e modal is TRUE, otherwise modeless.
*/

QFileDialog::QFileDialog( const char *dirName, const char *filter,
			  QWidget *parent, const char *name, bool modal )
    : QDialog( parent, name, modal )
{
    init();
    if ( filter ) {
	cwd.setNameFilter( filter );
	d->types->insertItem( filter );
    } else {
	d->types->insertItem( tr( "All files (*)" ) );
    }
    if ( dirName )
	cwd.setPath( dirName );
    cwd.convertToAbs();
    rereadDir();
    QSize s( files->sizeHint() );
    s = QSize( s.width() + 50, s.height() + 80 );
    if ( s.width() * 3 > QApplication::desktop()->width() * 2 )
	s.setWidth( QApplication::desktop()->width() * 2 / 3 );
    if ( s.height() * 3 > QApplication::desktop()->height() * 2 )
	s.setHeight( QApplication::desktop()->height() * 2 / 3 );
    else if ( s.height() * 2 < QApplication::desktop()->height() )
	s.setHeight( QApplication::desktop()->height() / 2 );
    resize( s );
}


/*!
  \internal
  Initializes the file dialog.
*/

void QFileDialog::init()
{
    d = new QFileDialogPrivate();
    d->mode = AnyFile;

    nameEdit = new QLineEdit( this, "name/filter editor" );
    connect( nameEdit, SIGNAL(returnPressed()),
	     this,  SLOT(fileNameEditDone()) );

    d->stack = new QWidgetStack( this, "files and more files" );
    d->stack->setFrameStyle( QFrame::WinPanel + QFrame::Sunken );
			
    files = new QListView( d->stack, "current directory listing" );
    QFontMetrics fm( fontMetrics() );
    files->setColumn( tr("Name"), 150 );
    files->setColumn( tr("Size"), 30 + fm.width( tr("Size") ) );
    files->setColumn( tr("Type"), 10 + fm.width( tr("Directory") ) );
    files->setColumn( tr("Date"), 150 );
    files->setColumn( tr("Attributes"), 10 + fm.width( tr("Attributes") ) );

    files->setMinimumSize( 50, 25 + 2*fontMetrics().lineSpacing() );

    connect( files, SIGNAL(currentChanged(QListViewItem *)),
	     this, SLOT(updateFileNameEdit(QListViewItem *)) );
    connect( files, SIGNAL(doubleClicked(QListViewItem *)),
	     this, SLOT(selectDirectoryOrFile(QListViewItem *)) );
    connect( files, SIGNAL(returnPressed(QListViewItem *)),
	     this, SLOT(selectDirectoryOrFile(QListViewItem *)) );
    connect( files, SIGNAL(rightButtonClicked(QListViewItem *,
					      const QPoint &, int)),
	     this, SLOT(popupContextMenu(QListViewItem *,
					 const QPoint &, int)) );
    files->setFocusPolicy( StrongFocus );

    d->moreFiles = new QFileDialogPrivate::MCList( files, d->stack );
    d->moreFiles->setFrameStyle( QFrame::NoFrame );
    d->moreFiles->setFocusPolicy( StrongFocus );

    okB = new QPushButton( tr("OK"), this, "OK" );
    okB->setAutoDefault( TRUE );
    okB->setDefault( TRUE );
    connect( okB, SIGNAL(clicked()), this, SLOT(okClicked()) );
    cancelB = new QPushButton( tr("Cancel") , this, "Cancel" );
    cancelB->setAutoDefault( TRUE );
    connect( cancelB, SIGNAL(clicked()), this, SLOT(cancelClicked()) );

    d->paths = new QComboBox( TRUE, this, "directory history/editor" );
    connect( d->paths, SIGNAL(activated(const char *)),
	     this, SLOT(setDir(const char *)) );
    d->geometryDirty = TRUE;
    d->types = new QComboBox( TRUE, this, "file types" );

    d->pathL = new QLabel( d->paths, tr("Look &in"), this );
    d->fileL = new QLabel( nameEdit, tr("File &name"), this );
    d->typeL = new QLabel( d->types, tr("File &type"), this );

    makeIcons();

    d->cdToParent = new QPushButton( this, "cd to parent" );
    d->cdToParent->setPixmap( *cdToParentIcon );
    connect( d->cdToParent, SIGNAL(clicked()),
	     this, SLOT(cdUpClicked()) );

    d->modeButtons = new QButtonGroup( 0, "invisible group" );
    d->modeButtons->setExclusive( TRUE );
    connect( d->modeButtons, SIGNAL(clicked(int)),
	     d->stack, SLOT(raiseWidget(int)) );

    d->detailView = new QPushButton( this, "list view" );
    d->detailView->setPixmap( *detailViewIcon );
    d->detailView->setToggleButton( TRUE );
    d->stack->addWidget( files, d->modeButtons->insert( d->detailView ) );
    d->mcView = new QPushButton( this, "mclistbox view" );
    d->mcView->setPixmap( *multiColumnListViewIcon );
    d->mcView->setToggleButton( TRUE );
    d->stack->addWidget( d->moreFiles, d->modeButtons->insert( d->mcView ) );

    d->stack->raiseWidget( files );
    d->detailView->setOn( TRUE );

    d->topLevelLayout = new QVBoxLayout( this, 6 );

    QHBoxLayout * h;

    h = new QHBoxLayout( 0 );
    d->topLevelLayout->addLayout( h );
    h->addWidget( d->pathL );
    h->addSpacing( 6 );
    h->addWidget( d->paths );
    h->addSpacing( 6 );
    h->addWidget( d->cdToParent );
    h->addSpacing( 6 );
    h->addWidget( d->detailView );
    h->addWidget( d->mcView );

    d->topLevelLayout->addWidget( d->stack, 3 );

    h = new QHBoxLayout();
    d->topLevelLayout->addLayout( h );
    h->addWidget( d->fileL );
    h->addWidget( nameEdit );
    h->addWidget( okB );

    h = new QHBoxLayout();
    d->topLevelLayout->addLayout( h );
    h->addWidget( d->typeL );
    h->addWidget( d->types );
    h->addWidget( cancelB );

    cwd.setMatchAllDirs( TRUE );
    cwd.setSorting( cwd.sorting() | QDir::DirsFirst );

    updateGeometry();

    d->cdToParent->setFocusPolicy( NoFocus );
    d->detailView->setFocusPolicy( NoFocus );
    d->mcView->setFocusPolicy( NoFocus );

    setTabOrder( d->paths, files );
    setTabOrder( files, nameEdit );
    setTabOrder( nameEdit, d->types );
    setTabOrder( d->types, okB );
    setTabOrder( okB, cancelB );

    nameEdit->setFocus();
}

/*!
  Destroys the file dialog.
*/

QFileDialog::~QFileDialog()
{
    delete d->modeButtons;
    delete d;
}


/*!
  Returns the selected file name.

  If a file name was selected, the returned string contains the
  absolute path name.  The returned string is a null string if no file
  name was selected.

  \sa QString::isNull()
*/

QString QFileDialog::selectedFile() const
{
    return d->currentFileName;
}


/*!
  Returns the active directory path string in the file dialog.
  \sa dir(), setDir()
*/

const char *QFileDialog::dirPath() const
{
    return cwd.path();
}

/*!
  Sets a directory path string for the file dialog.
  \sa dir()
*/

void QFileDialog::setDir( const char *pathstr )
{
    if ( strcmp(cwd.path(),pathstr) == 0 )
	return;
    cwd.setPath( pathstr );
    cwd.convertToAbs();
    rereadDir();
}

/*!
  Returns the active directory in the file dialog.
  \sa setDir()
*/

const QDir *QFileDialog::dir() const
{
    return &cwd;
}

/*!
  Sets a directory path for the file dialog.
  \sa dir()
*/

void QFileDialog::setDir( const QDir &dir )
{
    cwd = dir;
    cwd.convertToAbs();
    cwd.setMatchAllDirs( TRUE );
    cwd.setSorting( cwd.sorting() | QDir::DirsFirst );
    rereadDir();
}


/*!
  Re-reads the active directory in the file dialog.

  It is seldom necessary to call this function.	 It is provided in
  case the directory contents change and you want to refresh the
  directory list box.
*/

void QFileDialog::rereadDir()
{
    if ( d ) {
	QString cp( cwd.canonicalPath() );
	int i = d->paths->count()-1;
	while( i >= 0 && strcmp( d->paths->text( i ), cp ) )
	    i--;
	if ( i >= 0) {
	    d->paths->setCurrentItem( i );
	} else {
	    d->paths->insertItem( cwd.canonicalPath() );
	    d->paths->setCurrentItem( d->paths->count() - 1 );
	}
    }

    const QFileInfoList *filist = 0;

    int itemHeight = fontMetrics().height() + 6;

    while ( !filist ) {
	filist = cwd.entryInfoList();
	if ( !filist &&
	     QMessageBox::warning( this, tr("Open File"),
				   QString( tr("Unable to read directory\n") )
				   + cwd.absPath() + "\n\n" +
				   tr("Please make sure that the directory\n"
				      "in readable.\n"),
				   tr("Use Parent Directory"),
				   tr("Use Old Contents"), 0 ) ) {
	    return;
	}
	if ( !filist ) {
	    // change to parent, reread
	    // ...

	    // but for now
	    return;
	}
    }

    files->clear();

    QFileInfoListIterator it( *filist );
    QFileInfo *fi;
    while ( (fi = it.current()) != 0 ) {
	++it;
	(void) new QFileDialogPrivate::File( fi, files, itemHeight );
    }
    d->moreFiles->clear();
    d->moreFiles->repaint();
}



/*!
  \fn void QFileDialog::fileHighlighted( const char * )

  This signal is emitted when the user highlights a file.
*/

/*!
  \fn void QFileDialog::fileSelected( const char * )

  This signal is emitted when the user selects a file.
*/

/*!
  \fn void QFileDialog::dirEntered( const char * )

  This signal is emitted when the user has selected a new directory.
*/

static QString filedlg_dir;


/*!
  Opens a modal file dialog and returns the name of the file to be opened.
  Returns a \link QString::isNull() null string\endlink if the user cancelled
  the dialog.

  This static function is less capable than the full QFileDialog object,
  but is convenient and easy to use.

  Example:
  \code
    // start at the current working directory and with *.cpp as filter
    QString f = QFileDialog::getOpenFileName( 0, "*.cpp", this );
    if ( !f.isEmpty() ) {
        // the user selected a valid existing file
    } else {
        // the user cancelled the dialog
    }
  \endcode

  getSaveFileName() is another convenience function, equal to this one
  except that it allows the user to specify the name of a nonexistent file
  name.

  \sa getSaveFileName()
*/

QString QFileDialog::getOpenFileName( const char *dirName, const char *filter,
				      QWidget *parent, const char *name )
{
    if ( dirName && *dirName ) {
	filedlg_dir = dirName;
    } else if ( filedlg_dir.isNull() ) {
	filedlg_dir = QDir::currentDirPath();
    }

#if defined(_WS_WIN_)

    filedlg_dir = QDir::convertSeparators( filedlg_dir );

    const int maxstrlen = 256;
    char *file = new char[maxstrlen];
    file[0] = '\0';

    const char all_filter[] = "All Files\0*.*\0";
    const int all_len = sizeof(all_filter); // 15
    char* win_filter;
    int total_len = 0;
    if (filter) {
	int fl = strlen(filter)+1; // Include nul
	win_filter = new char[2*fl+all_len];
	for (int i=0; i<2; i++) {
	    memcpy(win_filter+total_len, filter, fl);
	    total_len += fl;
	}
    } else {
	win_filter = new char[all_len];
    }
    memcpy(win_filter+total_len, all_filter, all_len);

    OPENFILENAME ofn;
    memset( &ofn, 0, sizeof(OPENFILENAME) );
    ofn.lStructSize	= sizeof(OPENFILENAME);
    ofn.hwndOwner	= parent ? parent->topLevelWidget()->winId() : 0;
    ofn.lpstrFilter	= win_filter;
    ofn.lpstrFile	= file;
    ofn.nMaxFile	= maxstrlen;
    ofn.lpstrInitialDir = filedlg_dir;
    ofn.lpstrTitle	= "Open";
    ofn.Flags		= (OFN_CREATEPROMPT|OFN_NOCHANGEDIR);

    QString result;
    if ( GetOpenFileName(&ofn) ) {
	result = file;
	filedlg_dir = QFileInfo(file).dirPath();
    }

    delete [] win_filter;
    delete [] file;
    return result;

#else

    QFileDialog *dlg = new QFileDialog( filedlg_dir, filter, parent, name, TRUE );
    CHECK_PTR( dlg );
    dlg->setCaption( "Open" );
    QString result;
    if ( dlg->exec() == QDialog::Accepted ) {
	result = dlg->selectedFile();
	filedlg_dir = dlg->dirPath();
    }
    delete dlg;
    return result;

#endif
}

/*!
  Opens a modal file dialog and returns the name of the file to be saved.
  Returns a \link QString::isNull() null string\endlink if the user cancelled
  the dialog.

  This static function is less capable than the full QFileDialog object,
  but is convenient and easy to use.

  Example:
  \code
    // start at the current working directory and with *.cpp as filter
    QString f = QFileDialog::getSaveFileName( 0, "*.cpp", this );
    if ( !f.isEmpty() ) {
        // the user gave a file name
    } else {
        // the user cancelled the dialog
    }
  \endcode

  getOpenFileName() is another convenience function, equal to this one
  except that it allows the user to specify the name of a nonexistent file
  name.

  \sa getOpenFileName()
*/

QString QFileDialog::getSaveFileName( const char *dirName, const char *filter,
				      QWidget *parent, const char *name )
{
    if ( dirName && *dirName ) {
	filedlg_dir = dirName;
    } else if ( filedlg_dir.isNull() ) {
	filedlg_dir = QDir::currentDirPath();
    }

#if defined(_WS_WIN_)

    filedlg_dir = QDir::convertSeparators( filedlg_dir );

    const int maxstrlen = 256;
    char *file = new char[maxstrlen];
    file[0] = '\0';

    const char all_filter[] = "All Files\0*.*\0";
    const int all_len = sizeof(all_filter); // 15
    char* win_filter;
    int total_len = 0;
    if (filter) {
	int fl = strlen(filter)+1; // Include nul
	win_filter = new char[2*fl+all_len];
	for (int i=0; i<2; i++) {
	    memcpy(win_filter+total_len, filter, fl);
	    total_len += fl;
	}
    } else {
	win_filter = new char[all_len];
    }
    memcpy(win_filter+total_len, all_filter, all_len);

    OPENFILENAME ofn;
    memset( &ofn, 0, sizeof(OPENFILENAME) );
    ofn.lStructSize	= sizeof(OPENFILENAME);
    ofn.hwndOwner	= parent ? parent->topLevelWidget()->winId() : 0;
    ofn.lpstrFilter	= win_filter;
    ofn.lpstrFile	= file;
    ofn.nMaxFile	= maxstrlen;
    ofn.lpstrInitialDir = filedlg_dir;
    ofn.lpstrTitle	= "Save";
    ofn.Flags		= (OFN_CREATEPROMPT|OFN_NOCHANGEDIR);

    QString result;
    if ( GetSaveFileName(&ofn) ) {
	result = file;
	filedlg_dir = QFileInfo(file).dirPath();
    }

    delete [] win_filter;
    delete [] file;
    return result;

#else

    QFileDialog *dlg = new QFileDialog( filedlg_dir, filter, parent, name, TRUE );
    CHECK_PTR( dlg );
    dlg->setCaption( "Save As" );
    QString result;
    if ( dlg->exec() == QDialog::Accepted ) {
	result = dlg->selectedFile();
	filedlg_dir = dlg->dirPath();
    }
    delete dlg;
    return result;

#endif
}


/*!
  \internal
  Activated when the "Ok" button is clicked.
*/

void QFileDialog::okClicked()
{
    if ( strcmp( nameEdit->text(), "") != 0 ) {
	emit fileSelected( d->currentFileName );
	accept();
    }
}

/*!
  \internal
  Activated when the "Filter" button is clicked.
*/

void QFileDialog::filterClicked()
{
    // unused
}

/*!
  \internal
  Activated when the "Cancel" button is clicked.
*/

void QFileDialog::cancelClicked()
{
    reject();
}


/*!
  Handles resize events for the file dialog.
*/

void QFileDialog::resizeEvent( QResizeEvent * )
{
    updateGeometry();
}

/*! \internal

  Obsolete.
*/
void QFileDialog::updatePathBox( const char * )
{
    // unused
}


/*!  Make sure the minimum and maximum sizes of everything are sane.
*/

void QFileDialog::updateGeometry()
{
    if ( !d || !d->geometryDirty )
	return;

    d->geometryDirty = FALSE;

    QSize r, t;

    // we really should have a QSize::unite()
#define RM r.setWidth( QMAX(r.width(),t.width()) ); \
    r.setHeight( QMAX(r.height(),t.height()) )

    // labels first
    r = d->pathL->sizeHint();
    t = d->fileL->sizeHint();
    RM;
    t = d->typeL->sizeHint();
    RM;
    d->pathL->setFixedSize( r );
    d->fileL->setFixedSize( r );
    d->typeL->setFixedSize( r );

    // single-line input areas
    r = d->paths->sizeHint();
    t = nameEdit->sizeHint();
    RM;
    t = d->types->sizeHint();
    RM;
    t.setWidth( QCOORD_MAX );
    t.setHeight( r.height() );
    d->paths->setMinimumSize( r );
    d->paths->setMaximumSize( t );
    nameEdit->setMinimumSize( r );
    nameEdit->setMaximumSize( t );
    d->types->setMinimumSize( r );
    d->types->setMaximumSize( t );

    // buttons on top row
    r = QSize( 0, d->paths->minimumSize().height() );
    t = QSize( 20, 20 );
    RM;
    if ( r.height() > r.width() )
	r.setWidth( r.height() );
    d->cdToParent->setFixedSize( r );
    d->mcView->setFixedSize( r );
    d->detailView->setFixedSize( r );
    // ...

    // open/save, cancel
    r = QSize( 80, 0 );
    t = okB->sizeHint();
    RM;
    t = cancelB->sizeHint();
    RM;
    okB->setFixedSize( r );
    cancelB->setFixedSize( r );

    d->topLevelLayout->activate();

#undef RM
}


/*!  Updates the dialog when the cursor moves in the listview.
*/

void QFileDialog::updateFileNameEdit( QListViewItem * newItem )
{
    if ( !newItem )
	return;

    QFileDialogPrivate::File * i = (QFileDialogPrivate::File *)newItem;

    if ( mode() == Directory ) {
	if ( i->info.isDir() ) {
	    d->currentFileName = i->info.absFilePath();
	    nameEdit->clear();
	    nameEdit->insert( d->currentFileName );
	}
    } else if ( i->info.isFile() || mode() == AnyFile ) {
	d->currentFileName = i->info.absFilePath();
	nameEdit->setText( i->info.fileName() );
    }
}


/*!  Updates the dialog when enter is pressed in the file name edit. */

void QFileDialog::fileNameEditDone()
{
    QFileInfo f ( cwd, nameEdit->text() );
    if ( mode() == AnyFile ||
	 (mode() == ExistingFile && f.isFile()) ||
	 (mode() == Directory && f.isDir()) )
	d->currentFileName = f.absFilePath();
}



/*!  This private slot reacts to double-clicks in the list view. */

void QFileDialog::selectDirectoryOrFile( QListViewItem * newItem )
{
    if ( !newItem || !newItem->isSelectable() )
	return;

    QFileDialogPrivate::File * i = (QFileDialogPrivate::File *)newItem;

    if ( i->info.isDir() ) {
	setDir( i->info.absFilePath() );
    } else if ( i->info.isFile() && mode() != Directory ) {
	d->currentFileName = i->info.fileName();
	emit fileSelected( cwd.filePath( d->currentFileName ) );
	accept();
    }
}


/*!  Pops up a context menu at the global point \a p, for column \a c
  of item \a i. */

void QFileDialog::popupContextMenu( QListViewItem * i, const QPoint & p,
				    int c )
{
    QPopupMenu m( 0, "file dialog context menu" );

    int asc = m.insertItem( tr("&Ascending") );
    int desc = m.insertItem( tr("&Descending") );
    if ( i )
	m.insertItem( "&Sanctify" );

    m.move( p );
    int res = m.exec();
    if ( res == asc )
	files->setSorting( c, TRUE );
    else if ( res == desc )
	files->setSorting( c, FALSE );
}


void QFileDialog::fileSelected( int  )
{
    // unused
}

void QFileDialog::fileHighlighted( int )
{
    // unused
}

void QFileDialog::dirSelected( int )
{
    // unused
}

void QFileDialog::pathSelected( int )
{
    // unused
}


void QFileDialog::cdUpClicked()
{
    if ( cwd.cdUp() ) {
	cwd.convertToAbs();
	rereadDir();
    }
}


/*!  Ask the user for the name of an existing directory, starting at
  \a dir.  Returns the name of the directory the user selected.

  If \a dir is null, getExistingDirectory() starts wherever the
  previous file dialog left off.
*/

QString QFileDialog::getExistingDirectory( const char *dir,
					   QWidget *parent,
					   const char *name )
{
    QFileDialog *dialog	= new QFileDialog( parent, name, TRUE );
    dialog->setCaption( dialog->tr("Find Directory") );

    dialog->setMode( Directory );

    dialog->d->types->clear();
    dialog->d->types->insertItem( dialog->tr("Directories") );
    dialog->d->types->setEnabled( FALSE );

    if ( dir && *dir ) {
	QFileInfo f( dir );
	if ( f.isDir() ) {
	    filedlg_dir = dir;
	    filedlg_dir.detach();
	    dialog->setDir( dir );
	}
    } else if ( !filedlg_dir.isEmpty() ) {
	QFileInfo f( dir );
	if ( f.isDir() )
	    dialog->setDir( filedlg_dir );
    }	

    QString result;
    if ( dialog->exec() == QDialog::Accepted ) {
	result = dialog->selectedFile();
	QFileInfo f( result );
	if ( f.isDir() ) {
	    filedlg_dir = result;
	    filedlg_dir.detach();
	} else {
	    result = 0;
	}
    }
    delete dialog;
    return result;
}


/*!  Sets this file dialog to \a newMode, which can be one of \c
  Directory (directories are accepted), \c ExistingFile (existing
  files are accepted) or \c AnyFile (any valid file name is accepted).

  \sa mode()
*/

void QFileDialog::setMode( Mode newMode )
{
    if ( d->mode != newMode ) {
	d->mode = newMode;
	if ( newMode == Directory ) {
	    cwd.setFilter( QDir::Dirs | QDir::Drives );
	} else {
	    cwd.setFilter( 0 );
	}
	rereadDir();
    }
}


/*!  Returns the file mode of this dialog.

  \sa setMode()
*/

QFileDialog::Mode QFileDialog::mode() const
{
    return d->mode;
}
