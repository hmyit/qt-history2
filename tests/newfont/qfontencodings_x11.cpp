#include "qfontencodings_p.h"

#ifndef QT_NO_CODECS

#include <qjpunicode.h>


int QFontJis0208Codec::heuristicContentMatch(const char *, int) const
{
    return 0;
}

const QJpUnicodeConv * QFontJis0208Codec::convJP;

QFontJis0208Codec::QFontJis0208Codec()
{
    if ( !convJP )
	convJP = QJpUnicodeConv::newConverter(JU_Default);
}

const char* QFontJis0208Codec::name() const
{
    return "jisx0208.1983-0";
}

int QFontJis0208Codec::mibEnum() const
{
    return 63;
}

QString QFontJis0208Codec::toUnicode(const char* /*chars*/, int /*len*/) const
{
    return QString(); //###
}

QCString QFontJis0208Codec::fromUnicode(const QString& uc, int& lenInOut ) const
{
    QCString result;
    for ( int i = 0; i < lenInOut; i++ ) {
	QChar ch = uc[i];
	if ( ch.row() == 0) {
	    if ( ch.cell() == ' ' )
		ch = QChar( 0x3000 );
	    else if ( ch.cell() == '"' )
		ch = QChar( 0x2033 );
	    else if ( ch.cell() == '\'' )
		ch = QChar( 0x2032 );
	    else if ( ch.cell() == '-' )
		ch = QChar( 0x2212 );
	    else if ( ch.cell() == '~' )
		ch = QChar( 0x301c );
	    else if ( ch.cell() > ' ' && ch.cell() < 127 )
		ch = QChar( ch.cell()-' ', 255 );
	}
	ch = convJP->UnicodeToJisx0208( ch.unicode());
	if ( !ch.isNull() ) {
	    result += ch.row();
	    result += ch.cell();
	} else {
	    //black square
	    result += 0x22;
	    result += 0x23;
	}
    }
    lenInOut *=2;
    return result;
}


// ----------------------------------------------------------

extern unsigned int qt_UnicodeToKsc5601(unsigned int unicode);

int QFontKsc5601Codec::heuristicContentMatch(const char *, int) const
{
    return 0;
}

QFontKsc5601Codec::QFontKsc5601Codec()
{
}

const char* QFontKsc5601Codec::name() const
{
    return "ksc5601.1987-0";
}

int QFontKsc5601Codec::mibEnum() const
{
    return 63;
}

QString QFontKsc5601Codec::toUnicode(const char* /*chars*/, int /*len*/) const
{
    return QString(); //###
}

QCString QFontKsc5601Codec::fromUnicode(const QString& uc, int& lenInOut ) const
{
    QCString result;
    for ( int i = 0; i < lenInOut; i++ ) {
	QChar ch = uc[i];
	if ( ch.row() == 0) {
	    if ( ch.cell() == ' ' )
		ch = QChar( 0x3000 );
	    else if ( ch.cell() > ' ' && ch.cell() < 127 )
		ch = QChar( ch.cell()-' ', 255 );
	}
	ch = QChar( qt_UnicodeToKsc5601(ch.unicode()) );

	if ( ch.row() > 0 && ch.cell() > 0  ) {
	    result += ch.row() & 0x7f ;
	    result += ch.cell() & 0x7f;
	} else {
	    //black square
	    result += 0x21;
	    result += 0x61;
	}
    }
    lenInOut *=2;
    return result;
}


/********


Name: GB_2312-80                                        [RFC1345,KXS2]
MIBenum: 57
Source: ECMA registry
Alias: iso-ir-58
Alias: chinese
Alias: csISO58GB231280

*/


extern unsigned int qt_UnicodeToGBK(unsigned int code);


int QFontGB2312Codec::heuristicContentMatch(const char *, int) const
{
    return 0;
}

QFontGB2312Codec::QFontGB2312Codec()
{
}

const char* QFontGB2312Codec::name() const
{
    return "gb2312.1980-0";
}

int QFontGB2312Codec::mibEnum() const
{
    return 57;
}

QString QFontGB2312Codec::toUnicode(const char* /*chars*/, int /*len*/) const
{
    return QString(); //###
}

QCString QFontGB2312Codec::fromUnicode(const QString& uc, int& lenInOut ) const
{
    QCString result;
    for ( int i = 0; i < lenInOut; i++ ) {
	QChar ch = uc[i];
	if ( ch.row() == 0) {
	    if ( ch.cell() == ' ' )
		ch = QChar( 0x3000 );
	    else if ( ch.cell() > ' ' && ch.cell() < 127 )
		ch = QChar( ch.cell()-' ', 255 );
	}
	ch = QChar( qt_UnicodeToGBK(ch.unicode()) );

	if ( ch.row() > 0xa0 && ch.cell() > 0xa0  ) {
	    result += ch.row() & 0x7f ;
	    result += ch.cell() & 0x7f;
	} else {
	    //black square
	    result += 0x21;
	    result += 0x76;
	}
    }
    lenInOut *=2;
    return result;
}

// ----------------------------------------------------------------

extern unsigned int qt_UnicodeToBig5(unsigned int unicode);

int QFontBig5Codec::heuristicContentMatch(const char *, int) const
{
    return 0;
}

QFontBig5Codec::QFontBig5Codec()
{
}

const char* QFontBig5Codec::name() const
{
    return "big5-0";
}

int QFontBig5Codec::mibEnum() const
{
    return -1;
}

QString QFontBig5Codec::toUnicode(const char* /*chars*/, int /*len*/) const
{
    return QString(); //###
}

QCString QFontBig5Codec::fromUnicode(const QString& uc, int& lenInOut ) const
{
    QCString result;
    for ( int i = 0; i < lenInOut; i++ ) {
	QChar ch = uc[i];
	if ( ch.row() == 0) {
	    if ( ch.cell() == ' ' )
		ch = QChar( 0x3000 );
	    else if ( ch.cell() > ' ' && ch.cell() < 127 )
		ch = QChar( ch.cell()-' ', 255 );
	}
	ch = QChar( qt_UnicodeToBig5(ch.unicode()) );

	if ( ch.row() > 0xa0 && ch.cell() >= 0x40  ) {
	    result += ch.row();
	    result += ch.cell();
	} else {
	    //black square
	    result += 0xa1;
	    result += 0xbd;
	}
    }
    lenInOut *=2;
    return result;
}

// -----------------------------------------------------

/* 
   Arabic shaping obeys a number of rules according to the joining classes (see Unicode book, section on
   arabic).
   
   Each unicode char has a joining class (right, dual (left&right), center (joincausing) or transparent).
   transparent joining is not encoded in QChar::joining(), but applies to all combining marks and format marks.
   
   Right join-causing: dual + center
   Left join-causing: dual + right + center
   
   Rules are as follows (for a string already in visual order, as we have it here):
   
   R1 Transparent characters do not affect joining behaviour.
   R2 A right joining character, that has a right join-causing char on the right will get form XRight
   (R3 A left joining character, that has a left join-causing char on the left will get form XLeft)
   Note: the above rule is meaningless, as there are no pure left joining characters defined in Unicode
   R4 A dual joining character, that has a left join-causing char on the left and a right join-causing char on
             the right will get form XMedial
   R5  A dual joining character, that has a right join causing char on the right, and no left join causing char on the left
         will get form XRight
   R6 A dual joining character, that has a  left join causing char on the left, and no right join causing char on the right
         will get form XLeft
   R7 Otherwise the character will get form XIsolated
   
   Additionally we have to do the minimal ligature support for lam-alef ligatures:
   
   L1 Transparent characters do not affect ligature behaviour.
   L2 Any sequence of Alef(XRight) + Lam(XMedial) will form the ligature Alef.Lam(XLeft)
   L3 Any sequence of Alef(XRight) + Lam(XLeft) will form the ligature Alef.Lam(XIsolated)

   The two functions defined in this class do shaping in visual and logical order. For logical order just replace right with
   previous and left with next in the above rules ;-)
*/

/*
  Two small helper functions for arabic shaping. They get the next shape causing character on either
  side of the char in question. Implements rule R1.

  leftChar() returns true if the char to the left is a left join-causing char
  rightChar() returns true if the char to the right is a right join-causing char
*/
static inline bool leftChar( const QString &str, int pos)
{
    //qDebug("leftChar: pos=%d", pos);
    pos--;
    while( pos > -1 ) {
	const QChar &ch = str[pos];
	//qDebug("leftChar: %d isLetter=%d, joining=%d", pos, ch.isLetter(), ch.joining());
	if( ch.isLetter() )
	    return (	 ch.joining() != QChar::OtherJoining );
	// assume it's a transparent char, this might not be 100% correct
	pos--;
    }
    return FALSE;
}

static inline bool rightChar( const QString &str, int pos)
{
    pos++;
    int len = str.length();
    while( pos < len ) {
	const QChar &ch = str[pos];
	//qDebug("rightChar: %d isLetter=%d, joining=%d", pos, ch.isLetter(), ch.joining());
	if( ch.isLetter() ) {
	    QChar::Joining join = ch.joining();
	    return ( join == QChar::Dual || join == QChar::Center );
	}
	// assume it's a transparent char, this might not be 100% correct
	pos++;
    }
    return FALSE;
}


QArabicShaping::Shape QArabicShaping::glyphVariant( const QString &str, int pos)
{
    // ### ignores L1 - L3
    QChar::Joining joining = str[pos].joining();
    //qDebug("checking %x, joining=%d", str[pos].unicode(), joining);
    switch ( joining ) {
	case QChar::OtherJoining:
	case QChar::Center:
	    // these don't change shape
	    return XIsolated;
	case QChar::Right:
	    // only rule R2 applies
	    if( rightChar( str, pos ) )
		return XRight;
	    return XIsolated;
	case QChar::Dual:
	    bool right = rightChar( str, pos );
	    bool left = leftChar( str, pos );
	    //qDebug("dual: right=%d, left=%d", right, left);
	    if( right && left )
		return XMedial;
	    else if ( right )
		return XRight;
	    else if ( left )
		return XLeft;
	    else
		return XIsolated;
    }
    return XIsolated;
}

/* and the same thing for logical ordering :)
 */
static inline bool leftCharL( const QString &str, int pos)
{
    //qDebug("leftChar: pos=%d", pos);
    pos++;
    int len = str.length();
    while( pos < len ) {
	const QChar &ch = str[pos];
	//qDebug("leftChar: %d isLetter=%d, joining=%d", pos, ch.isLetter(), ch.joining());
	if( ch.isLetter() )
	    return (	 ch.joining() != QChar::OtherJoining );
	// assume it's a transparent char, this might not be 100% correct
	pos++;
    }
    return FALSE;
}

static inline bool rightCharL( const QString &str, int pos)
{
    pos--;
    while( pos > -1 ) {
	const QChar &ch = str[pos];
	//qDebug("rightChar: %d isLetter=%d, joining=%d", pos, ch.isLetter(), ch.joining());
	if( ch.isLetter() ) {
	    QChar::Joining join = ch.joining();
	    return ( join == QChar::Dual || join == QChar::Center );
	}
	// assume it's a transparent char, this might not be 100% correct
	pos--;
    }
    return FALSE;
}


QArabicShaping::Shape QArabicShaping::glyphVariantLogical( const QString &str, int pos)
{
    // ### ignores L1 - L3
    QChar::Joining joining = str[pos].joining();
    //qDebug("checking %x, joining=%d", str[pos].unicode(), joining);
    switch ( joining ) {
	case QChar::OtherJoining:
	case QChar::Center:
	    // these don't change shape
	    return XIsolated;
	case QChar::Right:
	    // only rule R2 applies
	    if( rightCharL( str, pos ) )
		return XRight;
	    return XIsolated;
	case QChar::Dual:
	    bool right = rightCharL( str, pos );
	    bool left = leftCharL( str, pos );
	    //qDebug("dual: right=%d, left=%d", right, left);
	    if( right && left )
		return XMedial;
	    else if ( right )
		return XRight;
	    else if ( left )
		return XLeft;
	    else
		return XIsolated;
    }
    return XIsolated;
}


// ---------------------------------------------------------------

// this table covers basic arabic letter, not the extensions used in various other languages.
static const uchar arabic68Mapping[112][4] = {
    // Isolated, left, right, medial forms or:
    // Isolated, Initial, Final, Medial 
    { 0xff, 0xff, 0xff, 0xff }, // 0x600 
    { 0xff, 0xff, 0xff, 0xff }, // 0x601 
    { 0xff, 0xff, 0xff, 0xff }, // 0x602 
    { 0xff, 0xff, 0xff, 0xff }, // 0x603 
    { 0xff, 0xff, 0xff, 0xff }, // 0x604 
    { 0xff, 0xff, 0xff, 0xff }, // 0x605 
    { 0xff, 0xff, 0xff, 0xff }, // 0x606 
    { 0xff, 0xff, 0xff, 0xff }, // 0x607 
    { 0xff, 0xff, 0xff, 0xff }, // 0x608 
    { 0xff, 0xff, 0xff, 0xff }, // 0x609 
    { 0xff, 0xff, 0xff, 0xff }, // 0x60a 
    { 0xff, 0xff, 0xff, 0xff }, // 0x60b 
    { 0xac, 0xff, 0xff, 0xff }, // 0x60c 		Arabic comma
    { 0xff, 0xff, 0xff, 0xff }, // 0x60d 
    { 0xff, 0xff, 0xff, 0xff }, // 0x60e 
    { 0xff, 0xff, 0xff, 0xff }, // 0x60f 

    { 0xff, 0xff, 0xff, 0xff }, // 0x610 
    { 0xff, 0xff, 0xff, 0xff }, // 0x611 
    { 0xff, 0xff, 0xff, 0xff }, // 0x612 
    { 0xff, 0xff, 0xff, 0xff }, // 0x613 
    { 0xff, 0xff, 0xff, 0xff }, // 0x614 
    { 0xff, 0xff, 0xff, 0xff }, // 0x615 
    { 0xff, 0xff, 0xff, 0xff }, // 0x616 
    { 0xff, 0xff, 0xff, 0xff }, // 0x617 
    { 0xff, 0xff, 0xff, 0xff }, // 0x618 
    { 0xff, 0xff, 0xff, 0xff }, // 0x619 
    { 0xff, 0xff, 0xff, 0xff }, // 0x61a 
    { 0xbb, 0xff, 0xff, 0xff }, // 0x61b  		Arabic semicolon
    { 0xff, 0xff, 0xff, 0xff }, // 0x61c 
    { 0xff, 0xff, 0xff, 0xff }, // 0x61d 
    { 0xff, 0xff, 0xff, 0xff }, // 0x61e 
    { 0xbf, 0xff, 0xff, 0xff }, // 0x61f  		Arabic question mark

    { 0xff, 0xff, 0xff, 0xff }, // 0x620 
    { 0xc1, 0xff, 0xff, 0xff }, // 0x621 	 	Hamza
    { 0xc2, 0xff, 0xc2, 0xff }, // 0x622 	R 	Alef with Madda above
    { 0xc3, 0xff, 0xc3, 0xff }, // 0x623 	R	Alef with Hamza above
    { 0xc4, 0xff, 0xc4, 0xff }, // 0x624 	R	Waw with Hamza above
    { 0xc5, 0xff, 0xc5, 0xff }, // 0x625 	R	Alef with Hamza below
    { 0xc6, 0xc0, 0xc6, 0xc0 }, // 0x626 	D	Yeh with Hamza above
    { 0xc7, 0xff, 0xc7, 0xff }, // 0x627 	R	Alef
    { 0xc8, 0xeb, 0xc8, 0xeb }, // 0x628 	D	Beh
    { 0xc9, 0xff, 0x8e, 0xff }, // 0x629 	R	Teh Marbuta
    { 0xca, 0xec, 0xc1, 0xec }, // 0x62a 	D	Teh
    { 0xcb, 0xed, 0xcb, 0xed }, // 0x62b 	D 	Theh
    { 0xcc, 0xee, 0xcc, 0xee }, // 0x62c 	D 	Jeem
    { 0xcd, 0xef, 0xcd, 0xef }, // 0x62d 	D 	Hah
    { 0xce, 0xf0, 0xce, 0xf0 }, // 0x62e 	D 	Khah
    { 0xcf, 0xff, 0xcf, 0xff }, // 0x62f 	R 	Dal

    { 0xd0, 0xff, 0xd0, 0xff }, // 0x630 	R 	Thal
    { 0xd1, 0xff, 0xd1, 0xff }, // 0x631 	R 	Reh
    { 0xd2, 0xff, 0xd2, 0xff }, // 0x632 	R 	Zain
    { 0xd3, 0xf1, 0x8f, 0xf1 }, // 0x633 	D 	Seen
    { 0xd4, 0xf2, 0x90, 0xf2 }, // 0x634 	D 	Sheen
    { 0xd5, 0xf3, 0x91, 0xf3 }, // 0x635 	D 	Sad
    { 0xd6, 0xf4, 0x92, 0xf4 }, // 0x636 	D 	Dad
    { 0xd7, 0xd7, 0x93, 0x93 }, // 0x637 	D 	Tah
    { 0xd8, 0xd8, 0x94, 0x94 }, // 0x638 	D 	Zah
    { 0xd9, 0xf5, 0x96, 0x95 }, // 0x639 	D 	Ain
    { 0xda, 0xf6, 0x98, 0x97 }, // 0x63a 	D	Ghain
    { 0xff, 0xff, 0xff, 0xff }, // 0x63b 
    { 0xff, 0xff, 0xff, 0xff }, // 0x63c 
    { 0xff, 0xff, 0xff, 0xff }, // 0x63d 
    { 0xff, 0xff, 0xff, 0xff }, // 0x63e 
    { 0xff, 0xff, 0xff, 0xff }, // 0x63f 

    { 0xe0, 0xff, 0xff, 0xff }, // 0x640 		Tatweel
    { 0xe1, 0xf7, 0xe1, 0x99 }, // 0x641 	D 	Feh
    { 0xe2, 0xf8, 0xe2, 0x9a }, // 0x642 	D 	Qaf
    { 0xe3, 0xf9, 0xe3, 0x9b }, // 0x643 	D 	Kaf
    { 0xe4, 0xfa, 0xe4, 0xfa }, // 0x644 	D 	Lam
    { 0xe5, 0xfb, 0xe5, 0xfb }, // 0x645 	D 	Meem
    { 0xe6, 0xfc, 0xe6, 0xfc }, // 0x646 	D 	Noon
    { 0xe7, 0xfd, 0xe7, 0xfd }, // 0x647 	D 	Heh
    { 0xe8, 0xff, 0xe8, 0xff }, // 0x648 	R 	Waw
    { 0x8d, 0x9f, 0x9f, 0x9f }, // 0x649 	D 	Alef Maksura 	### this looks strange, the font only has isolated and final forms, still dual joining?
    { 0xea, 0xfe, 0xea, 0xfe }, // 0x64a 	D 	Yeh
    { 0xeb, 0xff, 0xff, 0xff }, // 0x64b 	Mark Fathatan
    { 0xec, 0xff, 0xff, 0xff }, // 0x64c 	Mark Dammatan
    { 0xed, 0xff, 0xff, 0xff }, // 0x64d 	Mark Kasratan
    { 0xee, 0xff, 0xff, 0xff }, // 0x64e 	Mark Fatha
    { 0xef, 0xff, 0xff, 0xff }, // 0x64f 	Mark Damma

    { 0xf0, 0xff, 0xff, 0xff }, // 0x650 	Mark Kasra
    { 0xf1, 0xff, 0xff, 0xff }, // 0x651 	Mark Shadda
    { 0xf2, 0xff, 0xff, 0xff }, // 0x652 	Mark Sukan
    // these do not exist in latin6 anymore:
    { 0xff, 0xff, 0xff, 0xff }, // 0x653 	Mark Maddah above
    { 0xff, 0xff, 0xff, 0xff }, // 0x654 	Mark Hamza above
    { 0xff, 0xff, 0xff, 0xff }, // 0x655 	Mark Hamza below
    { 0xff, 0xff, 0xff, 0xff }, // 0x656 
    { 0xff, 0xff, 0xff, 0xff }, // 0x657 
    { 0xff, 0xff, 0xff, 0xff }, // 0x658 
    { 0xff, 0xff, 0xff, 0xff }, // 0x659 
    { 0xff, 0xff, 0xff, 0xff }, // 0x65a 
    { 0xff, 0xff, 0xff, 0xff }, // 0x65b 
    { 0xff, 0xff, 0xff, 0xff }, // 0x65c 
    { 0xff, 0xff, 0xff, 0xff }, // 0x65d 
    { 0xff, 0xff, 0xff, 0xff }, // 0x65e 
    { 0xff, 0xff, 0xff, 0xff }, // 0x65f 

    { 0xb0, 0xff, 0xff, 0xff }, // 0x660 	Arabic 0
    { 0xb1, 0xff, 0xff, 0xff }, // 0x661 	Arabic 1
    { 0xb2, 0xff, 0xff, 0xff }, // 0x662 	Arabic 2
    { 0xb3, 0xff, 0xff, 0xff }, // 0x663 	Arabic 3
    { 0xb4, 0xff, 0xff, 0xff }, // 0x664 	Arabic 4
    { 0xb5, 0xff, 0xff, 0xff }, // 0x665 	Arabic 5
    { 0xb6, 0xff, 0xff, 0xff }, // 0x666 	Arabic 6
    { 0xb7, 0xff, 0xff, 0xff }, // 0x667 	Arabic 7
    { 0xb8, 0xff, 0xff, 0xff }, // 0x668 	Arabic 8
    { 0xb9, 0xff, 0xff, 0xff }, // 0x669 	Arabic 9
    { 0x25, 0xff, 0xff, 0xff }, // 0x66a 	Arabic % sign
    { 0x2c, 0xff, 0xff, 0xff }, // 0x66b 	Arabic decimal separator
    { 0x2c, 0xff, 0xff, 0xff }, // 0x66c 	Arabic thousands separator
    { 0x2a, 0xff, 0xff, 0xff }, // 0x66d 	Arabic five pointed star
    { 0xff, 0xff, 0xff, 0xff }, // 0x66e 
    { 0xff, 0xff, 0xff, 0xff }, // 0x66f 
};

int QFontArabic68Codec::heuristicContentMatch(const char *, int) const
{
    return 0;
}

QFontArabic68Codec::QFontArabic68Codec()
{
}

const char* QFontArabic68Codec::name() const
{
    return "iso8859-6.8x";
}

int QFontArabic68Codec::mibEnum() const
{
    return -1;
}

QString QFontArabic68Codec::toUnicode(const char* /*chars*/, int /*len*/) const
{
    return QString(); //###
}

QCString QFontArabic68Codec::fromUnicode(const QString& uc, int& lenInOut ) const
{
    QCString result;
    const QChar *ch = uc.unicode();
    for ( int i = 0; i < lenInOut; i++ ) {
	if ( ch->row() == 0 && ch->cell() < 0x80 ) {
	    result += ch->cell();
	}
	if ( ch->row() != 0x06 || ch->cell() > 0x6f )
	    result += 0xff; // undefined char in iso8859-6.8x
	else {
	    int shape = QArabicShaping::glyphVariant( uc, i );
	    //qDebug("mapping U+%x to shape %d glyph=0x%x", ch->unicode(), shape, arabic68Mapping[ch->cell()][shape]);
	    result += arabic68Mapping[ch->cell()][shape];
	}
	ch++;
    }
    return result;
}

ushort QFontArabic68Codec::shapedGlyph( const QString &str, int pos )
{
    const QChar *ch = str.unicode() + pos;
    if ( ch->row() == 0 && ch->cell() < 0x80 ) {
	return ch->cell();
    }
    if ( ch->row() != 0x06 || ch->cell() > 0x6f )
	return 0xff; // undefined char in iso8859-6.8x
    else {
	int shape = QArabicShaping::glyphVariantLogical( str, pos );
	//qDebug("mapping U+%x to shape %d glyph=0x%x", ch->unicode(), shape, arabic68Mapping[ch->cell()][shape]);
	return arabic68Mapping[ch->cell()][shape];
    }
    
}

#if 0
static uchar arabic68Mapping[96][4] {
    // Isolated, left, right, medial forms
    { 0xff, 0xff, 0xff, 0xff }, // 0x600 
    { 0xff, 0xff, 0xff, 0xff }, // 0x601 
    { 0xff, 0xff, 0xff, 0xff }, // 0x602 
    { 0xff, 0xff, 0xff, 0xff }, // 0x603 
    { 0xff, 0xff, 0xff, 0xff }, // 0x604 
    { 0xff, 0xff, 0xff, 0xff }, // 0x605 
    { 0xff, 0xff, 0xff, 0xff }, // 0x606 
    { 0xff, 0xff, 0xff, 0xff }, // 0x607 
    { 0xff, 0xff, 0xff, 0xff }, // 0x608 
    { 0xff, 0xff, 0xff, 0xff }, // 0x609 
    { 0xff, 0xff, 0xff, 0xff }, // 0x60a 
    { 0xff, 0xff, 0xff, 0xff }, // 0x60b 
    { 0xac, 0xff, 0xff, 0xff }, // 0x60c 	Arabic comma
    { 0xff, 0xff, 0xff, 0xff }, // 0x60d 
    { 0xff, 0xff, 0xff, 0xff }, // 0x60e 
    { 0xff, 0xff, 0xff, 0xff }, // 0x60f 

    { 0xff, 0xff, 0xff, 0xff }, // 0x610 
    { 0xff, 0xff, 0xff, 0xff }, // 0x611 
    { 0xff, 0xff, 0xff, 0xff }, // 0x612 
    { 0xff, 0xff, 0xff, 0xff }, // 0x613 
    { 0xff, 0xff, 0xff, 0xff }, // 0x614 
    { 0xff, 0xff, 0xff, 0xff }, // 0x615 
    { 0xff, 0xff, 0xff, 0xff }, // 0x616 
    { 0xff, 0xff, 0xff, 0xff }, // 0x617 
    { 0xff, 0xff, 0xff, 0xff }, // 0x618 
    { 0xff, 0xff, 0xff, 0xff }, // 0x619 
    { 0xff, 0xff, 0xff, 0xff }, // 0x61a 
    { 0xbb, 0xff, 0xff, 0xff }, // 0x61b  	Arabic semicolon
    { 0xff, 0xff, 0xff, 0xff }, // 0x61c 
    { 0xff, 0xff, 0xff, 0xff }, // 0x61d 
    { 0xff, 0xff, 0xff, 0xff }, // 0x61e 
    { 0xbf, 0xff, 0xff, 0xff }, // 0x61f  	Arabic question mark

    { 0xff, 0xff, 0xff, 0xff }, // 0x620 
    { 0xc1, 0xff, 0xff, 0xff }, // 0x621 	Hamza
    { 0xc2, 0xff, 0xff, 0xff }, // 0x622 	Alef with Madda above
    { 0xc3, 0xff, 0xff, 0xff }, // 0x623 	Alef with Hamza above
    { 0xc4, 0xff, 0xff, 0xff }, // 0x624 	Waw with Hamza above
    { 0xc5, 0xff, 0xff, 0xff }, // 0x625 	Alef with Hamza below
    { 0xc6, 0xff, 0xff, 0xff }, // 0x626 	Yeh with Hamza above
    { 0xc7, 0xff, 0xff, 0xff }, // 0x627 	Alef
    { 0xc8, 0xff, 0xff, 0xff }, // 0x628 	Beh
    { 0xc9, 0xff, 0xff, 0xff }, // 0x629 	Teh Marbuta
    { 0xca, 0xff, 0xff, 0xff }, // 0x62a 	Teh
    { 0xcb, 0xff, 0xff, 0xff }, // 0x62b 	Theh
    { 0xcc, 0xff, 0xff, 0xff }, // 0x62c 	Jeem
    { 0xcd, 0xff, 0xff, 0xff }, // 0x62d 	Hah
    { 0xce, 0xff, 0xff, 0xff }, // 0x62e 	Khah
    { 0xcf, 0xff, 0xff, 0xff }, // 0x62f 	Dal

    { 0xd0, 0xff, 0xff, 0xff }, // 0x630 	Thal
    { 0xd1, 0xff, 0xff, 0xff }, // 0x631 	Reh
    { 0xd2, 0xff, 0xff, 0xff }, // 0x632 	Zain
    { 0xd3, 0xff, 0xff, 0xff }, // 0x633 	Seen
    { 0xd4, 0xff, 0xff, 0xff }, // 0x634 	Sheen
    { 0xd5, 0xff, 0xff, 0xff }, // 0x635 	Sad
    { 0xd6, 0xff, 0xff, 0xff }, // 0x636 	Dad
    { 0xd7, 0xff, 0xff, 0xff }, // 0x637 	Tah
    { 0xd8, 0xff, 0xff, 0xff }, // 0x638 	Zah
    { 0xd9, 0xff, 0xff, 0xff }, // 0x639 	Ain
    { 0xda, 0xff, 0xff, 0xff }, // 0x63a 	Ghain
    { 0xff, 0xff, 0xff, 0xff }, // 0x63b 
    { 0xff, 0xff, 0xff, 0xff }, // 0x63c 
    { 0xff, 0xff, 0xff, 0xff }, // 0x63d 
    { 0xff, 0xff, 0xff, 0xff }, // 0x63e 
    { 0xff, 0xff, 0xff, 0xff }, // 0x63f 

    { 0xe0, 0xff, 0xff, 0xff }, // 0x640 	Tatweel
    { 0xe1, 0xff, 0xff, 0xff }, // 0x641 	Feh
    { 0xe2, 0xff, 0xff, 0xff }, // 0x642 	Qaf
    { 0xe3, 0xff, 0xff, 0xff }, // 0x643 	Kaf
    { 0xe4, 0xff, 0xff, 0xff }, // 0x644 	Lam
    { 0xe5, 0xff, 0xff, 0xff }, // 0x645 	Meem
    { 0xe6, 0xff, 0xff, 0xff }, // 0x646 	Noon
    { 0xe7, 0xff, 0xff, 0xff }, // 0x647 	Heh
    { 0xe8, 0xff, 0xff, 0xff }, // 0x648 	Waw
    { 0xe9, 0xff, 0xff, 0xff }, // 0x649 	Alef Maksura
    { 0xea, 0xff, 0xff, 0xff }, // 0x64a 	Yeh
    { 0xeb, 0xff, 0xff, 0xff }, // 0x64b 	Mark Fathatan
    { 0xec, 0xff, 0xff, 0xff }, // 0x64c 	Mark Dammatan
    { 0xed, 0xff, 0xff, 0xff }, // 0x64d 	Mark Kasratan
    { 0xee, 0xff, 0xff, 0xff }, // 0x64e 	Mark Fatha
    { 0xef, 0xff, 0xff, 0xff }, // 0x64f 	Mark Damma

    { 0xf0, 0xff, 0xff, 0xff }, // 0x650 	Mark Kasra
    { 0xf1, 0xff, 0xff, 0xff }, // 0x651 	Mark Shadda
    { 0xf2, 0xff, 0xff, 0xff }, // 0x652 	Mark Sukan
    // these do not exist in latin6 anymore:
    { 0xff, 0xff, 0xff, 0xff }, // 0x653 	Mark Maddah above
    { 0xff, 0xff, 0xff, 0xff }, // 0x654 	Mark Hamza above
    { 0xff, 0xff, 0xff, 0xff }, // 0x655 	Mark Hamza below
    { 0xff, 0xff, 0xff, 0xff }, // 0x656 
    { 0xff, 0xff, 0xff, 0xff }, // 0x657 
    { 0xff, 0xff, 0xff, 0xff }, // 0x658 
    { 0xff, 0xff, 0xff, 0xff }, // 0x659 
    { 0xff, 0xff, 0xff, 0xff }, // 0x65a 
    { 0xff, 0xff, 0xff, 0xff }, // 0x65b 
    { 0xff, 0xff, 0xff, 0xff }, // 0x65c 
    { 0xff, 0xff, 0xff, 0xff }, // 0x65d 
    { 0xff, 0xff, 0xff, 0xff }, // 0x65e 
    { 0xff, 0xff, 0xff, 0xff }, // 0x65f 

    { 0xff, 0xff, 0xff, 0xff }, // 0x660 	Arabic 0
    { 0xff, 0xff, 0xff, 0xff }, // 0x661 	Arabic 1
    { 0xff, 0xff, 0xff, 0xff }, // 0x662 	Arabic 2
    { 0xff, 0xff, 0xff, 0xff }, // 0x663 	Arabic 3
    { 0xff, 0xff, 0xff, 0xff }, // 0x664 	Arabic 4
    { 0xff, 0xff, 0xff, 0xff }, // 0x665 	Arabic 5
    { 0xff, 0xff, 0xff, 0xff }, // 0x666 	Arabic 6
    { 0xff, 0xff, 0xff, 0xff }, // 0x667 	Arabic 7
    { 0xff, 0xff, 0xff, 0xff }, // 0x668 	Arabic 8
    { 0xff, 0xff, 0xff, 0xff }, // 0x669 	Arabic 9
    { 0xff, 0xff, 0xff, 0xff }, // 0x66a 	Arabic % sign
    { 0xff, 0xff, 0xff, 0xff }, // 0x66b 	Arabic decimal separator
    { 0xff, 0xff, 0xff, 0xff }, // 0x66c 	Arabic thousands separator
    { 0xff, 0xff, 0xff, 0xff }, // 0x66d 	Arabic five pointed star
    { 0xff, 0xff, 0xff, 0xff }, // 0x66e 
    { 0xff, 0xff, 0xff, 0xff }, // 0x66f 
};
#endif

#endif //QT_NO_CODECS
