/****************************************************************************
** $Id: //depot/qt/main/src/tools/qdir.h#35 $
**
** Definition of QDir class
**
** Created : 950427
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees with valid Qt Professional Edition licenses may distribute and
** use this file in accordance with the Qt Professional Edition License
** provided at sale or upon request.
**
** See http://www.troll.no/pricing.html or email sales@troll.no for
** information about the Professional Edition licensing, or see
** http://www.troll.no/qpl/ for QPL licensing information.
**
*****************************************************************************/

#ifndef QDIR_H
#define QDIR_H

#ifndef QT_H
#include "qstrlist.h"
#include "qfileinfo.h"
#endif // QT_H

typedef QList<QFileInfo> QFileInfoList;
typedef QListIterator<QFileInfo> QFileInfoListIterator;


class Q_EXPORT QDir
{
public:
    enum FilterSpec { Dirs	    = 0x001,
		      Files	    = 0x002,
		      Drives	    = 0x004,
		      NoSymLinks    = 0x008,
		      All	    = 0x007,
		      TypeMask	    = 0x00F,

		      Readable	    = 0x010,
		      Writable	    = 0x020,
		      Executable    = 0x040,
		      RWEMask	    = 0x070,
		      Modified	    = 0x080,
		      Hidden	    = 0x100,
		      System	    = 0x200,
		      AccessMask    = 0x3F0,
		      DefaultFilter = -1 };

    enum SortSpec   { Name	    = 0x00,
		      Time	    = 0x01,
		      Size	    = 0x02,
		      Unsorted	    = 0x03,
		      SortByMask    = 0x03,
		      DirsFirst	    = 0x04,
		      Reversed	    = 0x08,
		      IgnoreCase    = 0x10,
		      DefaultSort   = -1 };

    QDir();
    QDir( const QString &path, const QString &nameFilter = 0,
	  int sortSpec = Name | IgnoreCase, int filterSpec = All );
    QDir( const QDir & );

    virtual ~QDir();

    QDir       &operator=( const QDir & );
    QDir       &operator=( const QString &path );

    virtual void setPath( const QString &path );
    virtual QString path()		const;
    virtual QString absPath()	const;
    virtual QString canonicalPath()	const;

    virtual QString dirName() const;
    virtual QString filePath( const QString &fileName,
			  bool acceptAbsPath = TRUE ) const;
    virtual QString absFilePath( const QString &fileName,
			     bool acceptAbsPath = TRUE ) const;

    static QString convertSeparators( const QString &pathName );

    virtual bool cd( const QString &dirName, bool acceptAbsPath = TRUE );
    virtual bool cdUp();

    QString	nameFilter() const;
    virtual void setNameFilter( const QString &nameFilter );
    FilterSpec filter() const;
    virtual void setFilter( int filterSpec );
    SortSpec sorting() const;
    virtual void setSorting( int sortSpec );

    bool	matchAllDirs() const;
    virtual void setMatchAllDirs( bool );

    uint count() const;
    QString	operator[]( int ) const;

    const QStrList *entryList( int filterSpec = DefaultFilter,
        		       int sortSpec   = DefaultSort  ) const;
    const QStrList *entryList( const QString &nameFilter,
        		       int filterSpec = DefaultFilter,
        		       int sortSpec   = DefaultSort   ) const;
    
    const QFileInfoList *entryInfoList( int filterSpec = DefaultFilter,
        				int sortSpec   = DefaultSort  ) const;
    const QFileInfoList *entryInfoList( const QString &nameFilter,
					int filterSpec = DefaultFilter,
					int sortSpec   = DefaultSort   ) const;

    static const QFileInfoList *drives();

    virtual bool mkdir( const QString &dirName,
			bool acceptAbsPath = TRUE ) const;
    virtual bool rmdir( const QString &dirName,
			bool acceptAbsPath = TRUE ) const;

    virtual bool isReadable() const;
    virtual bool exists()   const;
    virtual bool isRoot()   const;

    virtual bool isRelative() const;
    virtual void convertToAbs();

    virtual bool operator==( const QDir & ) const;
    virtual bool operator!=( const QDir & ) const;

    virtual bool remove( const QString &fileName,
			 bool acceptAbsPath = TRUE );
    virtual bool rename( const QString &name, const QString &newName,
			 bool acceptAbsPaths = TRUE  );
    virtual bool exists( const QString &name,
			 bool acceptAbsPath = TRUE );

    static char separator();

    static bool setCurrent( const QString &path );
    static QDir current();
    static QDir home();
    static QDir root();
    static QString currentDirPath();
    static QString homeDirPath();
    static QString rootDirPath();

    static bool match( const QString &filter, const QString &fileName );
    static QString cleanDirPath( const QString &dirPath );
    static bool isRelativePath( const QString &path );

private:
    void init();
    virtual bool readDirEntries( const QString &nameFilter,
				 int FilterSpec, int SortSpec  );

    QString	dPath;
    QStrList   *fList;
    QFileInfoList *fiList;
    QString	nameFilt;
    FilterSpec	filtS;
    SortSpec	sortS;
    uint	dirty	: 1;
    uint	allDirs : 1;
};


inline QString QDir::path() const
{
    return dPath;
}

inline QString QDir::nameFilter() const
{
    return nameFilt;
}

inline QDir::FilterSpec QDir::filter() const
{
    return filtS;
}

inline QDir::SortSpec QDir::sorting() const
{
    return sortS;
}

inline bool QDir::matchAllDirs() const
{
    return allDirs;
}

inline bool QDir::operator!=( const QDir &d ) const
{
    return !(*this == d);
}


#endif // QDIR_H
