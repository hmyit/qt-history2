/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qtmain_win.cpp#3 $
**
** Implementation of Win32 startup routines.
**
** Created : 980823
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

#include "qapplication.h"
#if defined(_CC_BOOL_DEF_)
#undef	bool
#include <windows.h>
#define bool int
#else
#include <windows.h>
#endif


/*
  This file contains the code in the qtmain library for Windows.
  qtmain contains the Windows startup code and is required for
  linking to the Qt DLL.

  When a Windows application starts, the WinMain function is
  invoked. WinMain calls qWinMain in the Qt DLL/library, which
  initializes Qt.
*/

extern void qWinMain( HANDLE, HANDLE, LPSTR, int, int &, QArray<pchar> & );

#if defined(NEEDS_QMAIN)
int qMain( int, char ** );
#else
extern "C" int main( int, char ** );
#endif


/*
  WinMain() - Initializes Windows and calls user's startup function main().
  NOTE: WinMain() won't be called if the application was linked as a "console"
  application.
*/

extern "C"
int APIENTRY WinMain( HANDLE instance, HANDLE prevInstance,
		      LPSTR  cmdParam, int cmdShow )
{
    int argc = 0;
    QArray<pchar> argv( 8 );
    qWinMain( instance, prevInstance, cmdParam, cmdShow, argc, argv );
    return main( argc, argv.data() );
}
