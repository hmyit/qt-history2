/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qdragobject.cpp#21 $
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

    QString fmt;
    QByteArray enc;
    bool autoDelete;
    QDragObject * next;
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


/*!  Sets the encoded data of this drag object to \a encodedData.  The
  encoded data is what's delivered to the drop sites, and must be in a
  strictly defined and portable format.

  Every subclass must call this function, normally in a higher-level
  function such as QTextDragObject::setText(), or in a
  reimplementation of encodedData() in case the class wants lazy
  evaluation of the data.

  The drag object can't be dropped (by the user) until this function
  has been called.
*/

void QDragObject::setEncodedData( QByteArray & encodedData )
{
    d->enc = encodedData;
    d->enc.detach();
    if ( !manager && qApp )
	(void)new QDragManager();
}


/*!  Returns the encoded payload of this object.  The drag manager
  calls this when the recipient needs to see the content of the drag;
  this generally doesn't happen until the actual drop.

  The default returns whatever was set using setEncodedData().
*/

QByteArray QDragObject::encodedData() const
{
    return d->enc;
}


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

*/

void QDragObject::setFormat( const char * mimeType )
{
    d->fmt = mimeType;
    d->fmt.detach();
}


/*!

*/

const char * QDragObject::format() const
{
    return d->fmt;
}


/*!

*/

void QDragObject::encode()
{
    // nothing
}

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
  tranferring plain text.

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
    : QDragObject( parent, name )
{
    setFormat( "text/plain" );
    setText( text );
}


/*!  Creates a default text drag object.  \a parent must be the drag
  source, \a name is the object name.
*/

QTextDragObject::QTextDragObject( QWidget * parent, const char * name )
    : QDragObject( parent, name )
{
    setFormat( "text/plain" );
}


/*!  Destroys the text drag object and frees all allocated resources.
*/

QTextDragObject::~QTextDragObject()
{
    // nothing
}


/*

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
    setFormat( "image/ppm" );
    setImage( image );
}

/*!  Creates a default text drag object.  \a parent must be the drag
  source, \a name is the object name.
*/

QImageDragObject::QImageDragObject( QWidget * parent, const char * name )
    : QDragObject( parent, name )
{
    setFormat( "image/ppm" );
    dirty = FALSE;
}


/*!  Destroys the image drag object and frees all allocated resources.
*/

QImageDragObject::~QImageDragObject()
{
    // nothing
}


/*

*/
void QImageDragObject::setImage( QImage image )
{
    img = image;
    // ### should detach?

    // Hack to make it work.
    QByteArray a(1);
    setEncodedData( a );

    dirty = TRUE;
}

QByteArray QImageDragObject::encodedData() const
{
    if ( dirty ) {
	QImageDragObject* that = (QImageDragObject*)this;
	const char* f = "PPM";
	QByteArray data;
	QBuffer w( data );
	w.open( IO_WriteOnly );
	QImageIO io( &w, f );
	io.setImage( img );
	if  ( io.write() ) {
	    w.close();
	    that->setEncodedData( data );
	}
	that->dirty = FALSE;
    }
    return QDragObject::encodedData();
}
