/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qpngio.h#2 $
**
** Definition of PNG QImage IOHandler
**
** Created : 970521
**
** Copyright (C) 1992-1998 Troll Tech AS.  All rights reserved.
**
** This file is part of Troll Tech's internal development tree for Qt.
**
** This header text will be replaced by an appropriate text by the
** mkdist script which generates external distributions.
**
** If you are using the Qt Professional Edition or the Qt Free Edition,
** please notify Troll Tech at <info@troll.no> if you see this text.
**
** To Troll Tech developers: This header was generated by the script
** fixcopyright-int. It has the same number of text lines as the free
** and professional editions to avoid line number inconsistency.
**
*****************************************************************************/

#ifndef QPNGIO_H
#define QPNGIO_H

#include <qimage.h>

void qInitPngIO();

class QIODevice;
class QImage;

class QPNGImageWriter {
public:
    QPNGImageWriter(QIODevice*);
    ~QPNGImageWriter();

    enum DisposalMethod { Unspecified, NoDisposal, RestoreBackground, RestoreImage };
    void setDisposalMethod(DisposalMethod);
    void setLooping(int loops=0); // 0 == infinity
    void setFrameDelay(int msecs);

    bool writeImage(const QImage& img, int x, int y);
    bool writeImage(const QImage& img)
	{ return writeImage(img, 0, 0); }

    QIODevice* device() { return dev; }

private:
    QIODevice* dev;
    int frames_written;
    DisposalMethod disposal;
    int looping;
    int ms_delay;
};

class QPNGImagePacker : public QPNGImageWriter {
public:
    QPNGImagePacker(QIODevice*, int depth, int convflags);

    bool packImage(const QImage& img);

private:
    QImage previous;
    int depth;
    int convflags;
};

#endif
