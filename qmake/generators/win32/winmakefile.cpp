/****************************************************************************
** $Id: //depot/qt/main/src/%s#3 $
**
** Definition of ________ class.
**
** Created : 970521
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the network module of the Qt GUI Toolkit.
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
** Licensees holding valid Qt Enterprise Edition licenses may use this
** file in accordance with the Qt Commercial License Agreement provided
** with the Software.
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

#include "winmakefile.h"
#include <qtextstream.h>
#include "project.h"
#include "option.h"
#include <qstring.h>
#include <qstringlist.h>
#include <qdir.h>

Win32MakefileGenerator::Win32MakefileGenerator(QMakeProject *p) : MakefileGenerator(p)
{
    
}


void
Win32MakefileGenerator::writeSubDirs(QTextStream &t)
{
    t << "MAKEFILE=	" << var("MAKEFILE") << endl;
    t << "TMAKE =	" << var("TMAKE") << endl;
    t << "SUBDIRS	=" << varList("SUBDIRS") << endl;

    t << "all: $(SUBDIRS)" << endl << endl;

    QStringList &sdirs = project->variables()["SUBDIRS]"];
    QStringList::Iterator sdirit;

    for(sdirit = sdirs.begin(); sdirit != sdirs.end(); ++sdirit) {
	t << (*sdirit) << ":";
	if(project->variables()["TMAKE_NOFORCE"].isEmpty())
	    t << " FORCE";
	t << "\n\t"
	  << "cd " << (*sdirit) << "\n\t"
	  << "$(MAKE)" << "\n\t"
	  << "@cd .." << endl << endl;
    }

    t << "qmake: " << "\n\t"
      << "qmake " << project->projectFile();
    if (Option::output.name())
	t << " -o " << Option::output.name();
    t << endl << endl;

    t << "tmake_all:";
    for(sdirit = sdirs.begin(); sdirit != sdirs.end(); ++sdirit) { 
	t << "\n\t" 
	  << "cd " << (*sdirit) << "\n\t"
	  << "$(TMAKE) " << (*sdirit) << ".pro -o $(MAKEFILE)" << "\n\t"
	  << "@cd ..";
    }
    t << endl << endl;

    t << "clean:";
    for(sdirit = sdirs.begin(); sdirit != sdirs.end(); ++sdirit) { 
	t << "\n\t"
	  << "cd " << (*sdirit) << "\n\t"
	  << "$(MAKE) clean" << "\n\t"
	  << "@cd ..";
    }
    t << endl << endl;

    if(!project->variables()["TMAKE_NOFORCE"].isEmpty())
	t << "FORCE:" << endl << endl;

}


int
Win32MakefileGenerator::findHighestVersion(const QString &d, const QString &stem)
{
    QDir dir(d, stem + "*.lib");
    QRegExp num("[0-9]*");
    int nbeg, nend, biggest=-1;
    for(int x=0; x < dir.count(); x++) {
	if((nbeg = num.match(d[x], 0, &nend)) != -1) {
	    int n = QString(d[x]).mid(nbeg, nend - nbeg).toInt();
	    if(n > biggest)
		biggest = n;
	}
    }
}


