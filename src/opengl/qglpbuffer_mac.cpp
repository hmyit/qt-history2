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

#include "qglpbuffer.h"
#include "qglpbuffer_p.h"
#include <AGL/agl.h>

#include <qimage.h>
#include <private/qgl_p.h>
#include <qdebug.h>

QGLPbuffer::QGLPbuffer(const QSize &size, const QGLFormat &f, QGLWidget *shareWidget)
    : d_ptr(new QGLPbufferPrivate)
{
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_3)
    Q_D(QGLPbuffer);
    GLint attribs[40];
    int i = 0;

    attribs[i++] = AGL_RGBA;
    attribs[i++] = AGL_ACCELERATED;
    attribs[i] = AGL_NONE;

    AGLPixelFormat format = aglChoosePixelFormat(0, 0, attribs);
    if (!format) {
	qWarning("Unable to get a pixel format.");
    }

    AGLContext share = 0;
    if (shareWidget)
	share = d->share_ctx = static_cast<AGLContext>(shareWidget->d_func()->glcx->d_func()->cx);
    d->ctx = aglCreateContext(format, share);

    if (!aglCreatePBuffer(size.width(), size.height(), GL_TEXTURE_2D, GL_RGBA, 0, &d->pbuf)) {
	qWarning("Unable to create pbuffer - giving up.");
	return;
    }

    if (!aglSetPBuffer(d->ctx, d->pbuf, 0, 0, 0)) {
	qWarning("Unable to set pbuffer - giving up.");
	return;
    }

    aglDestroyPixelFormat(format);
    d->invalid = false;
    d->size = size;
    d->qctx = new QGLContext(f);
#else
    Q_UNUSED(size);
    Q_UNUSED(f);
    Q_UNUSED(shareWidget);
#endif
}

QGLPbuffer::~QGLPbuffer()
{
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_3)
    Q_D(QGLPbuffer);
    aglSetCurrentContext(0);
    aglDestroyContext(d->ctx);
    aglDestroyPBuffer(d->pbuf);
    delete d->qctx;
    delete d_ptr;
#endif
}

bool QGLPbuffer::bind(GLuint texture_id)
{
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_3)
    Q_D(QGLPbuffer);
    if (d->invalid)
	return false;
    glBindTexture(GL_TEXTURE_2D, texture_id);
    aglTexImagePBuffer(d->share_ctx, d->pbuf, GL_FRONT);
    glBindTexture(GL_TEXTURE_2D, texture_id); // updates texture target
    return true;
#else
    Q_UNUSED(texture_id);
    return false;
#endif
}

bool QGLPbuffer::release()
{
    return false;
}

bool QGLPbuffer::makeCurrent()
{
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_3)
    Q_D(QGLPbuffer);
    if (d->invalid)
        return false;

    return aglSetCurrentContext(d->ctx);
#else
    return false;
#endif
}

bool QGLPbuffer::doneCurrent()
{
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_3)
    Q_D(QGLPbuffer);
    if (d->invalid)
        return false;
    return aglSetCurrentContext(0);
#else
    return false;
#endif
}

GLuint QGLPbuffer::generateTexture(GLint)
{
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_3)
    Q_D(QGLPbuffer);
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    aglTexImagePBuffer(d->share_ctx, d->pbuf, GL_FRONT);
    glBindTexture(GL_TEXTURE_2D, texture); // updates texture target
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    return texture;
#else
    return 0;
#endif
}

bool QGLPbuffer::hasPbuffers()
{
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_3)
    return true;
#else
    return false;
#endif
}
