/****************************************************************************
**
** Copyright (C) 2005-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "glwidget.h"

static GLint cubeArray[][3] = {
    {0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0},
    {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1},
    {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1},
    {0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0},
    {0, 1, 0}, {0, 0, 0}, {0, 0, 1}, {0, 1, 1},
    {1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}
};

static GLint cubeTextureArray[][2] = {
    {0, 0}, {1, 0}, {1, 1}, {0, 1},
    {0, 0}, {0, 1}, {1, 1}, {1, 0},
    {0, 0}, {1, 0}, {1, 1}, {0, 1},
    {1, 0}, {0, 0}, {0, 1}, {1, 1},
    {0, 0}, {1, 0}, {1, 1}, {0, 1},
    {1, 0}, {0, 0}, {0, 1}, {1, 1}
};

static GLint faceArray[][2] = {
    {1, -1}, {1, 1}, {-1, 1}, {-1, -1}
};

static GLubyte colorArray[][4] = {
    {170, 202, 0, 255},
    {120, 143, 0, 255},
    {83, 102, 0, 255},
    {120, 143, 0, 255}
};

GLWidget::GLWidget(QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
    // create the pbuffer
    pbuffer = new QGLPbuffer(QSize(512, 512), format(), this);
    startTimer(20);
}

GLWidget::~GLWidget()
{
    glDeleteTextures(1, &dynamicTexture);
    glDeleteLists(pbufferList, 1);
    delete pbuffer;
}

void GLWidget::initializeGL()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glFrustum(-1, 1, -1, 1, 10, 100);
    glTranslatef(-0.5f, -0.5f, -0.5f);
    glTranslatef(0.0f, 0.0f, -15.0f);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_CULL_FACE);
    initCommon();
    initPbuffer();
    glBindTexture(GL_TEXTURE_2D, dynamicTexture);

    for (int i = 0; i < 3; ++i) {
        yOffs[i] = 0.0f;
        xInc[i] = 0.005f;
        rot[i] = 0.0f;
    }
    xOffs[0]= 0.0f;
    xOffs[1]= 0.5f;
    xOffs[2]= 1.0f;
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void GLWidget::paintGL()
{
    // draw a spinning cube into the pbuffer..
    pbuffer->makeCurrent();
    glCallList(pbufferList);

#if  defined(Q_WS_X11)
    // rendering directly to a texture is not supported on X11, unfortunately
    pbuffer->copyToTexture(dynamicTexture);
#endif
    // ..and use the pbuffer contents as a texture when rendering the
    // background and the bouncing cubes
    makeCurrent();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw the background
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glVertexPointer(2, GL_INT, 0, faceArray);
    glDrawArrays(GL_QUADS, 0, 4);
    glVertexPointer(3, GL_INT, 0, cubeArray);

    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    // draw the bouncing cubes
    drawCube(0, 0.0f, 1.5f, 2.5f, 1.5f);
    drawCube(1, 1.0f, 2.0f, 2.5f, 2.0f);
    drawCube(2, 2.0f, 3.5f, 2.5f, 2.5f);
}

void GLWidget::drawCube(int i, GLfloat z, GLfloat rotation, GLfloat jmp, GLfloat amp)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(xOffs[i], yOffs[i], z);
    glTranslatef(0.5f, 0.5f, 0.5f);
    GLfloat scale = 0.75 + i*(0.25f/2);
    glScalef(scale, scale, scale);
    glRotatef(rot[i], 1.0f, 1.0f, 1.0f);
    glTranslatef(-0.5f, -0.5f, -0.5f);

    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
    glDrawArrays(GL_QUADS, 0, 24);

    if (xOffs[i] > 1.0f || xOffs[i] < -1.0f) {
        xInc[i] = -xInc[i];
        xOffs[i] = xOffs[i] > 1.0f ? 1.0f : -1.0f;
    }
    xOffs[i] += xInc[i];
    yOffs[i] = qAbs(cos((-3.141592f * jmp) * xOffs[i]) * amp) - 1;
    rot[i] += rotation;
}

void GLWidget::drawFace()
{
    glDisable(GL_TEXTURE_2D);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_INT, 0, faceArray);

    glDrawArrays(GL_QUADS, 0, 4);

    glVertexPointer(3, GL_INT, 0, cubeArray);
    glDisableClientState(GL_COLOR_ARRAY);
    glEnable(GL_TEXTURE_2D);
}

void GLWidget::initCommon()
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_INT, 0, cubeArray);
    glTexCoordPointer(2, GL_INT, 0, cubeTextureArray);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colorArray);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void GLWidget::initPbuffer()
{
    // set up the pbuffer context
    pbuffer->makeCurrent();
    initCommon();

    glViewport(0, 0, pbuffer->size().width(), pbuffer->size().height());
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -99, 99);
    glTranslatef(-0.5f, -0.5f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    pbufferList = glGenLists(1);
    glNewList(pbufferList, GL_COMPILE);
    {
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        glPushMatrix();
        glLoadIdentity();
        glTranslatef(0.5f, 0.5f, 0.0f);
        glTranslatef(0.0f, 0.0f, -2.0f);
        drawFace();
        glPopMatrix();

        glTranslatef(0.5f, 0.5f, 0.5f);
        glRotatef(3.0f, 1.0f, 1.0f, 1.0f);
        glTranslatef(-0.5f, -0.5f, -0.5f);
        glColor4f(0.9f, 0.9f, 0.9f, 1.0f);
        glDrawArrays(GL_QUADS, 0, 24);
    }
    glEndList();
    // generate a texture that has the same size/format as the pbuffer
    dynamicTexture = pbuffer->generateTexture();
    makeCurrent();

    // bind the dynamic texture to the pbuffer - this is a no-op under X11
    pbuffer->bind(dynamicTexture);
}

