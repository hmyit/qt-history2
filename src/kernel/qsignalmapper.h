/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qsignalmapper.h#10 $
**
** Definition of QSignalMapper class
**
** Created : 980503
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef QSIGNALMAPPER_H
#define QSIGNALMAPPER_H

#ifndef QT_H
#include "qobject.h"
#endif // QT_H

class  QSignalMapperData;
struct QSignalMapperRec;


class Q_EXPORT QSignalMapper : public QObject {
    Q_OBJECT
public:
    QSignalMapper( QObject* parent, const char* name=0 );
    ~QSignalMapper();

    virtual void setMapping( const QObject* sender, int identifier );
    virtual void setMapping( const QObject* sender, const QString &identifier );
    void removeMappings( const QObject* sender );

signals:
    void mapped(int);
    void mapped(const QString &);

public slots:
    void map();

private:
    QSignalMapperData* d;
    QSignalMapperRec* getRec( const QObject* );

private slots:
    void removeMapping();
};


#endif // QSIGNALMAPPER_H
