/****************************************************************************
** $Id$
**
** Implementation of QFont, QFontMetrics and QFontInfo classes for FB
**
** Created : 991026
**
** Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses for Qt/Embedded may use this file in accordance with the
** Qt Embedded Commercial License Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qwidget.h"
#include "qpainter.h"
#include "qfontdata_p.h"
#include "qcomplextext_p.h"
#include <private/qunicodetables_p.h>
#include "qfontdatabase.h"
#include "qstrlist.h"
#include "qcache.h"
#include "qdict.h"
#include "qtextcodec.h"
#include "qapplication.h"
#include "qfile.h"
#include "qtextstream.h"
#include "qmap.h"
#include "qshared.h"
#include "qfontmanager_qws.h"
#include "qmemorymanager_qws.h"
#include "qgfx_qws.h"
#include "qtextengine_p.h"
#include "qfontengine_p.h"

QFont::Script QFontPrivate::defaultScript = QFont::UnknownScript;

void QFont::initialize()
{
    // create global font cache
    if ( ! QFontCache::instance ) (void) new QFontCache;
}

void QFont::cleanup()
{
    // delete the global font cache
    delete QFontCache::instance;
}


/*****************************************************************************
  QFont member functions
 *****************************************************************************/

Qt::HANDLE QFont::handle() const
{
    QFontEngine *engine = d->engineForScript( QFontPrivate::defaultScript );
#ifdef QT_CHECK_STATE
    Q_ASSERT( engine != 0 );
#endif // QT_CHECK_STATE

    return engine->handle();
}

QString QFont::rawName() const
{
    return "unknown";
}

void QFont::setRawName( const QString & )
{
}


bool QFont::dirty() const
{
    return d->engineData == 0;
}

QString QFont::defaultFamily() const
{
    switch( d->request.styleHint ) {
	case QFont::Times:
	    return QString::fromLatin1("times");
	case QFont::Courier:
	    return QString::fromLatin1("courier");
	case QFont::Decorative:
	    return QString::fromLatin1("old english");
	case QFont::Helvetica:
	case QFont::System:
	default:
	    return QString::fromLatin1("helvetica");
    }
}

QString QFont::lastResortFamily() const
{
    return QString::fromLatin1("helvetica");
}

QString QFont::lastResortFont() const
{
    qFatal( "QFont::lastResortFont: Cannot find any reasonable font" );
    // Shut compiler up
    return QString::null;
}

void QFontPrivate::load( QFont::Script )
{
    QFontDef req = request;

    // 75 dpi on embedded
    if ( req.pixelSize == -1 )
	req.pixelSize = req.pointSize/10;
    if ( req.pointSize == -1 )
	req.pointSize = req.pixelSize*10;

    if ( ! engineData ) {
	QFontCache::Key key( req, QFont::NoScript, screen );

	// look for the requested font in the engine data cache
	engineData = QFontCache::instance->findEngineData( key );

	if ( ! engineData ) {
	    // create a new one
	    engineData = new QFontEngineData;
	    QFontCache::instance->insertEngineData( key, engineData );
	} else {
	    engineData->ref();
	}
    }

    // the cached engineData could have already loaded the engine we want
    if ( engineData->engine ) return;

    // load the font
    engineData->engine = new QFontEngine(request);
    engineData->engine->ref();
}


/*****************************************************************************
  QFontMetrics member functions
 *****************************************************************************/

int QFontMetrics::leftBearing(QChar ch) const
{
    return memorymanager->lockGlyphMetrics(this->d->engineData->engine->handle(),ch)->bearingx;
}


int QFontMetrics::rightBearing(QChar ch) const
{
    QGlyphMetrics *metrics = memorymanager->lockGlyphMetrics(this->d->engineData->engine->handle(),ch);
    return metrics->advance - metrics->width - metrics->bearingx;
}

int QFontMetrics::charWidth( const QString &str, int pos ) const
{
    QTextEngine layout( str,  d );
    layout.itemize( QTextEngine::WidthOnly );
    return layout.width( pos, 1 );
}

int QFontMetrics::width( QChar ch ) const
{
    if ( ch.category() == QChar::Mark_NonSpacing ) return 0;

    return memorymanager->lockGlyphMetrics(this->d->engineData->engine->handle(),ch)->advance;
}

// ### should maybe use the common method aswell, but since that one is quite a bit slower
// and we currently don't support complex text rendering on embedded, leave it as is.
int QFontMetrics::width( const QString &str, int len ) const
{
    if ( len < 0 )
	len = str.length();
    int ret=0;
    for (int i=0; i<len; i++)
	ret += memorymanager->lockGlyphMetrics(this->d->engineData->engine->handle(), str[i])->advance;
    return ret;
}

QRect QFontMetrics::boundingRect( const QString &str, int len ) const
{
    return QRect( 0,-(ascent()),width(str,len),height());
}

/*!
    Saves the glyphs in the font that have previously been accessed as
    a QPF file. If \a all is TRUE (the default), then before saving,
    all glyphs are marked as used.

    If the font is large and you are sure that only a subset of
    characters will ever be required on the target device, passing
    FALSE for the \a all parameter can save a significant amount of
    disk space.

    Note that this function is only applicable on Qt/Embedded.
*/
void QFont::qwsRenderToDisk(bool all)
{
#ifndef QT_NO_QWS_SAVEFONTS
    memorymanager->savePrerenderedFont(handle(), all);
#endif
}
