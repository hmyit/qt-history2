/****************************************************************************
** $Id: //depot/qt/main/src/tools/qfileinfo.cpp#61 $
**
** Implementation of QDir class
**
** Created : 950628
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the tools module of the Qt GUI Toolkit.
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
** licenses for Unix/X11 or for Qt/Embedded may use this file in accordance
** with the Qt Commercial License Agreement provided with the Software.
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

#include "qglobal.h"

#include "qdir.h"
#ifndef QT_NO_DIR

#include "qdir_p.h"
#include "qfileinfo.h"
#include "qfiledefs_p.h"
#include "qregexp.h"
#include "qstringlist.h"
#include <stdlib.h>
#include <ctype.h>
#include "qt_mac.h"

void QDir::slashify( QString& n )
{
	if( n.isNull() )
		return;
	for ( int i = 0; i < (int)n.length(); i++) {
		if( n[i] == ':' )
			n[i] = '/';
	}
}

QString QDir::homeDirPath()
{
    QString d;
    d = QFile::decodeName(getenv("HOME"));
    slashify( d );
    if ( d.isNull() )
	d = rootDirPath();
    return d;
}

QString QDir::canonicalPath() const
{
    QString r;

    char cur[PATH_MAX];
    char tmp[PATH_MAX];
    GETCWD( cur, PATH_MAX );
    if ( CHDIR(QFile::encodeName(dPath)) >= 0 ) {
	GETCWD( tmp, PATH_MAX );
	r = QFile::decodeName(tmp);
    }
    CHDIR( cur );

    slashify( r );
    return r;
}

bool QDir::mkdir( const QString &dirName, bool acceptAbsPath ) const
{
    return MKDIR( QFile::encodeName(filePath(dirName,acceptAbsPath)), 0777 ) 
	== 0;
}

bool QDir::rmdir( const QString &dirName, bool acceptAbsPath ) const
{
    return RMDIR( QFile::encodeName(filePath(dirName,acceptAbsPath)) ) == 0;
}

bool QDir::isReadable() const
{
#ifdef Q_OS_MACX
    return ACCESS( QFile::encodeName(dPath), R_OK | X_OK ) == 0;
#else
	return TRUE;
#endif
}

bool QDir::isRoot() const
{
    return dPath == QString::fromLatin1("/");
}

bool QDir::rename( const QString &name, const QString &newName,
		   bool acceptAbsPaths	)
{
    if ( name.isEmpty() || newName.isEmpty() ) {
#if defined(QT_CHECK_NULL)
	qWarning( "QDir::rename: Empty or null file name(s)" );
#endif
	return FALSE;
    }
    QString fn1 = filePath( name, acceptAbsPaths );
    QString fn2 = filePath( newName, acceptAbsPaths );
    return ::rename( QFile::encodeName(fn1),
		     QFile::encodeName(fn2) ) == 0;
}

bool QDir::setCurrent( const QString &path )
{
    int r;
    r = CHDIR( QFile::encodeName(path) );
    return r >= 0;
}

QString QDir::currentDirPath()
{
    QString result;

    STATBUF st;
    if ( STAT( ".", &st ) == 0 ) {
	char currentName[PATH_MAX];
	if ( GETCWD( currentName, PATH_MAX ) != 0 )
	    result = QFile::decodeName(currentName);
#if defined(QT_DEBUG)
	if ( result.isNull() )
	    qWarning( "QDir::currentDirPath: getcwd() failed" );
#endif
    } else {
#if defined(QT_DEBUG)
	qWarning( "QDir::currentDirPath: stat(\".\") failed" );
#endif
    }
    slashify( result );
    return result;
}

QString QDir::rootDirPath()
{
    QString d = QString::fromLatin1( "/" );
    return d;
}

bool QDir::isRelativePath( const QString &path )
{
    int len = path.length();
    if ( len == 0 )
	return TRUE;
    return path[0] != '/';
}

const unsigned char * p_str(const char * c); //qwidget_mac.cpp

bool QDir::readDirEntries( const QString &nameFilter,
			   int filterSpec, int sortSpec )
{
    int i;
    if ( !fList ) {
	fList  = new QStringList;
	Q_CHECK_PTR( fList );
	fiList = new QFileInfoList;
	Q_CHECK_PTR( fiList );
	fiList->setAutoDelete( TRUE );
    } else {
	fList->clear();
	fiList->clear();
    }

    QStringList filters = qt_makeFilterList( nameFilter );

    bool doDirs	    = (filterSpec & Dirs)	!= 0;
    bool doFiles    = (filterSpec & Files)	!= 0;
    bool noSymLinks = (filterSpec & NoSymLinks) != 0;
    bool doReadable = (filterSpec & Readable)	!= 0;
    bool doWritable = (filterSpec & Writable)	!= 0;
    bool doExecable = (filterSpec & Executable) != 0;
    bool doHidden   = (filterSpec & Hidden)	!= 0;

    QFileInfo fi;
    FSIterator dir;
	FSRef ref;
	
	OSStatus bar;
	FSSpec spec;
	if( FSMakeFSSpec(0, 0, p_str(QFile::encodeName(convertSeparators(dPath))), &spec) != noErr)
		return FALSE;
 	if( FSpMakeFSRef(&spec, &ref) != noErr)
 		return FALSE;
	if( FSOpenIterator( &ref, kFSIterateFlat, &dir ) != noErr)
		return FALSE;

	ItemCount refn = 512;
	FSRef *refs = (FSRef *)malloc(refn);
	FSGetCatalogInfoBulk( dir, refn, &refn, NULL, kFSCatInfoNone, NULL, refs, NULL, NULL );
	const UInt32 tmp_len = 1024;
	char *tmp_n = (char *)malloc(tmp_len);
    for(int x = 0; x < refn; x++) {
	   if(FSRefMakePath( &refs[x], (UInt8 *)tmp_n, tmp_len) != noErr)
	   		continue;
		QString fn = QFile::decodeName(tmp_n);
		fi.setFile( *this, fn );
		if ( !match( filters, fn ) && !(allDirs && fi.isDir()) )
	     	continue;
		if  ( (doDirs && fi.isDir()) || (doFiles && fi.isFile()) ) {
	   		if ( noSymLinks && fi.isSymLink() )
	        	continue;
	    	if ( (filterSpec & RWEMask) != 0 )
	        	if ( (doReadable && !fi.isReadable()) ||
	             	 (doWritable && !fi.isWritable()) ||
	             	(doExecable && !fi.isExecutable()) )
	            	continue;
	    		if ( !doHidden && fn[0] == '.' &&
	         		fn != QString::fromLatin1(".")
	         		&& fn != QString::fromLatin1("..") )
	        		continue;
	    		fiList->append( new QFileInfo( fi ) );
		}
    }
    FSCloseIterator(dir);
    free(tmp_n);
    free(refs);
    
    // Sort...
    if(fiList->count()) {
		QDirSortItem* si= new QDirSortItem[fiList->count()];
		QFileInfo* itm;
		i=0;
		for (itm = fiList->first(); itm; itm = fiList->next())
	    	si[i++].item = itm;
		qt_cmp_si_sortSpec = sortSpec;
		qsort( si, i, sizeof(si[0]), qt_cmp_si );
		// put them back in the list
		fiList->setAutoDelete( FALSE );
		fiList->clear();
		int j;
		for ( j=0; j<i; j++ ) {
	    	fiList->append( si[j].item );
	    	fList->append( si[j].item->fileName() );
		}
		delete [] si;
		fiList->setAutoDelete( TRUE );
    }

    if ( filterSpec == (FilterSpec)filtS && sortSpec == (SortSpec)sortS &&
	 		nameFilter == nameFilt )
			dirty = FALSE;
    else
			dirty = TRUE;
    return TRUE;
}

const QFileInfoList * QDir::drives()
{
    // at most one instance of QFileInfoList is leaked, and this variable
    // points to that list
    static QFileInfoList * knownMemoryLeak = 0;

    if ( !knownMemoryLeak ) {
	knownMemoryLeak = new QFileInfoList;
	// non-win32 versions both use just one root directory
	knownMemoryLeak->append( new QFileInfo( rootDirPath() ) );
    }

    return knownMemoryLeak;
}
#endif //QT_NO_DIR
