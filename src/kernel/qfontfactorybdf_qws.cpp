/****************************************************************************
** $Id: //depot/q/main/src/kernel/qpaintdevice.h#73 $
**
** Definition of QFontFactory for BDF fonts for QT/Embedded
**
** Created : 000427
**
** Copyright (C) 2000 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
** information about the Professional Edition licensing, or see
** http://www.trolltech.com/qpl/ for QPL licensing information.
**
*****************************************************************************/

#include "qfontfactorybdf_qws.h"

#ifndef QT_NO_BDF

#include "qfontdata_p.h"
#include "qtextcodec.h"
#include "qtextstream.h"
#include "qfile.h"
#include "qrect.h"
#include "qstringlist.h"
#include <string.h>
#include <stdio.h>

QString QFontFactoryBDF::name()
{
    return "BDF";
}

static
QTextCodec* calc_mapper(const QString& charset_registry, const QString& charset_encoding)
{
    if ( !charset_registry.isNull() ) {
	QString e = charset_registry+"-"+charset_encoding;
	if ( e != "10646-1" ) // don't map Unicode, we do it faster directly.
	    return QTextCodec::codecForName(e);
    }
    return 0;
}

class QRenderedFontBDF : public QRenderedFont {
    QDiskFont* df;
    int ptsize;
    QGlyph *glyph;
    QChar default_qchar;

public:
    QRenderedFontBDF(QDiskFont* d, const QFontDef &fd) :
	QRenderedFont(d,fd)
    {
	df = d;

	QFile in(df->file);
	in.open(IO_ReadOnly);
	QTextStream s(&in);
	QString charset_registry, charset_encoding;
	int advance=0;
	bool bitmap_mode = FALSE;
	uchar* data=0;
	int datasize = 0;
	int glyph_index=0;
	int num_glyphs=0;
	QRect bbox;
	QTextCodec* mapper=0;
	int default_char = -1;
	default_qchar = 0;

	do {
	    QString line = s.readLine();
	    if ( bitmap_mode ) {
		if ( line == "ENDCHAR" ) {
		    // Crop character (some BDF files are poor)
		    QGlyph* g = glyph+glyph_index;
		    uchar* t;
		    int linestep = g->metrics->linestep;
		    int h = g->metrics->height;
		    int croptop=0,cropbot=0;
		    t = g->data;
		    for (int j=0; j<h; j++) {
			int i;
			for (i=0; i<linestep; i++)
			    if ( *t++ )
				goto donetop;
			if ( i==linestep )
			    croptop++;
		    }
		    donetop:
		    t = g->data+(h-1)*linestep;
		    for (int j=h-1; j>croptop; j--) {
			int i;
			for (i=0; i<linestep; i++)
			    if ( *t++ )
				goto donebot;
			if ( i==linestep )
			    cropbot++;
		    }
		    donebot:
		    if ( croptop || cropbot ) {
			g->metrics->height -= croptop+cropbot;
			g->metrics->bearingy -= croptop;
			int datasize = linestep*g->metrics->height;
			uchar* data = new uchar[datasize];
			memcpy(data,g->data+linestep*croptop,datasize);
			delete [] g->data;
			g->data = data;
		    }
		    bitmap_mode = FALSE;
		    glyph_index++;
		} else {
		    for (int i=0; i<(int)line.length(); i+=2)
			if ( datasize ) {
			    --datasize;
			    *data++ = line.mid(i,2).toInt(0,16); 
			}
		}
	    } else {
		QStringList token = QStringList::split(QChar(' '),line);
		QString tag = token[0];
		if ( tag == "BITMAP" ) {
		    if ( glyph_index >= 0 ) {
			int linestep = (bbox.width()+7)/8;
			datasize=linestep*bbox.height();
			data = new uchar[datasize];
			QGlyph* g = glyph+glyph_index;
			g->data = data;
			g->metrics = new QGlyphMetrics;
			g->metrics->advance=advance;
			g->metrics->bearingx=bbox.x();
			g->metrics->bearingy=bbox.y()+bbox.height();
			g->metrics->linestep=linestep;
			g->metrics->width=bbox.width();
			g->metrics->height=bbox.height();
			bitmap_mode = TRUE;
		    }
		} else if ( tag == "ENCODING" ) {
		    int encoding = token[1].toInt();
		    if ( encoding < 0 ) {
			glyph_index = -1;
		    } else {
			if ( mapper ) {
			    QCString c;
			    int e = encoding;
			    while (e) {
				c += char(e&0xff);
				e >>= 8;
			    }
			    glyph_index = mapper->toUnicode(c)[0].unicode();
			} else {
			    // No mapping. Assume Unicode/Latin1/ASCII7
			    glyph_index = encoding;
			}
			if ( encoding == default_char || default_char == -1 )
			    default_qchar = QChar((ushort)glyph_index);
		    }
		} else if ( tag == "DWIDTH" ) {
		    advance = token[1].toInt();
		} else if ( tag == "BBX" ) {
		    bbox = QRect(token[3].toInt(),token[4].toInt(),
				 token[1].toInt(),token[2].toInt());
		} else if ( tag == "CHARSET_REGISTRY" ) {
		    charset_registry = token[1];
		    mapper = calc_mapper(charset_registry,charset_encoding);
		} else if ( tag == "CHARSET_ENCODING" ) {
		    charset_encoding = token[1];
		    mapper = calc_mapper(charset_registry,charset_encoding);
		} else if ( tag == "FONTBOUNDINGBOX" ) {
		    bbox = QRect(token[3].toInt(),token[4].toInt(),
				 token[1].toInt(),token[2].toInt());
		    fmaxwidth = bbox.width();
		} else if ( tag == "FONT_ASCENT" ) {
		    fascent = token[1].toInt();
		} else if ( tag == "FONT_DESCENT" ) {
		    fdescent = token[1].toInt();
		} else if ( tag == "DEFAULT_CHAR" ) {
		    default_char = token[1].toInt();
		} else if ( tag == "CHARS" ) {
		    num_glyphs = token[1].toInt();
		    glyph = new QGlyph[0x10000];
		    for (int i=0; i<0x10000; i++) {
			glyph[i].data = 0;
			glyph[i].metrics = 0;
		    }
		} else if ( tag == "POINT_SIZE" ) {
		    ptsize = token[1].toInt();
		}
	    }
	} while (!s.atEnd());
	if ( !glyph[0].data ) {
	    glyph[0].data = glyph[default_qchar.unicode()].data;
	    glyph[0].metrics = glyph[default_qchar.unicode()].metrics;
	}
    }

    bool inFont(QChar ch) const
    {
	return glyph[ch.unicode()].data;
    }

    QGlyph render(QChar ch)
    {
	const QGlyph& g = glyph[ch.unicode()];
	if ( g.data )
	    return g;
	return glyph[0];
    }
};

QFontFactoryBDF::QFontFactoryBDF()
{
}

QFontFactoryBDF::~QFontFactoryBDF()
{
}


QRenderedFont * QFontFactoryBDF::get(const QFontDef & f,QDiskFont * f2)
{
    QRenderedFontBDF * qrf=new QRenderedFontBDF(f2, f);
    return qrf;
}

void QFontFactoryBDF::load(QDiskFont * qdf) const
{
    if(qdf->loaded)
	return;
    qdf->p=0;
    qdf->loaded=TRUE;
}

bool QFontFactoryBDF::unicode(QRenderedFont *,int &)
{
    qFatal("Why is this still used");
    return TRUE;
}

#endif // QT_NO_BDF
