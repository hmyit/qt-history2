/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/*!
    \class QGLPbuffer
    \brief The QGLPbuffer class encapsulates an OpenGL pixel buffer.
    \since 4.1

    \ingroup multimedia

    QGLPbuffer provides functionality for creating and managing an
    OpenGL pixel buffer (pbuffer). A pixel buffer can be rendered into
    using full hardware acceleration. This is usually much faster than
    rendering into a system pixmap, where software rendering is often
    used. Under Windows and on Mac OS X it is also possible to bind the
    pixel buffer directly as a texture using the \c render_texture
    extension, thus eliminating the need for additional copy
    operations to generate dynamic textures.

    Note that when making use of the \c render_texture extension, the
    well known power-of-2 rule applies to the size of the buffer. If
    the size of the buffer is a non-power of 2 size, it can not be
    bound to a texture.

    \sa {opengl/pbuffers}
*/

#include <qglpbuffer.h>
#include <private/qglpbuffer_p.h>
#include <private/qpaintengine_opengl_p.h>
#include <qimage.h>


/*! \fn QGLPbuffer::QGLPbuffer(const QSize &size,
                               const QGLFormat &format=QGLFormat::defaultFormat(),
                               QGLWidget *shareWidget=0)

   Constructs an OpenGL pbuffer of the size \a size. If no \a format is
   specified the \link QGLFormat::defaultFormat() default
   format\endlink is used. If the \a shareWidget parameter points to a
   valid QGLWidget, the pbuffer will share its context with \a
   shareWidget.

*/

/*! \fn QGLPbuffer::~QGLPbuffer()

   Destroys the QGLPbuffer and frees its resources.
*/

/*! \fn bool QGLPbuffer::makeCurrent()

    Makes this pbuffer the current GL rendering context. Returns true
    on success, false otherwise.
 */

/*! \fn bool QGLPbuffer::doneCurrent()

    Makes no context the current GL context. Returns true on success,
    false otherwise.
 */

/*!
    This is a convenience function that generates and binds a 2D GL
    texture that is the same size as the buffer. The generated texture
    id is returned.
*/

#if defined(Q_WS_X11) || defined(Q_WS_WIN)
GLuint QGLPbuffer::generateDynamicTexture() const
{
    Q_D(const QGLPbuffer);
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, d->size.width(), d->size.height(), 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    return texture;
}
#endif

/*! \fn bool QGLPbuffer::bind(GLuint texture_id)

    Binds the texture specified with \a texture_id to this
    buffer. Returns true on success, false otherwise.

    This function uses the \c {render_texture} extension, which is
    currently not supported under X11. Under X11 you can achieve the
    same by copying the buffer contents to a texture after drawing
    into the buffer using copyToTexture().

    For the bind() call to succeed on the Mac, the pbuffer needs a
    shared context, i.e. the QGLPbuffer have to be created with a
    share widget.
 */

/*! \fn bool QGLPbuffer::release()

    Releases the buffer from any previously bound texture. Returns
    true on success, false otherwise.

    This function uses the \c {render_texture} extension, which is
    currently not supported under X11.
*/

/*! \fn bool QGLPbuffer::hasOpenGLPbuffers()

    Returns true if pbuffers are supported on this system, otherwise
    false.
 */


/*!
    This is a convenience function that copies the buffer contents
    (using \c {glCopyTexImage2D()}) into the texture specified with \a
    texture_id.
 */
void QGLPbuffer::updateDynamicTexture(GLuint texture_id) const
{
    Q_D(const QGLPbuffer);
    if (d->invalid)
        return;
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, d->size.width(), d->size.height(), 0);
}

/*!
    Returns the size of the buffer.
 */
QSize QGLPbuffer::size() const
{
    Q_D(const QGLPbuffer);
    return d->size;
}

/*!
    Returns the contents of the buffer as a QImage.
 */
QImage QGLPbuffer::toImage() const
{
    Q_D(const QGLPbuffer);
    if (d->invalid)
        return QImage();

    const_cast<QGLPbuffer *>(this)->makeCurrent();
    QImage img(d->size, QImage::Format_ARGB32);
    int w = d->size.width();
    int h = d->size.height();
    glReadPixels(0, 0, d->size.width(), d->size.height(), GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
	// OpenGL gives RGBA; Qt wants ARGB
	uint *p = (uint*)img.bits();
	uint *end = p + w*h;
	if (1) {
	    while (p < end) {
		uint a = *p << 24;
		*p = (*p >> 8) | a;
		p++;
	    }
	} else {
	    while (p < end) {
		*p = 0xFF000000 | (*p>>8);
		++p;
	    }
	}
    } else {
	// OpenGL gives ABGR (i.e. RGBA backwards); Qt wants ARGB
	img = img.rgbSwapped();
    }
    return img.mirrored();
}

/*!
    Returns the native pbuffer handle.
*/
Qt::HANDLE QGLPbuffer::handle() const
{
    Q_D(const QGLPbuffer);
    if (d->invalid)
        return 0;
    return d->pbuf;
}

/*!
    Returns true if this buffer is valid.
*/
bool QGLPbuffer::isValid() const
{
    Q_D(const QGLPbuffer);
    return !d->invalid;
}

Q_GLOBAL_STATIC(QOpenGLPaintEngine, qt_buffer_paintengine)
/*! \reimp
*/
QPaintEngine *QGLPbuffer::paintEngine() const
{
    return qt_buffer_paintengine();
}

extern int qt_defaultDpi();

/*! \reimp
*/
int QGLPbuffer::metric(PaintDeviceMetric metric) const
{
    Q_D(const QGLPbuffer);

    float dpmx = qt_defaultDpi()*100./2.54;
    float dpmy = qt_defaultDpi()*100./2.54;
    int w = d->size.width();
    int h = d->size.height();
    switch (metric) {
    case PdmWidth:
        return w;

    case PdmHeight:
        return h;

    case PdmWidthMM:
        return qRound(w * 1000 / dpmx);

    case PdmHeightMM:
        return qRound(h * 1000 / dpmy);

    case PdmNumColors:
        return 0;

    case PdmDepth:
        return 32;//d->depth;

    case PdmDpiX:
        return (int)(dpmx * 0.0254);

    case PdmDpiY:
        return (int)(dpmy * 0.0254);

    case PdmPhysicalDpiX:
        return (int)(dpmx * 0.0254);

    case PdmPhysicalDpiY:
        return (int)(dpmy * 0.0254);

    default:
        qWarning("QGLPbuffer::metric(), Unhandled metric type: %d\n", metric);
        break;
    }
    return 0;
}

/*!
    Generates and binds a 2D GL texture to the current context, based
    on \a image. The generated texture id is returned and can be used
    in later glBindTexture() calls.

    The \a target parameter specifies the texture target.

    Equivalent to calling QGLContext::bindTexture().

    \sa deleteTexture()
*/
GLuint QGLPbuffer::bindTexture(const QImage &image, GLenum target)
{
    Q_D(QGLPbuffer);
    return d->qctx->bindTexture(image, target, GL_RGBA8);
}


/*! \overload

    Generates and binds a 2D GL texture based on \a pixmap.

    Equivalent to calling QGLContext::bindTexture().

    \sa deleteTexture()
*/
GLuint QGLPbuffer::bindTexture(const QPixmap &pixmap, GLenum target)
{
    Q_D(QGLPbuffer);
    return d->qctx->bindTexture(pixmap, target, GL_RGBA8);
}

/*! \overload

    Reads the DirectDrawSurface (DDS) compressed file \a fileName and
    generates a 2D GL texture from it.

    Equivalent to calling QGLContext::bindTexture().

    \sa deleteTexture()
*/
GLuint QGLPbuffer::bindTexture(const QString &fileName)
{
    Q_D(QGLPbuffer);
    return d->qctx->bindTexture(fileName);
}

/*!
    Removes the texture identified by \a texture_id from the texture cache.

    Equivalent to calling QGLContext::deleteTexture().
 */
void QGLPbuffer::deleteTexture(GLuint texture_id)
{
    Q_D(QGLPbuffer);
    d->qctx->deleteTexture(texture_id);
}

/*!
    Returns the format of the pbuffer. The format may be different
    from the one that was requested.
*/
QGLFormat QGLPbuffer::format() const
{
    Q_D(const QGLPbuffer);
    return d->format;
}

/*! \fn int QGLPbuffer::devType() const
    \reimp
*/
