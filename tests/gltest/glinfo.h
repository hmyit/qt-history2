/****************************************************************************
** $Id: $
**
** Copyright (C) 1992-2001 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#ifndef GLINFO_H
#define GLINFO_H

#include <qstring.h>
#include <qstringlist.h>
#include <X11/Xlib.h>

struct visual_attribs {
    /* X visual attribs */
    int id;
    int klass;
    int depth;
    int redMask, greenMask, blueMask;
    int colormapSize;
    int bitsPerRGB;
 
    /* GL visual attribs */
    int supportsGL;
    int transparent;
    int bufferSize;
    int level;
    int rgba;
    int doubleBuffer;
    int stereo;
    int auxBuffers;
    int redSize, greenSize, blueSize, alphaSize;
    int depthSize;
    int stencilSize;
    int accumRedSize, accumGreenSize, accumBlueSize, accumAlphaSize;
    int numSamples, numMultisample;
    int visualCaveat;
};

class GLInfo
{
public:
    GLInfo();
    QString getText();
    QStringList getViewList();

protected:
    QString *infotext;
    QStringList *viewlist;
    void print_screen_info(Display *dpy, int scrnum);
    void print_extension_list(const char *ext);
    void print_visual_info(Display *dpy, int scrnum);
};

#endif
