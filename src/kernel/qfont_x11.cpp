/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qfont_x11.cpp#72 $
**
** Implementation of QFont, QFontMetrics and QFontInfo classes for X11
**
** Created : 940515
**
** Copyright (C) 1994-1996 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#include "qwidget.h"
#include "qpainter.h"
#include "qfontdta.h"
#include "qcache.h"
#include "qdict.h"
#include <ctype.h>
#include <stdlib.h>
#define GC GC_QQQ
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>

RCSTAG("$Id: //depot/qt/main/src/kernel/qfont_x11.cpp#72 $");


static const int fontFields = 14;

enum FontFieldNames {				// X LFD fields
    Foundry,
    Family,
    Weight_,
    Slant,
    Width,
    AddStyle,
    PixelSize,
    PointSize,
    ResolutionX,
    ResolutionY,
    Spacing,
    AverageWidth,
    CharsetRegistry,
    CharsetEncoding };


// Internal functions

static QFontInternal *loadXFont( const QString &fontName, bool *newFont );
static bool	parseXFontName( QString &fontName, char **tokens );
static QString	bestFitFamily( const QString & );
static char   **getXFontNames( const char *pattern, int *count );
static bool	smoothlyScalable( const char *fontName );
static bool	fontExists( const char *fontName );
static int	computeLineWidth( const char *fontName );
static int	getWeight( const char *weightString, bool adjustScore=FALSE );


// QFont_Private accesses QFont protected functions

class QFont_Private : public QFont
{
public:
    int	    fontMatchScore( char *fontName, QString &buffer,
			    float *pointSizeDiff, int *weightDiff,
			    bool *scalable, bool *polymorphic,
			    int *resx, int *resy );
    QString bestMatch( const QString &pattern, int *score );
    QString bestFamilyMember( const QString &family, int *score );
    QString findFont( bool *exact );
};

#define PRIV ((QFont_Private*)this)


/*****************************************************************************
  QFontInternal contains X11 font data: an XLFD font name ("-*-*-..") and
  an X font struct.

  Two global dictionaries and a cache hold QFontInternal objects, which
  are shared between all QFonts.
  This mechanism makes font loading much faster than using XLoadQueryFont.
 *****************************************************************************/

class QFontInternal
{
public:
   ~QFontInternal();
    bool	 dirty() const;
    const char  *name()  const;
    XFontStruct *fontStruct() const;
    void	 setFontStruct( XFontStruct * );
    void	 reset();
private:
    QFontInternal( const char * );
    QString	 n;
    XFontStruct *f;
    friend QFontInternal *loadXFont( const QString &, bool * );
};

inline QFontInternal::QFontInternal( const char *name )
    : n(name)
{
}

inline QFontInternal::~QFontInternal()
{
    if ( f )
	XFreeFont(QPaintDevice::x__Display(), f);
}

inline bool QFontInternal::dirty() const
{
    return f == 0;
}

inline const char *QFontInternal::name() const
{
    return n;
}

inline XFontStruct *QFontInternal::fontStruct() const
{
    return f;
}

inline void QFontInternal::reset()
{
    if ( f ) {
	XFreeFont( QPaintDevice::x__Display(), f );
	f = 0;
    }
}


static const int reserveCost   = 1024*100;
static const int fontCacheSize = 1024*1024*4;


Q_DECLARE(QCacheM,QFontInternal);		// inherited by QFontCache
typedef Q_DECLARE(QCacheIteratorM,QFontInternal) QFontCacheIt;
typedef Q_DECLARE(QDictM,QFontInternal)		 QFontDict;
typedef Q_DECLARE(QDictIteratorM,QFontInternal)  QFontDictIt;


class QFontCache : public QCacheM(QFontInternal)
{
public:
    QFontCache( int maxCost, int size=17, bool cs=TRUE, bool ck=TRUE )
	: QCacheM(QFontInternal)(maxCost,size,cs,ck) {}
    void deleteItem( GCI );
};

void QFontCache::deleteItem( GCI d )
{
    QFontInternal *fin = (QFontInternal *)d;
    fin->reset();
}


struct QXFontName
{
    QString name;
    bool    exactMatch;
};

typedef Q_DECLARE(QDictM,QXFontName) QFontNameDict;

static QFontCache    *fontCache	     = 0;	// cache of loaded fonts
static QFontDict     *fontDict	     = 0;	// dict of all loaded fonts
static QFontNameDict *fontNameDict   = 0;	// dict of matched font names
QFont		     *QFont::defFont = 0;	// default font


/*****************************************************************************
  QFontData member functions
 *****************************************************************************/

QFontData::QFontData()
{
    fin = 0;
}

QFontData::~QFontData()
{
  // Font data is cleaned up by font cache and font dict
}

QFontData::QFontData( const QFontData &d )
{
    req = d.req;
    act = d.act;
    exactMatch = d.exactMatch;
    lineW = d.lineW;
    fin = d.fin;				// safe to copy
}


QFontData &QFontData::operator=( const QFontData &d )
{
    req = d.req;
    act = d.act;
    exactMatch = d.exactMatch;
    lineW = d.lineW;
    fin = d.fin;				// safe to copy
    return *this;
}


//
// This function returns the X font struct for a QFontData.
// It is called from QPainter::drawText().
//

XFontStruct *qt_get_xfontstruct( QFontData *d )
{
    return d->fin->fontStruct();
}


/*****************************************************************************
  QFont member functions
 *****************************************************************************/

/*!
  Internal function that initializes the font system.
*/

void QFont::initialize()
{
    fontCache = new QFontCache( fontCacheSize );// create font cache
    CHECK_PTR( fontCache );
    fontDict  = new QFontDict( 29 );		// create font dict
    CHECK_PTR( fontDict );
    fontNameDict = new QFontNameDict( 29 );	// create font name dictionary
    CHECK_PTR( fontNameDict );
    fontNameDict->setAutoDelete( TRUE );
    if ( !defFont )
	defFont = new QFont( TRUE );		// create the default font
}

/*!
  Internal function that cleans up the font system.
*/

void QFont::cleanup()
{
    fontDict->setAutoDelete( TRUE );
    delete defFont;
    defFont = 0;
    delete fontCache;
    delete fontDict;
    delete fontNameDict;
}

/*!
  Internal function that dumps font cache statistics.
*/

void QFont::cacheStatistics()
{
#if defined(DEBUG)
    fontCache->statistics();
    QFontCacheIt it(*fontCache);
    QFontInternal *fin;
    debug( "{" );
    while ( (fin = it.current()) ) {
	++it;
	debug( "   [%s]", fin->name() );
    }
    debug( "}" );
#endif
}


/*!
  \internal
  Constructs a font object that refers to the default font.
*/

QFont::QFont( bool )
{
    init();
    d->req.family    = "6x13";
    d->req.pointSize = 11*10;
    d->req.weight    = QFont::Normal;
    d->req.rawMode   = TRUE;
}


// If d->req.dirty is not TRUE the font must have been loaded
// and we can safely assume that d->fin is a valid pointer:

#define DIRTY_FONT (d->req.dirty || d->fin->dirty())


/*!
  Returns the window system handle to the font, for low-level
  access.  <em>Using this function is not portable.</em>
*/

HANDLE QFont::handle( HANDLE ) const
{
    static Font last = 0;
    if ( DIRTY_FONT ) {
	loadFont();
    } else {
	if ( d->fin->fontStruct()->fid != last )
	    fontCache->find( d->fin->name() );
    }
    last = d->fin->fontStruct()->fid;
    return last;
}

/*!
  Returns TRUE if the font attributes have been changed and the font has to
  be (re)loaded, or FALSE if no changes have been made.
*/

bool QFont::dirty() const
{
    return DIRTY_FONT;
}


/*!
  Returns the family name that corresponds to the current style hint.
*/

QString QFont::defaultFamily() const
{
    switch( d->req.styleHint ) {
	case Times:
	    return "times";
	case Courier:
	    return "courier";
	case Decorative:
	    return "old english";
	case Helvetica:
	case System:
	default:
	    return "helvetica";
    }
}

/*!
  Returns a last resort family name for the \link fontmatch.html font
  matching algorithm. \endlink

  \sa lastResortFont()
*/

QString QFont::lastResortFamily() const
{
    return "helvetica";
}

static const char *tryFonts[] = {
    "6x13",
    "7x13",
    "8x13",
    "9x15",
    "fixed",
    "-*-helvetica-medium-r-*-*-*-120-*-*-*-*-*-*",
    "-*-courier-medium-r-*-*-*-120-*-*-*-*-*-*",
    "-*-times-medium-r-*-*-*-120-*-*-*-*-*-*",
    "-*-lucida-medium-r-*-*-*-120-*-*-*-*-*-*",
    "-*-helvetica-*-*-*-*-*-120-*-*-*-*-*-*",
    "-*-courier-*-*-*-*-*-120-*-*-*-*-*-*",
    "-*-times-*-*-*-*-*-120-*-*-*-*-*-*",
    "-*-lucida-*-*-*-*-*-120-*-*-*-*-*-*",
    "-*-helvetica-*-*-*-*-*-*-*-*-*-*-*-*",
    "-*-courier-*-*-*-*-*-*-*-*-*-*-*-*",
    "-*-times-*-*-*-*-*-*-*-*-*-*-*-*",
    "-*-lucida-*-*-*-*-*-*-*-*-*-*-*-*",
    0 };

/*!
  Returns a last resort raw font name for the \link fontmatch.html font
  matching algorithm. \endlink

  This is used if not even the last resort family is available.

  \sa lastResortFamily()
*/

QString QFont::lastResortFont() const
{
    static const char *last = 0;
    if ( last )					// already found
	return last;
    int i = 0;
    const char *f;
    while ( (f = tryFonts[i]) ) {
	if ( fontExists(f) ) {
	    last = f;
	    return last;
	}
	i++;
    }
#if defined(CHECK_NULL)
    fatal( "QFont::lastResortFont: Cannot find any reasonable font" );
#endif
    return last;
}


static void resetFontDef( QFontDef *def )	// used by updateFontInfo()
{
    def->pointSize     = 0;
    def->styleHint     = QFont::AnyStyle;
    def->weight	       = QFont::Normal;
    def->italic	       = FALSE;
    def->charSet       = QFont::Latin1;
    def->underline     = FALSE;
    def->strikeOut     = FALSE;
    def->fixedPitch    = FALSE;
    def->hintSetByUser = FALSE;
}

/*!
  Updates the font information.
*/

void QFont::updateFontInfo() const
{
    if ( !d->act.dirty )
	return;
    if ( DIRTY_FONT )
	loadFont();
    if ( exactMatch() ) {			// match: copy font description
	d->act = d->req;
	return;
    }
    char *tokens[fontFields];
    QString buffer( 256 );			// holds parsed X font name

    buffer = d->fin->name();
    if ( !parseXFontName( buffer, tokens ) ) {
	resetFontDef( &d->act );
	d->exactMatch  = FALSE;
	d->act.family  = d->fin->name();
	d->act.rawMode = TRUE;
	d->act.dirty   = FALSE;
	return;					// not an XLFD name
    }

    d->act.family      = tokens[Family];
    d->act.pointSize   = atoi( tokens[PointSize] );
    d->act.styleHint   = QFont::AnyStyle;    // ### any until we match families

    if ( strcmp( tokens[CharsetRegistry], "iso8859" ) == 0 &&
	 strcmp( tokens[CharsetEncoding], "1"	    ) == 0 )
	d->act.charSet = QFont::Latin1;
    else
	d->act.charSet = QFont::AnyCharSet;

    char slant	       = tolower( tokens[Slant][0] );
    d->act.italic      = ( slant == 'o' || slant == 'i' ) ? TRUE : FALSE;

    char fixed	       = tolower( tokens[Spacing][0] );
    d->act.fixedPitch  = ( fixed == 'm' || fixed == 'c' ) ? TRUE : FALSE;

    d->act.weight      = getWeight( tokens[Weight_] );

    if ( strcmp( tokens[ResolutionY], "75") != 0 ) {	// if not 75 dpi
	d->act.pointSize = ( 2*d->act.pointSize*atoi(tokens[ResolutionY]) + 1 )
			  / ( 75 * 2 );		// adjust actual pointsize
    }
    d->act.underline = d->req.underline;
    d->act.strikeOut = d->req.strikeOut;
    d->act.rawMode = FALSE;
    d->act.dirty   = FALSE;
}


/*!
  Loads the requested font.
*/

void QFont::loadFont( HANDLE ) const
{
    if ( !fontNameDict || !fontCache )		// not initialized
	return;

    QString instanceID = key();
    QXFontName *fn = fontNameDict->find( instanceID );
    if ( !fn ) {
	fn = new QXFontName;
	CHECK_PTR( fn );
	if ( d->req.rawMode ) {
	    if ( fontExists(family()) ) {
		fn->name = family();
		fn->exactMatch = TRUE;
	    } else {
		fn->name = lastResortFont();
		fn->exactMatch = FALSE;
	    }
	} else {				// get loadable font
	    fn->name = PRIV->findFont( &fn->exactMatch );
	}
	fontNameDict->insert( instanceID, fn );
    }
    bool newFont;
    d->exactMatch = fn->exactMatch;
    d->fin = loadXFont( fn->name, &newFont );
    if ( !d->fin ) {
	d->exactMatch = FALSE;
	d->fin = loadXFont( lastResortFont(), &newFont );
#if defined(CHECK_NULL)
	if ( !d->fin )
	    fatal( "QFont::loadFont: Internal error" );
#endif
    }
    d->lineW = computeLineWidth( d->fin->name() );
    d->req.dirty = FALSE;
    d->act.dirty = TRUE;	// actual font information no longer valid
}


/*****************************************************************************
  QFont_Private member functions
 *****************************************************************************/

#define exactScore	 0xffff

#define CharSetScore	 0x40
#define PitchScore	 0x20
#define ResolutionScore	 0x10
#define SizeScore	 0x08
#define WeightScore	 0x04
#define SlantScore	 0x02
#define WidthScore	 0x01

//
// Returns a score describing how well a font name matches the contents
// of a font.
//

int QFont_Private::fontMatchScore( char	 *fontName,	 QString &buffer,
				   float *pointSizeDiff, int  *weightDiff,
				   bool	 *scalable     , bool *polymorphic,
                                   int   *resx	       , int  *resy )
{
    char *tokens[fontFields];
    bool   exactMatch = TRUE;
    int	   score      = 0;
    *scalable	      = FALSE;
    *polymorphic      = FALSE;
    *weightDiff	      = 0;
    *pointSizeDiff    = 0;

    strcpy( buffer.data(), fontName );	// NOTE: buffer must be large enough
    if ( !parseXFontName( buffer, tokens ) )
	return 0;	// Name did not conform to X Logical Font Description

    if ( strncmp( tokens[CharsetRegistry], "ksc", 3 ) == 0 &&
		  isdigit( tokens[CharsetRegistry][3] )	  ||
	 strncmp( tokens[CharsetRegistry], "jisx", 4 ) == 0  &&
		  isdigit( tokens[CharsetRegistry][4] )	  ||
	 strncmp( tokens[CharsetRegistry], "gb", 2 ) == 0  &&
		  isdigit( tokens[CharsetRegistry][2] ) ) {
	     return 0; // Dirty way of avoiding common 16 bit charsets ###
    }

#undef	IS_ZERO
#define IS_ZERO(X) (X[0] == '0' && X[1] == 0)

    if ( IS_ZERO(tokens[Weight_]) ||
	 IS_ZERO(tokens[Slant])	  ||
	 IS_ZERO(tokens[Width]) )
	*polymorphic = TRUE;			// polymorphic font

    if ( IS_ZERO(tokens[PixelSize]) &&
	 IS_ZERO(tokens[PointSize]) &&
	 IS_ZERO(tokens[AverageWidth]) )
	*scalable = TRUE;			// scalable font

    switch( charSet() ) {
	case Latin1 :
	    if ( strcmp( tokens[CharsetRegistry], "iso8859" ) == 0 &&
		 strcmp( tokens[CharsetEncoding], "1"	 ) == 0 )
		score |= CharSetScore;
	    else
		exactMatch = FALSE;
	    break;
	case AnyCharSet :
	    score |= CharSetScore;
	    break;
    }

    char pitch = tolower( tokens[Spacing][0] );
    if ( fixedPitch() ) {
	if ( pitch == 'm' || pitch == 'c' )
	    score |= PitchScore;
	else
	    exactMatch = FALSE;
    } else {
	if ( pitch != 'p' )
	    exactMatch = FALSE;
    }

    int pSize;

    if ( *scalable ) {
	pSize = deciPointSize();	// scalable font
    } else {
	pSize = atoi( tokens[PointSize] );
    } 


    if ( strcmp(tokens[ResolutionX], "0") == 0 &&
	   strcmp(tokens[ResolutionY], "0") == 0 ) {
	                                // smoothly scalable font
	score |= ResolutionScore;	// know we can ask for any resolution
    } else {
	int localResx = atoi(tokens[ResolutionX]);
	int localResy = atoi(tokens[ResolutionY]);
	if ( *resx == 0 || *resy == 0 ) {
	    *resx = localResx;
	    *resy = localResy;
	}
	if ( localResx == *resx && localResy == *resy ) 
	    score |= ResolutionScore;
	else
	    exactMatch = FALSE;
    }

    float diff;
    if ( deciPointSize() != 0 )
	diff = ((float)QABS(pSize - deciPointSize())/deciPointSize())*100.0F;
    else
	diff = (float)pSize;

    if ( diff < 20 ) {
	if ( pSize != deciPointSize() )
	    exactMatch = FALSE;
	score |= SizeScore;
    } else {
	exactMatch = FALSE;
    }
    if ( pointSizeDiff )
	*pointSizeDiff = diff;

    int weightVal = getWeight( tokens[Weight_], TRUE );

    if ( weightVal == weight() )
	score |= WeightScore;
    else
	exactMatch = FALSE;

    *weightDiff = QABS( weightVal - weight() );
    char slant = tolower( tokens[Slant][0] );
    if ( italic() ) {
	if ( slant == 'o' || slant == 'i' )
	    score |= SlantScore;
	else
	    exactMatch = FALSE;
    } else {
	if ( slant == 'r' )
	    score |= SlantScore;
	else
	    exactMatch = FALSE;
    }
    if ( stricmp( tokens[Width], "normal" ) == 0 )
	score |= WidthScore;
    else
	exactMatch = FALSE;
    return exactMatch ? exactScore : score;
}


struct MatchData {			// internal for bestMatch
    MatchData() { score=0; name=0; pointDiff=99; weightDiff=99; }
    int	    score;
    char   *name;
    float   pointDiff;
    int	    weightDiff;
};

QString QFont_Private::bestMatch( const QString &pattern, int *score )
{
    MatchData	best;
    MatchData	bestScalable;

    QString	matchBuffer( 256 );	// X font name always <= 255 chars
    char **	xFontNames;
    int		count;
    int		sc;
    float	pointDiff;	// difference in % from requested point size
    int		weightDiff;	// difference from requested weight
    int		resx        = 0;
    int         resy	    = 0;
    bool	scalable    = FALSE;
    bool	polymorphic = FALSE;
    int		i;

    xFontNames = getXFontNames( pattern, &count );

    for( i = 0; i < count; i++ ) {
	sc = fontMatchScore( xFontNames[i], matchBuffer,
			     &pointDiff, &weightDiff,
			     &scalable, &polymorphic, &resx, &resy );
	if ( sc > best.score ||
	    sc == best.score && pointDiff < best.pointDiff ||
	    sc == best.score && pointDiff == best.pointDiff &&
				weightDiff < best.weightDiff ) {
	    if ( scalable ) {
		if ( sc > bestScalable.score ||
		     sc == bestScalable.score &&
			   weightDiff < bestScalable.weightDiff ) {
		    bestScalable.score	    = sc;
		    bestScalable.name	    = xFontNames[i];
		    bestScalable.pointDiff  = pointDiff;
		    bestScalable.weightDiff = weightDiff;
		}
	    } else {
		best.score	= sc;
		best.name	= xFontNames[i];
		best.pointDiff	= pointDiff;
		best.weightDiff = weightDiff;
	    }
	}
    }
    QString bestName;
    char *tokens[fontFields];

    if ( best.score != exactScore && ( bestScalable.score > best.score ||
	     bestScalable.score == best.score && (
		 best.pointDiff != 0 ||
		 best.weightDiff < bestScalable.weightDiff ) ) ) {
	if ( smoothlyScalable( bestScalable.name ) ) {
	    if ( resx == 0 || resy == 0 ) {
		resx = 75;
		resy = 75;
	    }
	    best.score = bestScalable.score;
	    strcpy( matchBuffer.data(), bestScalable.name );
	    if ( parseXFontName( matchBuffer, tokens ) ) {
		bestName.sprintf( "-%s-%s-%s-%s-%s-%s-*-%i-%i-%i-%s-*-%s-%s",
				  tokens[Foundry],
				  tokens[Family],
				  tokens[Weight_],
				  tokens[Slant],
				  tokens[Width],
				  tokens[AddStyle],
				  deciPointSize(),
				  resx, resy,
				  tokens[Spacing],
				  tokens[CharsetRegistry],
				  tokens[CharsetEncoding] );
		best.name = bestName.data();
	    }
	}
    }
    *score = best.score;
    bestName = best.name;
    XFreeFontNames( xFontNames );

    return bestName;
}


QString QFont_Private::bestFamilyMember( const QString &family, int *score )
{
    QString pattern;
    QString match;
    pattern.sprintf( "-*-%s-*-*-*-*-*-*-*-*-*-*-*-*", family.data() );
    return bestMatch( pattern, score );
}


QString QFont_Private::findFont( bool *exact )
{
    QString familyName;
    QString bestName;
    int	    score;
    const char *fam = family();			// fam = font family name

    if ( fam == 0 || fam[0] == '\0' ) {		// null or empty string
	familyName = defaultFamily();
	*exact	   = FALSE;
    } else {
	familyName = bestFitFamily( fam );
	*exact	   = TRUE;
    }
    bestName = bestFamilyMember( familyName, &score );
    if ( *exact && score != exactScore )
	*exact = FALSE;

    if ( score == 0 ) {
	QString df = defaultFamily();
	if( familyName != df ) {
	    familyName = df;			// try default family for style
	    bestName   = bestFamilyMember( familyName, &score );
	}
    }
    if ( score == 0 ) {
	QString lrf = lastResortFamily();
	if ( familyName != lrf ) {
	    familyName = lrf;			// try system default family
	    bestName   = bestFamilyMember( familyName, &score );
	}
    }
    if ( bestName.isNull() ) {			// no matching fonts found
	bestName = lastResortFont();
    }
    return bestName;
}


/*****************************************************************************
  QFontMetrics member functions
 *****************************************************************************/

/*!
  Returns the maximum ascent of the font.

  The ascent is the distance from the base line to the uppermost line
  where pixels may be drawn.

  \sa descent()
*/

int QFontMetrics::ascent() const
{
    if ( w == 0 && p == 0 ) {
#if defined(CHECK_STATE)
	warning( "QFontMetrics: Invalid font metrics" );
#endif
	return 0;
    }
    QFont fnt = w ? w->font() : p->font();
    fnt.handle();
    return fnt.d->fin->fontStruct()->max_bounds.ascent;
}


/*!
  Returns the maximum descent of the font.

  The descent is the distance from the base line to the lowermost line
  where pixels may be drawn. (Note that this is different from X, which
  adds 1 pixel.)

  \sa ascent()
*/

int QFontMetrics::descent() const
{
    if ( w == 0 && p == 0 ) {
#if defined(CHECK_STATE)
	warning( "QFontMetrics: Invalid font metrics" );
#endif
	return 0;
    }
    QFont fnt = w ? w->font() : p->font();
    fnt.handle();
    return fnt.d->fin->fontStruct()->max_bounds.descent - 1;
}


/*!
  Returns the height of the font.

  This is always equal to ascent()+descent()+1 (the 1 is for the base line).

  \sa leading(), lineSpacing()
*/

int QFontMetrics::height() const
{
    if ( w == 0 && p == 0 ) {
#if defined(CHECK_STATE)
	warning( "QFontMetrics: Invalid font metrics" );
#endif
	return 0;
    }
    QFont fnt = w ? w->font() : p->font();
    fnt.handle();
    XFontStruct *f = fnt.d->fin->fontStruct();
    return f->max_bounds.ascent + f->max_bounds.descent;
}


/*!
  Returns the leading of the font.

  This is the natural inter-line spacing.

  \sa height(), lineSpacing()
*/

int QFontMetrics::leading() const
{
    if ( w == 0 && p == 0 ) {
#if defined(CHECK_STATE)
	warning( "QFontMetrics: Invalid font metrics" );
#endif
	return 0;
    }
    QFont fnt = w ? w->font() : p->font();
    fnt.handle();
    XFontStruct *f = fnt.d->fin->fontStruct();
    int l = f->ascent		 + f->descent -
	    f->max_bounds.ascent - f->max_bounds.descent;
    return l > 0 ? l : 0;
}


/*!
  Returns the distance from one base line to the next.

  This value is always equal to leading()+height().

  \sa height(), leading()
*/

int QFontMetrics::lineSpacing() const
{
    return leading() + height();
}


/*!
  Returns the width in pixels of the first \e len characters of \e str.

  If \e len is negative (default value), the whole string is used.

  Note that this value is \e not equal to boundingRect().width();
  boundingRect() returns a rectangle describing the pixels this string
  will cover whereas width() returns the distance to where the next string
  should be drawn.  Thus, width(stra)+width(strb) is always equal to
  width(strcat(stra, strb)).  This is almost never the case with
  boundingRect().

  \sa boundingRect()
*/

int QFontMetrics::width( const char *str, int len ) const
{
    if ( w == 0 && p == 0 ) {
#if defined(CHECK_STATE)
	warning( "QFontMetrics: Invalid font metrics" );
#endif
	return 0;
    }
    QFont fnt = w ? w->font() : p->font();
    fnt.handle();
    if ( len < 0 )
	len = strlen( str );
    return XTextWidth( fnt.d->fin->fontStruct(), str, len );
}


/*!
  Returns the bounding rectangle of the first \e len characters of \e str.

  If \e len is negative (default value), the whole string is used.

  Note that the bounding rectangle may extend to the left of (0,0) and
  that the text output may cover \e all pixels in the bounding rectangle.

  \sa width()
*/

QRect QFontMetrics::boundingRect( const char *str, int len ) const
{
    if ( w == 0 && p == 0 ) {
#if defined(CHECK_STATE)
	warning( "QFontMetrics: Invalid font metrics" );
#endif
	return QRect(0,0,0,0);
    }
    QFont fnt = w ? w->font() : p->font();
    fnt.handle();
    if ( len < 0 )
	len = strlen( str );
    XFontStruct *f = fnt.d->fin->fontStruct();
    int direction;
    int ascent;
    int descent;
    XCharStruct overall;

    XTextExtents( f, str, len, &direction, &ascent, &descent, &overall );
    int startX = overall.lbearing;
    int width  = overall.rbearing - startX;
    ascent     = overall.ascent;
    descent    = overall.descent;
    if ( !fnt.d->req.underline && !fnt.d->req.strikeOut ) {
	width  = overall.rbearing - startX;
    } else {
	if ( startX > 0 )
	    startX = 0;
	if ( overall.rbearing < overall.width )
	   width =  overall.width - startX;
	else
	   width =  overall.rbearing - startX;
	if ( fnt.d->req.underline && len != 0 ) {
	    int ulTop = underlinePos();
	    int ulBot = ulTop + lineWidth(); // X descent is 1
	    if ( descent < ulBot )	// more than logical descent, so don't
		descent = ulBot;	// subtract 1 here!
	    if ( ascent < -ulTop )
		ascent = -ulTop;
	}
	if ( fnt.d->req.strikeOut && len != 0 ) {
	    int soTop = strikeOutPos();
	    int soBot = soTop - lineWidth(); // same as above
	    if ( descent < -soBot )
		descent = -soBot;
	    if ( ascent < soTop )
		ascent = soTop;
	}
    }
    return QRect( startX, -ascent, width, descent + ascent );
}


/*!
  Returns the width of the widest character in the font.
*/

int QFontMetrics::maxWidth() const
{
    if ( w == 0 && p == 0 ) {
#if defined(CHECK_STATE)
	warning( "QFontMetrics: Invalid font metrics" );
#endif
	return 0;
    }
    QFont fnt = w ? w->font() : p->font();
    fnt.handle();
    return fnt.d->fin->fontStruct()->max_bounds.width;
}


/*!
  Returns the distance from the base line to where an underscore should be
  drawn.
  \sa strikeOutPos(), lineWidth()
*/

int QFontMetrics::underlinePos() const
{
    int pos = ( lineWidth()*2 + 3 )/6;
    return pos ? pos : 1;
}


/*!
  Returns the distance from the base line to where the strike-out line
  should be drawn.
  \sa underlinePos(), lineWidth()
*/

int QFontMetrics::strikeOutPos() const
{
    if ( w == 0 && p == 0 ) {
#if defined(CHECK_STATE)
	warning( "QFontMetrics: Invalid font metrics" );
#endif
	return 0;
    }
    QFont fnt = w ? w->font() : p->font();
    fnt.handle();
    int pos = fnt.d->fin->fontStruct()->max_bounds.ascent/3;
    return pos ? pos : 1;
}


/*!
  Returns the width of the underline and strike-out lines, adjusted for
  the point size of the font.
  \sa underlinePos(), strikeOutPos()
*/

int QFontMetrics::lineWidth() const
{
    if ( w == 0 && p == 0 ) {
#if defined(CHECK_STATE)
	warning( "QFontMetrics: Invalid font metrics" );
#endif
	return 0;
    }
    QFont f = w ? w->font() : p->font();
    f.handle();
    return f.d->lineW;
}


/*****************************************************************************
  Internal X font functions
 *****************************************************************************/

//
// Loads an X font
//

static QFontInternal *loadXFont( const QString &fontName, bool *newFont )
{
    if ( !fontCache )
	return 0;

    if ( newFont )
	*newFont = FALSE;
    QFontInternal *fin = fontCache->find( fontName );
    if ( fin )
	return fin;
						// font is not cached
    XFontStruct *f;
    f = XLoadQueryFont( QPaintDevice::x__Display(), fontName );
    if ( !f )
	return 0;				// could not load font

    fin = fontDict->find( fontName );
    if ( fin ) {
	if ( newFont )
	    *newFont = TRUE;
    } else {					// first time font is loaded?
	fin = new QFontInternal( fontName );
	CHECK_PTR( fin );
	fontDict->insert( fontName, fin );
    }
    fin->f = f;
    int sz = (f->max_bounds.ascent + f->max_bounds.descent)
	      *f->max_bounds.width
	      *(f->max_char_or_byte2 - f->min_char_or_byte2) / 8;
    if ( sz > fontCache->maxCost() + reserveCost )	// make room for this
	fontCache->setMaxCost( sz + reserveCost );	//   font only
    if ( !fontCache->insert(fontName, fin, sz) ) {
#if defined(DEBUG)
	fatal( "loadXFont: Internal error; cache overflow" );
#endif
    }
    return fin;
}


//
// Splits an X font name into fields separated by '-'
//

static bool parseXFontName( QString &fontName, char **tokens )
{
    if ( fontName.isEmpty() || fontName[0] != '-' ) {
	tokens[0] = 0;
	return FALSE;
    }
    int	  i;
    char *f = fontName.data() + 1;
    for ( i=0; i<fontFields && f && f[0]; i++ ) {
	tokens[i] = f;
	f = strchr( f, '-' );
	if( f )
	    *f++ = '\0';
    }
    if ( i < fontFields ) {
	for( int j=i ; j<fontFields; j++ )
	    tokens[j] = 0;
	return FALSE;
    }
    return TRUE;
}


//
// Maps a font family name to a valid X family name
//

static QString bestFitFamily( const QString &fam )
{
    QString family = QFont::substitute( fam );
    return family.lower();
}


//
// Get an array of X font names that matches a pattern
//

static char **getXFontNames( const char *pattern, int *count )
{
    static int maxFonts = 256;
    char **list;
    while( 1 ) {
	list = XListFonts( QPaintDevice::x__Display(), (char*)pattern,
			   maxFonts, count );
	if ( *count != maxFonts || maxFonts >= 32768 )
	    return list;
	XFreeFontNames( list );
	maxFonts *= 2;
    }
}


//
// Returns TRUE if the font can be smoothly scaled
//

static bool smoothlyScalable ( const char * /* fontName */  )
{
    return TRUE;
}


//
// Returns TRUE if the font exists, FALSE otherwise
//

static bool fontExists( const char *fontName )
{
    char **fontNames;
    int	   count;
    fontNames = getXFontNames( fontName, &count );
    XFreeFontNames( fontNames );
    return count != 0;
}


//
// Computes the line width (underline,strikeout) for the X font.
//

static int computeLineWidth( const char *fontName )
{
    char *tokens[fontFields];
    QString buffer(256);		// X font name always <= 255 chars
    strcpy( buffer.data(), fontName );
    if ( !parseXFontName(buffer, tokens) )
	return 1;			// name did not conform to X LFD
    int weight = getWeight( tokens[Weight_] );
    int pSize  = atoi( tokens[PointSize] ) / 10;
    if ( strcmp( tokens[ResolutionX], "75") != 0 || // adjust if not 75 dpi
	 strcmp( tokens[ResolutionY], "75") != 0 )
	pSize = ( 2*pSize*atoi(tokens[ResolutionY]) + 75 ) / ( 75 * 2 );
    int score = pSize*weight;		// ad hoc algorithm
    int lw = ( score ) / 700;
    if ( lw < 2 && score >= 1050 )	// looks better with thicker line
	lw = 2;				//   for small pointsizes
    return lw ? lw : 1;
}


//
// Converts a weight string to a value
//

Q_DECLARE(QDictM,int);
QDictM(int) *weightDict = 0;

static void cleanupWeightDict()
{
    delete weightDict;
    weightDict = 0;
}

static int getWeight( const char *weightString, bool adjustScore )
{
    if ( !weightDict ) {
	qAddPostRoutine( cleanupWeightDict );
	weightDict = new QDictM(int)( 17, FALSE ); // case insensitive
	CHECK_PTR( weightDict );
	weightDict->insert( "medium",   (int*)((int)QFont::Normal+1) );
	weightDict->insert( "bold",     (int*)((int)QFont::Bold+1) );
	weightDict->insert( "demibold", (int*)((int)QFont::DemiBold+1) );
	weightDict->insert( "black",    (int*)((int)QFont::Black+1) );
	weightDict->insert( "light",    (int*)((int)QFont::Light+1) );
    }
    int *p = weightDict->find(weightString);
    if ( p )
	return (int)p - 1;
    QString s = weightString;
    s = s.lower();
    if ( s.contains("bold") ) {
	if ( adjustScore )
	    return (int) QFont::Bold - 1;  // - 1, not sure that this IS bold
	else
	    return (int) QFont::Bold;
    }
    if ( s.contains("light") ) {
	if ( adjustScore )
	    return (int) QFont::Light - 1; // - 1, not sure that this IS light
       else
	    return (int) QFont::Light;
    }
    if ( s.contains("black") ) {
	if ( adjustScore )
	    return (int) QFont::Black - 1; // - 1, not sure this IS black
	else
	    return (int) QFont::Black;
    }
    if ( adjustScore )
	return (int) QFont::Normal - 2;	   // - 2, we hope it's close to normal
    else
	return (int) QFont::Normal;
}
