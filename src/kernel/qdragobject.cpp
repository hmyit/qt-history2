/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qdragobject.cpp#26 $
**
** Implementation of Drag and Drop support
**
** Copyright (C) 1996 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#include "qdragobject.h"
#include "qapplication.h"
#include "qcursor.h"
#include "qpoint.h"
#include "qwidget.h"
#include "qobjectlist.h"
#include "qobjectdict.h"
#include "qimage.h"
#include "qbuffer.h"


// both a struct for storing stuff in and a wrapper to avoid polluting
// the name space

struct QDragData {
    QDragData(): autoDelete( TRUE ), next( 0 ) {}
    bool autoDelete;
    QDragObject * next;
};


struct QStoredDragData {
    QStoredDragData() {}
    QString fmt;
    QByteArray enc;
};



// the universe's only drag manager
static QDragManager * manager = 0;


QDragManager::QDragManager()
    : QObject( qApp, "global drag manager" )
{
    object = 0;
    dragSource = 0;
    dropWidget = 0;
    if ( !manager )
	manager = this;
    beingCancelled = FALSE;
    restoreCursor = FALSE;
}


QDragManager::~QDragManager()
{
    if ( restoreCursor )
	QApplication::restoreOverrideCursor();
    manager = 0;
}




/*!  Creates a drag object which is a child of \a dragSource and
  named \a name.

  Note that the drag object will be deleted when \a dragSource is.
*/

QDragObject::QDragObject( QWidget * dragSource, const char * name )
    : QObject( dragSource, name )
{
    d = new QDragData();
    if ( !manager && qApp )
	(void)new QDragManager();
}


/*!  Deletes the drag object and frees up the storage used. */

QDragObject::~QDragObject()
{
    d->autoDelete = FALSE; // so cancel() won't delete this object
    if ( manager && manager->object == this )
	manager->cancel();
    delete d;
}


/*!
  Starts a drag operation using the contents of this object.

  Under X11, this function usually returns immediately.  Under Windows,
  it does not return until the drag is complete. In either case,
  the application should take care to prepare for events that might
  occur during the drag \e prior to calling this function.
*/
void QDragObject::startDrag()
{
    if ( manager )
	manager->startDrag( this );
}




/*!
  \fn QByteArray QDragObject::encodedData(const char*) const

  Returns the encoded payload of this object.  The drag manager
  calls this when the recipient needs to see the content of the drag;
  this generally doesn't happen until the actual drop.

  Subclasses must override this function.
*/

/*!  Sets this object to be deleted automatically when Qt no longer
  needs it if \a enable is TRUE, and to not be deleted by Qt if \a
  enable is FALSE.

  The default is TRUE. */

void QDragObject::setAutoDelete( bool enable )
{
    d->autoDelete = enable;
}


/*!  Returns TRUE if the object should be deleted when the drag
  operation finishes, or FALSE if it should not. */

bool QDragObject::autoDelete() const
{
    return d->autoDelete;
}



/*!
  Returns TRUE if the drag object can provide the data
  in format \a mimeType.  The default implementation
  iterates over format().
*/
bool QDragObject::provides(const char* mimeType) const
{
    const char* fmt;
    for (int i=0; (fmt = format(i)); i++) {
	if ( !qstricmp(mimeType,fmt) )
	    return TRUE;
    }
    return FALSE;
}


/*!
  \fn const char * QDragObject::format(int i) const

  Returns the \e ith format, or NULL.
*/


/*!  Returns a pointer to the drag source where this object originated.
*/

QWidget * QDragObject::source()
{
    if ( parent()->isWidgetType() )
	return (QWidget *)parent();
    else
	return 0;
}


/*!
  Allows another QDragObject to be linked to this one
  as an alternative representation of the data.

  Subclasses of QDragObject should \e not use this - it is
  provided for the application programmer (if a subclass used
  it, the application programmer might well replace it!)
*/
void QDragObject::setAlternative( QDragObject * next )
{
    d->next = next;
}


/*!
  Returns the currently set alternative QDragObject.

  \sa setAlternative()
*/
QDragObject * QDragObject::alternative() const
{
    return d->next;
}


/*! \class QTextDragObject qdragobject.h

  \brief The QTextDragObject provides a drag and drop object for
  transferring plain text.

  \ingroup kernel

  Plain text is defined as single- or multi-line US-ASCII or an
  unspecified 8-bit character set.

  Qt provides no built-in mechanism for delivering only single-line
  or only US-ASCII text.
*/


/*!  Creates a text drag object and sets it to \a text.  \a parent
  must be the drag source, \a name is the object name. */

QTextDragObject::QTextDragObject( const char * text,
				  QWidget * parent, const char * name )
    : QStoredDragObject( "text/plain", parent, name )
{
    setText( text );
}


/*!  Creates a default text drag object.  \a parent must be the drag
  source, \a name is the object name.
*/

QTextDragObject::QTextDragObject( QWidget * parent, const char * name )
    : QStoredDragObject( "text/plain", parent, name )
{
}


/*!  Destroys the text drag object and frees all allocated resources.
*/

QTextDragObject::~QTextDragObject()
{
    // nothing
}


/*!
  Sets the text to be dragged.  You will need to call this if you did
  not pass the text during construction.
*/

void QTextDragObject::setText( const char * text )
{
    QString tmp( text );
    setEncodedData( tmp );
}


/*! \class QImageDragObject qdragobject.h

  \brief The QImageDragObject provides a drag and drop object for
  tranferring images.

  \ingroup kernel

  Images are offered to the receiving application in multiple formats,
  determined by the \link QImage::outputFormats() output formats\endlink
  in Qt.
*/


/*!  Creates an image drag object and sets it to \a image.  \a parent
  must be the drag source, \a name is the object name. */

QImageDragObject::QImageDragObject( QImage image,
				  QWidget * parent, const char * name )
    : QDragObject( parent, name )
{
    setImage( image );
}

/*!  Creates a default text drag object.  \a parent must be the drag
  source, \a name is the object name.
*/

QImageDragObject::QImageDragObject( QWidget * parent, const char * name )
    : QDragObject( parent, name )
{
}


/*!  Destroys the image drag object and frees all allocated resources.
*/

QImageDragObject::~QImageDragObject()
{
    // nothing
}


/*!
  Sets the image to be dragged.  You will need to call this if you did
  not pass the image during construction.
*/
void QImageDragObject::setImage( QImage image )
{
    img = image;
    // ### should detach?
    ofmts = QImage::outputFormats();
    ofmts.remove("PBM");
    if ( image.depth()!=32 ) {
	// BMP better than PPM for paletted images
	if ( ofmts.remove("BMP") ) // move to front
	    ofmts.insert(0,"BMP");
    }
    // Could do more magic to order mime types
}

const char * QImageDragObject::format(int i) const
{
    if ( i < (int)ofmts.count() ) {
	static QString str;
	str.sprintf("image/%s",(((QImageDragObject*)this)->ofmts).at(i));
	str = str.lower();
	if ( str == "image/pbmraw" )
	    str = "image/ppm";
	return str;
    } else {
	return 0;
    }
}

QByteArray QImageDragObject::encodedData(const char* fmt) const
{
    if ( qstrnicmp( fmt, "image/", 6 )==0 ) {
	QString f = fmt+6;
	QByteArray data;
	QBuffer w( data );
	w.open( IO_WriteOnly );
	QImageIO io( &w, f.upper() );
	io.setImage( img );
	if  ( !io.write() )
	    return QByteArray();
	w.close();
	return data;
    } else {
	return QByteArray();
    }
}


/*!
  \class QStoredDragObject qdragobject.h
  \brief Simple stored-value drag object for arbitrary MIME data.

  When a block of data only has one representation, you can use
  a QStoredDragObject to hold it.
*/

/*!
  Constructs a QStoredDragObject.  The parameters are passed
  to the QDragObject constructor, and the format is set to \a mimeType.

  The data will be unset.  Use setEncodedData() to set it.
*/
QStoredDragObject::QStoredDragObject( const char* mimeType, QWidget * dragSource, const char * name ) :
    QDragObject(dragSource,name)
{
    d = new QStoredDragData();
    d->fmt = mimeType;
}

/*!
  Destroys the drag object and frees all allocated resources.
*/
QStoredDragObject::~QStoredDragObject()
{
    delete d;
}

const char * QStoredDragObject::format(int i) const
{
    if ( i==0 )
	return d->fmt;
    else
	return 0;
}


/*!
  Sets the encoded data of this drag object to \a encodedData.  The
  encoded data is what's delivered to the drop sites, and must be in a
  strictly defined and portable format.

  The drag object can't be dropped (by the user) until this function
  has been called.
*/

void QStoredDragObject::setEncodedData( QByteArray & encodedData )
{
    d->enc = encodedData;
    d->enc.detach();
}

/*!
  Returns the stored data.

  \sa setEncodedData()
*/
QByteArray QStoredDragObject::encodedData(const char* m) const
{
    if ( m == d->fmt )
	return d->enc;
    else
	return QByteArray();
}


