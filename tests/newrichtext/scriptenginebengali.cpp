#include "scriptengine.h"
#include "opentype.h"
#include "qfont.h"
#include "qtextdata.h"
#include "qtextengine.h"

#ifdef None
#undef None
#endif

enum Features {
    NuktaFeature = 0x0002,
    AkhantFeature = 0x0004,
    RephFeature = 0x0008,
    BelowFormFeature = 0x0010,
    HalfFormFeature = 0x0020,
    PostFormFeature = 0x0040,
    VattuFeature = 0x0080,
    PreSubstFeature = 0x0100,
    BelowSubstFeature = 0x0200,
    AboveSubstFeature = 0x0400,
    PostSubstFeature = 0x0800,
    HalantFeature = 0x1000
};

enum Form {
    Invalid = 0x0,
    Unknown = Invalid,
    Consonant,
    Nukta,
    Halant,
    Matra,
    VowelMark,
    StressMark,
    IndependentVowel,
    LengthMark,
    Other,
};

static int bengaliForms[0x80] = {
    Invalid, VowelMark, VowelMark, VowelMark,
    Invalid, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, Invalid, Invalid, IndependentVowel,

    IndependentVowel, Invalid, Invalid, IndependentVowel,
    IndependentVowel, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Invalid, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Invalid, Consonant, Invalid,
    Invalid, Invalid, Consonant, Consonant,
    Consonant, Consonant, Unknown, Unknown,
    Nukta, Other, Matra, Matra,

    Matra, Matra, Matra, Matra,
    Matra, Invalid, Invalid, Matra,
    Matra, Invalid, Invalid, Matra,
    Matra, Halant, Unknown, Unknown,

    Invalid, Invalid, Invalid, Invalid,
    Invalid, Invalid, Invalid, VowelMark,
    Invalid, Invalid, Invalid, Invalid,
    Consonant, Consonant, Invalid, Consonant,

    Other, Other, VowelMark, VowelMark,
    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other
};

static Form form( const QChar &ch ) {
    ushort uc = ch.unicode();
    if ( uc < 0x980 || uc > 0x9ff ) {
	if ( uc == 0x25cc )
	    return Consonant;
	return Other;
    }
    return (Form)bengaliForms[uc-0x980];
}

static bool isRa( const QChar &ch ) {
    return (ch.unicode() == 0x9b0);
}

enum Position {
    None,
    Pre,
    Above,
    Below,
    Post,
    Split
};

static int bengaliPosition[0x80] = {
    None, Above, Post, Post,
    None, None, None, None,
    None, None, None, None,
    None, None, None, None,

    None, None, None, None,
    None, None, None, None,
    None, None, None, None,
    None, None, None, None,

    None, None, None, None,
    None, None, None, None,
    None, None, None, None,
    None, None, None, None,

    None, None, None, None,
    None, None, None, None,
    None, None, None, None,
    Below, None, Post, Pre,

    Post, Below, Below, Below,
    Below, None, None, Pre,
    Pre, None, None, Split,
    Split, Below, None, None,

    None, None, None, None,
    None, None, None, Post,
    None, None, None, None,
    None, None, None, None,

    None, None, Below, Below,
    None, None, None, None,
    None, None, None, None,
    None, None, None, None,

    None, None, None, None,
    None, None, None, None,
    None, None, None, None,
    None, None, None, None,
};

static inline Position position( const QChar &ch ) {
    unsigned short uc = ch.unicode();
    if ( uc < 0x900 && uc > 0x97f )
	return None;
    return (Position) bengaliPosition[uc-0x980];
}

/* syllables are of the form:

   (Consonant Nukta? Halant)* Consonant Matra? VowelMark? StressMark?
   (Consonant Nukta? Halant)* Consonant Halant
   IndependentVowel VowelMark? StressMark?

   We return syllable boundaries on invalid combinations aswell
*/
static int nextSyllableBoundary( const QString &s, int start, int end, bool *invalid )
{
    *invalid = FALSE;
//      qDebug("nextSyllableBoundary: start=%d, end=%d", start, end );
    const QChar *uc = s.unicode()+start;

    int pos = 0;
    Form state = form( uc[pos] );
//     qDebug("state[%d]=%d (uc=%4x)", pos, state, uc[pos].unicode() );
    pos++;

    if ( state != Consonant && state != IndependentVowel ) {
	if ( state != Other )
	    *invalid = TRUE;
	goto finish;
    }

    while ( pos < end - start ) {
	Form newState = form( uc[pos] );
//  	qDebug("state[%d]=%d (uc=%4x)", pos, newState, uc[pos].unicode() );
	switch( newState ) {
	case Consonant:
	    if ( state == Halant )
		break;
	    goto finish;
	case Halant:
	    if ( state == Nukta || state == Consonant )
		break;
	    goto finish;
	case Nukta:
	    if ( state == Consonant )
		break;
	    goto finish;
	case StressMark:
	    if ( state == VowelMark )
		break;
	    // fall through
	case VowelMark:
	    if ( state == Matra || state == IndependentVowel )
		break;
	    // fall through
	case Matra:
	    if ( state == Consonant || state == Nukta )
		break;
	    // the combination Independent_A + Vowel Sign AA is allowed.
	    if ( uc[pos].unicode() == 0x9be && uc[pos-1].unicode() == 0x985 )
		break;
	    goto finish;

	case LengthMark:
	case IndependentVowel:
	case Invalid:
	case Other:
	    goto finish;
	}
	state = newState;
	pos++;
    }
 finish:
    return pos+start;
}

// vowel matras that have to be split into two parts.
static const unsigned short split_o[2] = { 0x9c7, 0x9be };
static const unsigned short split_au[2] = { 0x9c7, 0x9d7 };

static QString reorderSyllable( const QString &string, int start, int end, unsigned short *featuresToApply,
				GlyphAttributes *attributes, bool invalid )
{
    int len = end - start;

    QString reordered = string.mid( start, len );
    if ( invalid ) {
	reordered = QChar( 0x25cc ) + reordered;
	len++;
    }

    // in case mid() returns the whole string!
    reordered.setLength( reordered.length() );

    QChar *uc = (QChar *)reordered.unicode();

    for ( int i = 0; i < len; i++ )
	featuresToApply[i] = 0;

    // We can do this rule at the beginning, as it doesn't interact with later operations.
    // Rule 4: split two part matras into parts
    // This could be done better, but works for now.
    for ( int i = 0; i < len; i++ ) {
	const QChar *split = 0;
	if ( uc[i].unicode() == 0x9cb )
	    split = (const QChar *)split_o;
	else if ( uc[i].unicode() == 0x9cc )
	    split = (const QChar *)split_au;
	if ( split ) {
	    reordered.replace( i, 1, split, 2 );
	    uc = (QChar *)reordered.unicode();
	    len++;
	    break;
	}
    }
//     qDebug("length=%d",  len );

    // nothing to do in this case!
    if ( len == 1 ) {
	attributes[0].mark = (category( reordered.unicode()[0] ) == QChar::Mark_NonSpacing);
	attributes[0].clusterStart = FALSE;
	return reordered;
    }

    int base = 0;
    if ( form( *uc ) == Consonant ) {
	bool reph = FALSE;
	if ( len > 2 && isRa( uc[0] ) && form( uc[1] ) == Halant ) {
	    reph = TRUE;
// 	    qDebug("Reph");
	}

	// Rule 1: find base consonant
	int lastConsonant = 0;
	for ( int i = len-1; i > 0; i-- ) {
	    if ( form( uc[i] ) == Consonant ) {
		if ( !lastConsonant )
		    lastConsonant = i;
		// ### The MS specs says, that this should be done only if the syllable starts with a reph,
		// but they seem to act differently.
		if ( /*!reph ||*/ !isRa( uc[i] ) ) {
		    base = i;
		    break;
		}
	    }
	}
	if ( reph && base == 0 )
	    base = lastConsonant;

// 	qDebug("base consonant at %d skipped=%s", base, lastConsonant != base ? "true" :"false" );

	// Rule 2: move halant of base consonant to last one. Only
	// possible if we skipped consonants while finding the base
	if ( lastConsonant != base && form( uc[base+1] ) == Halant ) {
// 	    qDebug("moving halant from %d to %d!", base+1, lastConsonant);
	    QChar halant = uc[base+1];
	    for ( int i = base+1; i < lastConsonant; i++ )
		uc[i] = uc[i+1];
	    uc[lastConsonant] = halant;

	}

	// Rule 3: Move reph to follow post base matra
	if ( reph ) {
	    int toPos = base+1;
	    if ( toPos < len && form( uc[toPos] ) == Nukta )
		toPos++;
	    // doing this twice takes care of split matras.
	    if ( toPos < len && form( uc[toPos] ) == Matra )
		toPos++;
	    if ( toPos < len && form( uc[toPos] ) == Matra )
		toPos++;
// 	    qDebug("moving reph from %d to %d", 0, toPos );
	    QChar ra = uc[0];
	    QChar halant = uc[1];
	    for ( int i = 2; i < toPos; i++ )
		uc[i-2] = uc[i];
	    uc[toPos-2] = ra;
	    uc[toPos-1] = halant;
	    featuresToApply[toPos-2] |= RephFeature;
	    featuresToApply[toPos-1] |= RephFeature;
	    base -= 2;
	}
    }

    // Rule 5: identify matra position. there are no post/below base consonats
    // in devanagari except for [Ra Halant]_Vattu, but these are already at the
    // right position

    // all reordering happens now to the chars after (base+(reph halant)_vattu?)
    // so we move base to there
    int fixed = base+1;
    if ( fixed < len && form( uc[fixed] ) == Nukta )
	fixed++;
    if ( fixed < len - 1 && isRa( uc[fixed] ) && form( uc[fixed+1] ) == Halant )
	fixed += 2;


    // we continuosly position the matras and vowel marks and increase the fixed
    // until we reached the end.
    static struct {
	Form form;
	Position position;
    } finalOrder [] = {
	{ Matra, Pre },
	{ Matra, Below },
	{ VowelMark, Below },
	{ StressMark, Below },
	{ Matra, Above },
	{ Matra, Post },
	{ Consonant, None },
	{ Halant, None },
	{ VowelMark, Above },
	{ StressMark, Above },
	{ VowelMark, Post },
	{ (Form)0, None }
    };

//      qDebug("base=%d fixed=%d", base, fixed );
    int toMove = 0;
    while ( fixed < len ) {
//  	qDebug("fixed = %d", fixed );
	for ( int i = fixed; i < len; i++ ) {
	    if ( form( uc[i] ) == finalOrder[toMove].form &&
		 position( uc[i] ) == finalOrder[toMove].position ) {
		// need to move this glyph
		int to = fixed;
		if ( finalOrder[toMove].position == Pre )
		    to = 0;
//  		qDebug("moving from %d to %d", i,  to );
		QChar ch = uc[i];
		unsigned short feature = featuresToApply[i];
		for ( int j = i; j > to; j-- ) {
		    uc[j] = uc[j-1];
		    featuresToApply[j] = featuresToApply[j-1];
		}
		uc[to] = ch;
		switch( finalOrder[toMove].position ) {
		case Pre:
// 		    feature |= PreSubstFeature;
		    break;
		case Above:
// 		    feature |= AboveSubstFeature;
		    break;
		case Below:
		    feature |= BelowFormFeature;//|BelowSubstFeature;
		    break;
		case Post:
		    feature |= PostSubstFeature;//|PostFormFeature;
		    break;
		case None:
		    break;
		case Split:
		    break;
		}
		featuresToApply[to] = feature;
		fixed++;
	    }
	}
	toMove++;
	if ( finalOrder[toMove].form == 0 )
	    break;
    }

    bool halantForm = base < len-1 && (form( uc[base+1] ) == Halant);
    if ( halantForm ) {
	// #### we have to take care this doesn't get applied to Akhant ligatures,
	// but that's currently rather hard (without a bigger rewrite of the open type
	// API (ftx*.c)
	featuresToApply[base] |= HalantFeature;
	featuresToApply[base+1] |= HalantFeature;
    }

    // set the features we need to apply in OT
    int state = form( uc[0] );
    bool lastWasBase = (base == 0);
    if ( state == Consonant )
	featuresToApply[0] |= AkhantFeature|NuktaFeature;

    for ( int i = 1; i < len; i++ ) {
	int newState = form( uc[i] );
	switch( newState ) {
	case Consonant:
	    lastWasBase = (i == base);
	    featuresToApply[i] |= AkhantFeature|NuktaFeature;
	    break;
	case Halant:
	    if ( state == Nukta || state == Consonant ) {
		// vattu or halant feature
		if ( isRa( uc[i-1] ) && len > 2 ) {
		    if ( !(featuresToApply[i] & RephFeature) ) {
			featuresToApply[i-1] |= BelowFormFeature|VattuFeature;
			featuresToApply[i] |= BelowFormFeature|VattuFeature;
			int j = i-2;
			while ( j >= 0 ) {
			    int f = form( uc[j] );
			    featuresToApply[j] |= VattuFeature;
			    if ( f == Consonant )
				break;
			    j--;
			}
		    }
		}
		else if ( !lastWasBase  ) {
		    if ( state == Nukta )
			featuresToApply[i-2] |= HalfFormFeature;
		    featuresToApply[i-1] |= HalfFormFeature;
		    featuresToApply[i] |= HalfFormFeature;
		}
	    }
	    break;
	case Nukta:
	    if ( state == Consonant ) {
		featuresToApply[i-1] |= NuktaFeature;
		featuresToApply[i] |= NuktaFeature;
	    }
	case StressMark:
	case VowelMark:
	case Matra:
	case LengthMark:
	case IndependentVowel:
	case Invalid:
	case Other:
	    break;
	}
	state = newState;
    }

    for ( int i = 0; i < (int)reordered.length(); i++ ) {
	attributes[i].mark = (category( reordered.unicode()[0] ) == QChar::Mark_NonSpacing);
	attributes[i].clusterStart = FALSE;
    }
    attributes[0].clusterStart = TRUE;

//     qDebug("reordered:");
//     for ( int i = 0; i < (int)reordered.length(); i++ )
// 	qDebug("    %d: %4x apply=%4x clusterStart=%d", i, reordered[i].unicode(), featuresToApply[i], attributes[i].clusterStart );

    return reordered;
}

static QString analyzeSyllables( const QString &string, int from, int length, unsigned short *featuresToApply,
				 GlyphAttributes *attributes ) {
    QString reordered;
    int sstart = from;
    int end = sstart + length;
    int fpos = 0;
    while ( sstart < end ) {
	bool invalid;
	int send = nextSyllableBoundary( string, sstart, end, &invalid );
// 	qDebug("syllable from %d, length %d, invalid=%s", sstart, send-sstart,
// 	       invalid ? "true" : "false" );
	QString str = reorderSyllable( string, sstart, send, featuresToApply+fpos, attributes+fpos, invalid );
	reordered += str;
	fpos += str.length();

	sstart = send;
    }
    return reordered;
}


void QScriptEngineBengali::shape( const QString &string, int from, int len, QScriptItem *item )
{
//     qDebug("QScriptEngineBengali::shape()");

    QShapedItem *shaped = item->shaped;

    unsigned short fa[256];
    unsigned short *featuresToApply = fa;
    if ( len > 127 )
	featuresToApply = new unsigned short[ 2*len ];


    shaped->glyphAttributes = (GlyphAttributes *)realloc( shaped->glyphAttributes, len * 2 * sizeof( GlyphAttributes ) );

    QString reordered = analyzeSyllables( string, from, len, featuresToApply, shaped->glyphAttributes );
    shaped->num_glyphs = reordered.length();

    shaped->logClusters = (unsigned short *) realloc( shaped->logClusters, shaped->num_glyphs * sizeof( unsigned short ) );
    int pos = 0;
    for ( int i = 0; i < shaped->num_glyphs; i++ ) {
	if ( shaped->glyphAttributes[i].clusterStart )
	    pos = i;
	shaped->logClusters[i] = pos;
    }

    shaped->glyphs = (glyph_t *)realloc( shaped->glyphs, shaped->num_glyphs*sizeof( glyph_t ) );
    int error = item->fontEngine->stringToCMap( reordered.unicode(), shaped->num_glyphs, shaped->glyphs, &shaped->num_glyphs );
    if ( error == QFontEngine::OutOfMemory ) {
	shaped->glyphs = (glyph_t *)realloc( shaped->glyphs, shaped->num_glyphs*sizeof( glyph_t ) );
	item->fontEngine->stringToCMap( reordered.unicode(), shaped->num_glyphs, shaped->glyphs, &shaped->num_glyphs );
    }

    QOpenType *openType = item->fontEngine->openTypeIface();

    if ( openType && openType->supportsScript( QFont::Bengali ) ) {
	((QOpenType *) openType)->apply( QFont::Bengali, featuresToApply, item, len );
    } else {
	heuristicSetGlyphAttributes( string, from, len, item );
	calculateAdvances( item );
    }

    if ( len > 127 )
	delete featuresToApply;
}

