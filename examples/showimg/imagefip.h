/****************************************************************************
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of an example program for Qt.
** EDITIONS: NOLIMITS
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef IMAGEFIP_H
#define IMAGEFIP_H

#include <qfiledialog.h>
#include <qpixmap.h>

class ImageIconProvider : public QFileIconProvider
{
    Q_OBJECT
    QList<QByteArray> fmts;
    QPixmap imagepm;

public:
    ImageIconProvider( QWidget *parent=0, const char *name=0 );
    ~ImageIconProvider();

    const QPixmap * pixmap( const QFileInfo &fi );
};


#endif // IMAGEFIP_H
