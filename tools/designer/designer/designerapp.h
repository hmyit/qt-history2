 /**********************************************************************
** Copyright (C) 2000 Trolltech AS.  All rights reserved.
**
** This file is part of Qt Designer.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef DESIGNERAPP_H
#define DESIGNERAPP_H

#include <qapplication.h>

class QLabel;

#if defined(HAVE_KDE)
#include <kapp.h>
class DesignerApplication : public KApplication
#else
#include <qapplication.h>
class DesignerApplication : public QApplication
#endif
{
public:
#if defined(HAVE_KDE)
    DesignerApplication( int &argc, char **argv, const QCString &rAppName );
#else
    DesignerApplication( int &argc, char **argv );
#endif

    QLabel *showSplash();
    static void closeSplash();

    static QString settingsKey() { return "/Qt Designer/3.0/"; }

protected:
    QDateTime lastMod;

#if defined(Q_WS_WIN)
    bool winEventFilter( MSG *msg );
    uint DESIGNER_OPENFILE;
#endif

};


#endif
