/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qfont.cpp#148 $
**
** Implementation of QFont, QFontMetrics and QFontInfo classes
**
** Created : 941207
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
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
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
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

#include "qfont.h"
#include "qfontdata_p.h"
#include "qfontmetrics.h"
#include "qfontinfo.h"
#include "qpainter.h"
#include "qpainter_p.h"

#include <qdict.h>
#include <qcache.h>
#include <qdatastream.h>
#include <qapplication.h>
#include <qcleanuphandler.h>
#include <qstringlist.h>
#include <ctype.h>
#include <limits.h>


// #define QFONTCACHE_DEBUG


// REVISED: brad
/*! \class QFont qfont.h

  \brief The QFont class specifies a font used for drawing text.

  \ingroup drawing
  \ingroup appearance
  \ingroup shared

  More precisely, QFont is a collection of attributes of a font.
  When Qt needs to draw text, it will look up the closest
  matching installed font, load it and use it.
  If a choosen X11 font does not cover all characters to be displayed,
  QFont blends in the missing characters from other fonts if possible.

  The most important attributes of a QFont are its font family (family()),
  its size (pointSize()), its boldness (weight()) and
  whether it is italic() or not. There are
  QFont constructors that take these attributes as arguments, for example:

  \walkthrough fonts/simple-qfont-demo/viewer.cpp
  \skipto defaultButton
  \printline defaultButton
  \printuntil times

  12pt (the default if nothing else is specified) Times, normal weight,
  roman (i.e. non-italic) is used for the label of the defaultButton push button.

  \walkthrough fonts/simple-qfont-demo/viewer.cpp
  \skipto sansSerifButton
  \printline sansSerifButton
  \printuntil Helvetica

  This button's label is drawn using Helvetica 12pt, non-bold, non-italic.

  \walkthrough fonts/simple-qfont-demo/viewer.cpp
  \skipto italicsButton
  \printline italicsButton
  \printuntil lucida

  To draw this button's label 12pt Lucida, bold italic is used.

  (Code taken from \link simple-font-demo-example.html
  fonts/simple-qfont-demo/viewer.cpp \endlink)

  The default QFont constructor without any arguments
  keeps a copy of the application's default font, QApplication::font().

  You can change the attributes of an existing QFont object
  using functions such as setFamily(), setPointSize(), setWeight() and
  setItalic().

  There are also some less-used attributes. setUnderline() decides
  whether the font is underlined or not; setStrikeOut() can be used to get
  overstrike (a horizontal line through the middle of the characters);
  setFixedPitch() determines whether Qt should give preference to
  fixed-pitch (also known as monospace, fixed-width or typewriter fonts)
  or variable-pitch (proportional) fonts when it needs to choose an installed font.

  setStyleHint() can be used to offer
  more general help to the font matching algorithm, and on X11
  setRawName() can be used to bypass the entire font matching and use an
  X11 XLFD (X Logical Font Description).

  Of course there is also a reader function for each of the set*()
  functions. Note that the reader functions return the values
  previously set, \e not the attributes of the actual window system
  font that will be used for drawing. Information about
  the font that will be used for drawing can be obtained by using QFontInfo, but be
  aware that QFontInfo may be slow and that its results depend on
  which fonts are installed.

  In general font handling and loading are costly operations,
  especially on X11.  The QFont class contains extensive optimizations
  to make copying of QFont objects fast, and to cache the results of
  the slow window system functions it uses.

  QFont also offers a few static functions, mostly to tune the font
  matching algorithm: You can control what happens if a font family
  isn't installed using insertSubstitution(), insertSubstitutions()
  and removeSubstitution().

  This is especially important when different character sets are
  used at the same time. As a Times font for example does not include
  Arabic characters, an Arabic substitution font for Times can
  be specified and will be used when Arabic characters show up.

  Each QFont can have an entire substitution list so that Unicode
  characters can be displayed even if no Unicode fonts are available.
  How good this approximation works depends on the variety
  of fonts installed.

  You can get a complete list of the fallback families using substitutions().

  Finally, QApplication::setFont() allows you to set the default font.
  the default default font is chosen at application startup from a set
  of common installed fonts that support the correct character set for
  the current locale. Of course, the initialization algorithm has a
  default, too: The default default default font!

  The font matching algorithm works as follows:
  First an available font family is found. If the requested is not
  available the styleHint() is used to select a replacement family. If
  the style hint has not been set, "helvetica" will be used.

  If even the replacement family is not found, "helvetica" is searched
  for, if that too is not found Qt will search for a last resort font,
  i.e.  a specific font to match to, ignoring the attribute
  settings. Qt searches through a built-in list of very common
  fonts. If none of these are available, Qt gives you an error message
  and aborts (of course this only happens if you are using fonts and
  Qt \e has to load a font). We have not been able to find a case
  where this happens. Please <a href="bughowto.html">report it as a
  bug</a> if it does, preferably with a list of the fonts you have
  installed.

  The following attributes are then matched, in order of priority: <ol>
  <li> fixedPitch()
  <li> pointSize() (see below)
  <li> weight()
  <li> italic()
  </ol>

  If, for example, a font with the correct character set is found, but
  with all other attributes in the list unmatched, it will be chosen
  before a font with the wrong character set but with all other
  attributes correct.

  The point size is defined to match if it is within 20% of the
  requested point size. Of course, when several fonts match and only
  point size differs the closest point size to the one requested will
  be chosen.

  You can also specify a font size in pixels (with the setPixelSize() method), but
  usually this is not recommended, as the pixel size is device dependent. A
  font with a point size of 72 points will have a size of approximately one inch on all
  devices (screen or printer), whereas a font with a size of 12 pixels will look the
  same as the 12 point font on a 75dpi display, but will have a very small size on a 600dpi
  printer.

  For more general information on fonts, see the
  \link http://www.nwalsh.com/comp.fonts/FAQ/ comp.fonts FAQ.\endlink
  Information on encodings can be found on
  \link http://czyborra.com/charsets/ Roman Czyborra's respective page.\endlink

  \sa QFontMetrics QFontInfo QFontDatabase QApplication::setFont()
  QWidget::setFont() QPainter::setFont() QFont::StyleHint
  QFont::Weight
*/

/*! \enum QFont::Script

  The QFont::Script enum represents \link unicode.html Unicode \endlink
  allocated scripts. For exhaustive
  coverage see \link http://www.amazon.com/exec/obidos/ASIN/0201616335/trolltech/t
  The Unicode Standard Version 3.0 \endlink.
  The following scripts are supported:

  Modern European alphabetic scripts (left to right):

  \value Latin consists of most alphabets based on the original Latin alphabet.
  \value Greek covers ancient and modern Greek and Coptic.
  \value Cyrillic covers the Slavic and non-Slavic languages using cyrillic alphabets.
  \value Armenian contains the Armenian alphabet used with the Armenian language.
  \value Georgian covers at least the language Georgian.
  \value Runic covers the known constituents of the Runic alphabets used
	 by the early and medieval societies in the Germanic, Scandinavian, and Anglo-Saxon
	 areas.
  \value Ogham is an alphabetical script used to write a very early form of Irish.
  \value SpacingModifiers are small signs indicating modifications of a preceeding letter.
  \value CombiningMarks consists of diacritical marks not specific to a particular alphabet,
	 diacritical marks used in combination with mathematical and technical symbols,
	 and glyph encodings applied to multiple letterforms.

  Middle Eastern scripts (right to left):

  \value Hebrew is used for writing Hebrew, Yiddish, and some other languages.
  \value Arabic covers the Arabic language as well as Persian, Urdu, Kurdish and some
	 others.
  \value Syriac is used to write the active liturgical languages and dialects of several
	 Middle Eastern and Southeast Indian communities.
  \value Thaana is used to write the Maledivian Dhivehi language.

  South and Southeast Asian scripts (left to right with few historical exceptions):

  \value Devanagari covers classical Sanskrit and modern Hindi as well as several other
	 languages.
  \value Bengali is a relative to Devanagari employed to write the Bengali language
	 used in West Bengal/India and Bangladesh as well as several minority languages.
  \value Gurmukhi is another Devanagari relative used to write Punjabi.
  \value Gujarati is closely related to Devanagari and used to write the Gujarati
	 language of the Gujarat state in India.
  \value Oriya is used to write the Oriya language of Orissa state/India.
  \value Tamil is used to write the Tamil language of Tamil Nadu state/India,
	 Sri Lanka, Singapore and parts of Malaysia as well as some minority languages.
  \value Telugu is used to write the Telugu language of Andhra Pradesh state/India
	 and some minority languages.
  \value Kannada is another South Indian script used to write the Kannada language of
	 Karnataka state/India and some minority languages.
  \value Malayalam is used to write the Malayalam language of Kerala state/India.
  \value Sinhala is used for Sri Lanka's majority language Sinhala and is also employed
	 to write Pali, Sanskrit, and Tamil.
  \value Thai is used to write Thai and other Southeast Asian languages.
  \value Lao is a language and script quite similar to Thai.
  \value Tibetan is the script used to write Tibetan in several countries like Tibet,
	 the bordering Indian regions, or Nepal. It is also used in the Buddist
	 philosophy and liturgy of the Mongolian cultural area.
  \value Myanmar is mainly used to write the Burmese language of Myanmar (former Burma).
  \value Khmer is the official language of Kampuchea.

  East Asian scripts (traditionally top-down, right to left, modern often horizontal
	 left to right):

  \value Han consists of the CJK (Chinese, Japanese, Korean) idiographic characters.
  \value Hiragana is a cursive syllabary used to indicate phonetics and pronounciation
	 of Japanese words.
  \value Katakana is a non-cursive syllabic script used to write Japanese words with
	 visual emphasis and non-Japanese words in a phonetical manner.
  \value Hangul is a Korean script consisting of alphabetic components.
  \value Bopomofo is a phonetic alphabet for Chinese
	 (mainly Mandarin).
  \value Yi (also called Cuan or Wei) is a syllabary used to write the Yi language
	 of Southwestern China, Myanmar, Laos, and Vietnam.

  Additional scripts that do not fit well into the script categories above:

  \value Ethiopic is a syllabary used by several Central East African languages.
  \value Cherokee is a left-to-right syllabic script used to write the Cherokee language.
  \value CanadianAboriginal consists of the syllabics used by some Canadian aboriginal societies.
  \value Mongolian is the traditional (and recently reintroduced) script used to write Mongolian.

  Symbols:

  \value CurrencySymbols contains currency symbols not encoded in other scripts.
  \value LetterlikeSymbols consists of symbols derived  from ordinary letters of an
	 alphabetical script.
  \value NumberForms are provided for compatibility with other existing character sets.
  \value MathematicalOperators consists of encodings for operators,
	 relations and other symbols like arrows used in a mathematical context.
  \value TechnicalSymbols contains representations for control codes, the space symbol,
	 APL symbols and other symbols mainly used in the context of electronic data
	 processing.
  \value GeometricSymbols covers block elements and geometric shapes.
  \value MiscellaneousSymbols consists of a heterogeneous collection of symbols that
	 do not fit any other Unicode character block, e.g. Dingbats.
  \value EnclosedAndSquare is provided for compatibility with some East Asian standards.
  \value Braille is an international writing system used by blind people. This script encodes
	 the 256 eight-dot patterns with the 64 six-dot patterns as a subset.


  \value Unicode includes all the above scripts.

  The values below are provided for completeness and must not be used in user programs.

  \value HanX11 For internal use only.
  \value LatinBasic For internal use only.
  \value LatinExtendedA_2 For internal use only.
  \value LatinExtendedA_3 For internal use only.
  \value LatinExtendedA_4 For internal use only.
  \value LatinExtendedA_14 For internal use only.
  \value LatinExtendedA_15 For internal use only.

  \value LastPrivateScript For internal use only.

  \value NScripts For internal use only.
  \value NoScript For internal use only.
  \value UnknownScript For internal use only.
*/

/*! \internal

  Constructs a font that gets by default a
  \link shclass.html deep copy \endlink of \a data.
  If \a deep is false, the copy will be shallow.
*/
QFont::QFont( QFontPrivate *data, bool deep )
{
    if ( deep ) {
	d = new QFontPrivate( *data );
	Q_CHECK_PTR( d );

	// now a single reference
	d->count = 1;
    } else {
	d = data;
	d->ref();
    }
}


/*!
  \internal
  Detaches the font object from common font data.
*/
void QFont::detach()
{
    if (d->count != 1) {
	*this = QFont(d);
    }
}


/*! Constructs a font object that refers to the default font.
  The settings for this font are copied from the default QApplication font.

  \sa QApplication::setFont(), QApplication::font()
*/
QFont::QFont()
{
    const QFont appfont = QApplication::font();
    d = appfont.d;
    d->ref();
}


/*! Constructs a font object with the specified \a family, \a pointSize,
  \a weight and \a italic settings.

  If \a pointSize is less than or equal to 0 it is set to 1.

  \sa Weight, setFamily(), setPointSize(), setWeight(), setItalic(), QApplication::font()
*/
QFont::QFont( const QString &family, int pointSize, int weight, bool italic )
{
    if (pointSize <= 0) pointSize = 1;

    d = new QFontPrivate;
    Q_CHECK_PTR( d );

    d->request.family = family;
    d->request.pointSize = pointSize * 10;
    d->request.pixelSize = -1;
    d->request.weight = weight;
    d->request.italic = italic;
}


/*! Constructs a font that is a copy of \a font.
*/
QFont::QFont( const QFont &font )
{
    d = font.d;
    d->ref();
}

/*! Destroys the font object and frees all allocated resources.
*/
QFont::~QFont()
{
    if ( d->deref() ) {
	delete d;
    }
}


/*! Assigns \a font to this font and returns a reference to it.
*/
QFont &QFont::operator=( const QFont &font )
{
    if (d->deref()) delete d;

    d = font.d;
    d->ref();

    return *this;
}


/*! Returns the family name set by setFamily().

  Use QFontInfo to find the family name of the window system font that
  is actually used for drawing.

  \walkthrough fonts/simple-qfont-demo/viewer.cpp
  \skipto messageText
  \printline messageText
  \printuntil font.family()
  \skipto Font used
  \printline Font used
  \printuntil info.pointSize()

  (Code taken from \link simple-font-demo-example.html
  fonts/simple-qfont-demo/viewer.cpp \endlink)

  \sa substitutes substitute()
*/
QString QFont::family() const
{
    return d->request.family;
}


/*! Defines the \a family name of the font (e.g. "Helvetica" or "times").

  The family name is case insensitive.

  If the family is not available a default family is used.

  \sa family(), setStyleHint(), QFontInfo
*/
void QFont::setFamily( const QString &family )
{
    if (d->request.family == family) return;

    detach();
    d->request.family = family;
    d->request.dirty  = TRUE;
}


/*! Returns the point size in 1/10ths of a point.

  The returned value will be -1 if the font size has been specified in pixels.

  \sa pointSize()
  */
int QFont::deciPointSize() const
{
    return d->request.pointSize;
}


/*! Returns the point size of \e this font.

  Use QFontInfo to find the point size of the window system font
  actually used:

   \walkthrough fonts/simple-qfont-demo/viewer.cpp
   \skipto messageText
   \printuntil messageText =
   \skipto font.pointSize()
   \printuntil "Font used: \""
   \skipto info.pointSize()
   \printline info.pointSize()

  (Code taken from \link simple-font-demo-example.html
  fonts/simple-qfont-demo/viewer.cpp \endlink)

  The returned value will be -1 if the font size has been specified in pixels.

  \sa setPointSize() deciPointSize()
*/
int QFont::pointSize() const
{
    return d->request.pointSize == -1 ? -1 : d->request.pointSize/ 10;
}


/*! Sets the point size to \a pointSize. The point size must be greater
  than zero.

  \walkthrough fonts/simple-qfont-demo/viewer.cpp
  \skipto Tokyo
  \printuntil setPointSize

  (Code taken from \link simple-font-demo-example.html
  fonts/simple-qfont-demo/viewer.cpp \endlink)

  \sa pointSize(), QFontInfo, setPixelSize()
*/
void QFont::setPointSize( int pointSize )
{
    if ( pointSize <= 0 ) {

#if defined(CHECK_RANGE)
	qWarning( "QFont::setPointSize: Point size <= 0 (%d)", pointSize );
#endif

	return;
    }

    pointSize *= 10;
    if (d->request.pointSize == pointSize) return;

    detach();
    d->request.pointSize = (short) pointSize;
    d->request.pixelSize = -1;
    d->request.dirty = TRUE;
}


/*! Sets the point size to \a pointSize. The point size must be greater
  than zero. The requested precision may not be achieved on all platforms.

  \sa pointSizeFloat() setPointSize() setPixelSize()
*/
void QFont::setPointSizeFloat( float pointSize )
{
    if ( pointSize <= 0 ) {
#if defined(CHECK_RANGE)
	qWarning( "QFont::setPointSize: Point size <= 0 (%f)", pointSize );
#endif
	return;
    }

    int ps = int(pointSize * 10.0 + 0.5);
    if (d->request.pointSize == ps) return;

    detach();
    d->request.pointSize = (short) ps;
    d->request.pixelSize = -1;
    d->request.dirty = TRUE;
}


/*! Returns the requested height of characters in the font in points (1/72 inch).
  If the font size was determined in pixels, this function returns -1.

  \sa pointSize() pixelSize() QFontInfo::pointSize() QFontInfo::pixelSize()
*/
float QFont::pointSizeFloat() const
{
    return float(d->request.pointSize == -1 ? -10 : d->request.pointSize) / 10.0;
}


/*! Sets the height of font characters shown on
  the screen or printer to \a pixelSize pixels.

  This way of specifying a font size makes the font device dependent, and the
  resulting font will vary in size on devices with different resolutions. setPointSize()
  is the device independent approach.

  \sa pixelSize()
*/
void QFont::setPixelSize( int pixelSize )
{
    if ( pixelSize <= 0 ) {
#if defined(CHECK_RANGE)
	qWarning( "QFont::setPointSize: Point size <= 0 (%f)", pointSize );
#endif
	return;
    }
    if (d->request.pixelSize == pixelSize) return;

    detach();
    d->request.pixelSize = (int) pixelSize;
    d->request.pointSize = -1;
    d->request.dirty = TRUE;

    setPixelSizeFloat(float(pixelSize));
}

/*! Returns the pixel height of characters in the font if shown on
  the screen. Will return -1 if the font size was set in points.

  Use the QFontInfo class to get the real height of the font.

  \sa setPixelSize() pointSize() QFontInfo::pointSize() QFontInfo::pixelSize()
*/
int QFont::pixelSize() const
{
    return d->request.pixelSize;
}


/*! \obsolete

  Sets the logical pixel height of font characters when shown on
  the screen to \a pixelSize.
*/
void QFont::setPixelSizeFloat( float pixelSize )
{
    setPixelSize( (int)pixelSize );
}


/*! Returns whether an italic font should be used (TRUE) or not (FALSE).

  Use QFontInfo to find out whether the window system font actually
  used is italic.

  \sa setItalic()
*/
bool QFont::italic() const
{
    return d->request.italic;
}


/*! Sets italic on or off, depending on the value of \a enable (TRUE or
  FALSE respectively).

  \sa italic(), QFontInfo
*/
void QFont::setItalic( bool enable )
{

    if ((bool) d->request.italic == enable) return;

    detach();
    d->request.italic = enable;
    d->request.dirty = TRUE;
}


/*! Returns the requested font boldness.

  Use QFontInfo to find the weight of the window system font actually used.

  \sa setWeight(), Weight, QFontInfo
*/
int QFont::weight() const
{
    return d->request.weight;
}


/*! \enum QFont::Weight

  The weight of a font is a measure for its thickness.

  Qt uses a weighting scale from 0 to 99 similar but not
  equal to the scales used in Windows or CSS. A thickness of
  0 leads to an ultralight, 99 to an extremely black font.

  This enum contains the predefined font weights:

  \value Light 25
  \value Normal 50
  \value DemiBold 63
  \value Bold 75
  \value Black 87
*/

/*! Sets the weight (or boldness) of \e this font to \a weight, which should be a value
  from the QFont::Weight enumeration.

  \walkthrough fonts/simple-qfont-demo/viewer.cpp
  \skipto Tokyo
  \printline font
  \skipto setWeight
  \printline setWeight

  (Code taken from \link simple-font-demo-example.html
  fonts/simple-qfont-demo/viewer.cpp \endlink)

  Strictly speaking you can use all values in the range [0,99] (where
  0 is ultralight and 99 is extremely black), but this is perhaps
  asking too much of the underlying window system.

  If the specified weight is not available the closest available will
  be used. Use QFontInfo::weight() to check the actual weight.

  \sa weight(), QFontInfo
*/
void QFont::setWeight( int weight )
{
    if ( weight < 0 || weight > 99 ) {

#if defined(CHECK_RANGE)
	qWarning( "QFont::setWeight: Value out of range (%d)", weight );
#endif

	return;
    }

    if ((int) d->request.weight == weight) return;

    detach();
    d->request.weight = weight;
    d->request.dirty = TRUE;
}


/*! \fn bool QFont::bold() const

  Returns TRUE if weight() is a value greater than \link Weight QFont::Normal \endlink,
  otherwise FALSE.

  \sa weight(), setBold(), QFontInfo::bold()
*/

/*! \fn void QFont::setBold( bool enable )

  Sets the weight to \link Weight QFont::Bold \endlink if \a enable is TRUE, or to
  \link Weight QFont::Normal \endlink if \a enable is FALSE.

  Use setWeight() to set the boldness of \e this font to other values.

  \sa bold(), setWeight()
*/


/*! Returns whether \e this font should be drawn with underlines or not.

  Use QFontInfo to find out whether the window system font
  actually used for drawing is underlined or not.

  \sa setUnderline(), QFontInfo::underline()
*/
bool QFont::underline() const
{
    return d->request.underline;
}


/*! Switches underlining on or off, depending on the value of \a enable
  (TRUE or FALSE, respectively).

  \sa underline(), QFontInfo
*/
void QFont::setUnderline( bool enable )
{
    if ((bool) d->request.underline == enable) return;

    detach();
    d->request.underline = enable;
    d->request.dirty = TRUE;
}


/*! Returns whether strings should be striken out or not.

  Use QFontInfo to find out whether the window system font
  actually used draws a horizontal line through strings.

  \sa setStrikeOut(), QFontInfo::strikeOut().
*/
bool QFont::strikeOut() const
{
    return d->request.strikeOut;
}


/*! Sets strike out on or off, depending on the value of \a enable
  (TRUE or FALSE, respectively).

  If \a enable is TRUE a horizontal line in half character height indicating
  cancelation is drawn above strings written with \e this QFont.

  \sa strikeOut(), QFontInfo
*/
void QFont::setStrikeOut( bool enable )
{
    if ((bool) d->request.strikeOut == enable) return;

    detach();
    d->request.strikeOut = enable;
    d->request.dirty = TRUE;
}


/*! If this function returns TRUE a monospaced
  font is preferred, if FALSE a proportional font will be used if possible.

  QFontInfo reveals whether the window system font
  actually used fits this requirement.

  \sa setFixedPitch(), QFontInfo::fixedPitch()
*/
bool QFont::fixedPitch() const
{
    return d->request.fixedPitch;
}


/*! Sets monospacing on or off, depending on the value of \a enable (TRUE or
  FALSE respectively).

  A fixed pitch font is a font where all characters have the same width.

  \sa fixedPitch(), QFontInfo
*/
void QFont::setFixedPitch( bool enable )
{
    if ((bool) d->request.fixedPitch == enable) return;

    detach();
    d->request.fixedPitch = enable;
    d->request.dirty = TRUE;
}


/*! Returns the current StyleStrategy.

  \sa setStyleHint()
*/
QFont::StyleStrategy QFont::styleStrategy() const
{
    return (StyleStrategy) d->request.styleStrategy;
}


/*! Returns the current StyleHint.

  \sa setStyleHint(), QFontInfo::styleHint()
*/
QFont::StyleHint QFont::styleHint() const
{
    return (StyleHint) d->request.styleHint;
}


/*! \enum QFont::StyleHint

  Style hints are used by the font matching algorithm to find an appropriate
  default family if a selected font family is not available.

  \value AnyStyle leaves the task of finding a
  good default family to the font matching algorithm. This is the default.

  \value SansSerif prefers sans serif fonts.
  \value Helvetica indicates the same as \c SansSerif.

  \value Serif chooses fonts with serifs if possible.
  \value Times is the same as \c Serif.

  \value TypeWriter prefers fixed-pitch fonts.
  \value Courier has the same meaning as \c TypeWriter.

  \value OldEnglish chooses decorative fonts preferably.
  \value Decorative is the same as \c OldEnglish.

  \value System defers to system fonts.
*/

/*! \enum QFont::StyleStrategy

  The style strategy tells the font matching algorithm what type
  of fonts should be used to find an appropriate default family.

  The following strategies are available:

  \value PreferDefault is the default style strategy. It does not prefer
	 any type of font.
  \value PreferBitmap prefers bitmap fonts (as opposed to outline fonts).
  \value PreferDevice prefers device fonts.
  \value PreferOutline prefers outline fonts (as opposed to bitmap fonts).
  \value ForceOutline forces the use of outline fonts.

  Any of these may be ORed with an indicator whether
  \value PreferMatch exact matching or
  \value PreferQuality good quality should be preferred.

  Whilst all strategies work on Windows, \c PreferDefault and \c PreferBitmap are
  the only ones currently supported with X11.
*/

/*! Sets the style hint and strategy to \a hint and \a strategy,
  respectively.

  Without explicit setting of a style hint \link StyleHint AnyStyle\endlink
  is used.

  The style strategy defaults to \link StyleStrategy PreferDefault.\endlink
  When setting a strategy note that right now, the X version only supports bitmap fonts.

  In the example below the push button will
  display its text label with the NewYork font family if this family
  is available, if not it will display its text label with another
  serif font:

  \walkthrough fonts/simple-qfont-demo/viewer.cpp
  \skipto Newyork
  \printline Newyork
  \printline setStyleHint

  \skipto setFont
  \printline setFont

  \sa StyleHint, styleHint(), StyleStrategy, styleStrategy(), QFontInfo
*/
void QFont::setStyleHint( StyleHint hint, StyleStrategy strategy )
{
    if ((StyleHint) d->request.styleHint == hint) return;

    detach();
    d->request.styleHint = hint;
    d->request.styleStrategy = strategy;
    d->request.hintSetByUser = TRUE;
    d->request.dirty = TRUE;
}


/*! Turns raw mode on if \a enable is TRUE, or turns it off if \a
  enable is FALSE.

  Calling this function only has effect under the X Window System. If raw mode
  is enabled, Qt will search for an X font with a complete font name
  matching the family name, ignoring all other values set for the
  QFont.  If the font name matches several fonts, Qt will use the
  first font returned by X.  QFontInfo \e cannot be used to fetch
  information about a QFont using raw mode (it will return the values
  set in the QFont for all parameters, including the family name).

  \warning Do not use raw mode unless you really, really need it! In
  most (if not all) cases, setRawName() is a much better choice.

  \sa rawMode(), setRawName()
*/
void QFont::setRawMode( bool enable )
{
    if ((bool) d->request.rawMode == enable) return;

    detach();
    d->request.rawMode = enable;
    d->request.dirty = TRUE;
}


/*! Returns TRUE if a window system font exactly matching the settings
  of this font is available.

  \sa QFontInfo
*/
bool QFont::exactMatch() const
{
    d->load();

    return d->exactMatch;
}


/*! Returns TRUE if this font is equal to \a f, or FALSE if they are
  different.

  Two QFonts are equal if their font attributes are equal.  If
  rawMode() is enabled for both fonts, only the family fields are
  compared.

  \sa operator!=() isCopyOf()
*/
bool QFont::operator==( const QFont &f ) const
{
    return f.d == d || f.key() == key();
}


/*! Returns TRUE if this font is different from \a f, or FALSE if they are
  equal.

  Two QFonts are different if their font attributes are different.  If
  rawMode() is enabled for both fonts, only the family fields are
  compared.

  \sa operator==()
*/
bool QFont::operator!=( const QFont &f ) const
{
    return !(operator==( f ));
}


/*! Returns TRUE if this font and \a f are copies of each other,
  i.e. one of them was created as a copy of the other and neither was
  subsequently modified.  This is much stricter than equality.

  \sa operator=, operator==
*/
bool QFont::isCopyOf( const QFont & f ) const
{
    return d && d == f.d;
}


/*! Returns the family name that corresponds to the current style hint.

  \sa StyleHint styleHint() setStyleHint()
*/
QString QFont::defaultFamily() const
{
    return d->defaultFamily();
}


/*! Returns a last resort family name for the font matching algorithm.

  \sa lastResortFont()
*/
QString QFont::lastResortFamily() const
{
    return d->lastResortFamily();
}


/*! Returns a last resort raw font name for the font matching algorithm.
  This is used if even the last resort family is not available. It returns
  \e something, almost no matter what.

  The current implementation tries a wide variety of common fonts, returning
  the first one it finds. This implementation may change at any time.

  \sa lastResortFamily() rawName()
*/
QString QFont::lastResortFont() const
{
    return d->lastResortFont();
}


/*! Returns whether raw mode is used for font name matching (TRUE) or not (FALSE).

  \sa setRawMode() rawName()
*/
bool QFont::rawMode() const
{
    return d->request.rawMode;
}


/*! \obsolete

  Please use QApplication::font() instead.
*/
QFont QFont::defaultFont()
{
    return QApplication::font();
}


/*! \obsolete

  Please use QApplication::setFont() instead.
*/
void  QFont::setDefaultFont( const QFont &f )
{
    QApplication::setFont( f );
}







#ifndef QT_NO_STRINGLIST

/*****************************************************************************
  QFont substitution management
 *****************************************************************************/

typedef QDict<QStringList> QFontSubst;
static QFontSubst *fontSubst = 0;
QCleanupHandler<QFontSubst> qfont_cleanup_fontsubst;


// create substitution dict
static void initFontSubst()
{
    // default substitutions
    static const char *initTbl[] = {

#if defined(Q_WS_X11)
	"arial",        "helvetica",
	"helv",         "helvetica",
	"tms rmn",      "times",
#elif defined(Q_WS_WIN)
	"times",        "Times New Roman",
	"courier",      "Courier New",
	"helvetica",    "Arial",
#endif

	0,              0
    };

    if (fontSubst)
	return;

    fontSubst = new QFontSubst(17, FALSE);
    Q_CHECK_PTR( fontSubst );
    fontSubst->setAutoDelete( TRUE );
    qfont_cleanup_fontsubst.add(fontSubst);

    for ( int i=0; initTbl[i] != 0; i += 2 )
	QFont::insertSubstitution(QString::fromLatin1(initTbl[i]),
				  QString::fromLatin1(initTbl[i+1]));
}


/*! Returns the first family name to be used whenever \a familyName is
  specified. The lookup is case insensitive.

  If there is no substitution for \a familyName, \a familyName is
  returned.

  To obtain a list of all substitutions use substitutes().

  \sa setFamily() insertSubstitutions() insertSubstitution() removeSubstitution()
*/
QString QFont::substitute( const QString &familyName )
{
    initFontSubst();

    QStringList *list = fontSubst->find(familyName);
    if (list && list->count() > 0)
	return *(list->at(0));

    return familyName;
}


/*! Returns a list of family names to be used whenever \a familyName is
  specified.  The lookup is case insensitive.

  If there is no substitution for \a familyName, an empty
  list is returned.

  \walkthrough fonts/simple-qfont-demo/viewer.cpp
  \skipto QFont::substitutes(

  (Code taken from \link simple-font-demo-example.html
   fonts/simple-qfont-demo/viewer.cpp \endlink)

   \sa substitute() insertSubstitutions() insertSubstitution() removeSubstitution()
 */
QStringList QFont::substitutes(const QString &familyName)
{
    initFontSubst();

    QStringList ret, *list = fontSubst->find(familyName);
    if (list)
	ret += *list;
    return ret;
}


/*! Inserts the family name \a substituteName into the substitution
  table for \a familyName. Afterwards \a substituteName can be used if \a familyName is not
  available. The search for \a familyName is case insensitive.

  \walkthrough fonts/simple-qfont-demo/viewer.cpp
  \skipto insertSubstitution(
  \printline insertSubstitution(

  (Code taken from \link simple-font-demo-example.html
  fonts/simple-qfont-demo/viewer.cpp \endlink)

  The family name \a substituteName is appended to the substitution list
  for \a familyName only if it does not already exist in the substitution table.
  If \a substituteName is the first substitute, a new substitution table is automatically
  created.

  \sa insertSubstitutions() removeSubstitution() substitutions() substitute() substitutes()
*/
void QFont::insertSubstitution(const QString &familyName,
			       const QString &substituteName)
{
    initFontSubst();

    QStringList *list = fontSubst->find(familyName);
    if (! list) {
	list = new QStringList;
	fontSubst->insert(familyName, list);
    }

    if (! list->contains(substituteName))
	list->append(substituteName);
}


/*! Inserts the list of families \a substituteNames into the substitution
  table for \a familyName.  The search for \a familyName is case insensitive.

  \walkthrough fonts/simple-qfont-demo/viewer.cpp
  \skipto QStringList substitutes
  \printline substitutes;
  \skipto substitutes <<
  \printline substitutes <<
  \skipto insertSubstitutions( "Bavaria"
  \printline Bavaria

  (Code taken from \link simple-font-demo-example.html
  fonts/simple-qfont-demo/viewer.cpp \endlink)

  A member of \a substituteNames is appended to the substitution list for
  \a familyName only if it does not already exist in the substitution table.
  In case of a previously non-existing substitution table a new one is created automatically.

  \sa insertSubstitution, removeSubstitution, substitutions(), substitute()
*/
void QFont::insertSubstitutions(const QString &familyName,
				const QStringList &substituteNames)
{
    initFontSubst();

    QStringList *list = fontSubst->find(familyName);
    if (! list) {
	list = new QStringList;
	fontSubst->insert(familyName, list);
    }

    QStringList::ConstIterator it = substituteNames.begin();
    while (it != substituteNames.end()) {
	if (! list->contains(*it))
	    list->append(*it);
	it++;
    }
}


/*! Removes the substitution list for \a familyName from the substitution table.
  The search for \a familyName is case insensitive.

  \sa insertSubstitutions(), insertSubstitution(), substitutions(), substitute()
*/
void QFont::removeSubstitution( const QString &familyName )
{ // ### function name should be removeSubstitutions() or
  // ### removeSubstitutionList()
    initFontSubst();

    fontSubst->remove(familyName);
}


/*! Returns a sorted list of substituted family names.

  \sa insertSubstitution(), removeSubstitution(), substitute()
*/
QStringList QFont::substitutions()
{
    initFontSubst();

    QStringList ret;
    QDictIterator<QStringList> it(*fontSubst);

    while (it.current()) {
	ret.append(it.currentKey());
	++it;
    }

    ret.sort();

    return ret;
}

#endif // QT_NO_STRINGLIST


/*
  Internal function. Converts boolean font settings (except dirty)
  to an unsigned 8-bit number. Used for serialization etc.
*/
static Q_UINT8 get_font_bits( const QFontDef &f )
{
    Q_UINT8 bits = 0;
    if ( f.italic )
	bits |= 0x01;
    if ( f.underline )
	bits |= 0x02;
    if ( f.strikeOut )
	bits |= 0x04;
    if ( f.fixedPitch )
	bits |= 0x08;
    if ( f.hintSetByUser )
	bits |= 0x10;
    if ( f.rawMode )
	bits |= 0x20;
    return bits;
}


#ifndef QT_NO_DATASTREAM

/*
  Internal function. Sets boolean font settings (except dirty)
  from an unsigned 8-bit number. Used for serialization etc.
*/
static void set_font_bits( Q_UINT8 bits, QFontDef *f )
{
    f->italic        = (bits & 0x01) != 0;
    f->underline     = (bits & 0x02) != 0;
    f->strikeOut     = (bits & 0x04) != 0;
    f->fixedPitch    = (bits & 0x08) != 0;
    f->hintSetByUser = (bits & 0x10) != 0;
    f->rawMode       = (bits & 0x20) != 0;
}

#endif


/*! Returns the font's key, a textual representation of the font
  settings. It is typically used to insert and find fonts in a
  dictionary or a cache.

  \sa QMap
*/
QString QFont::key() const
{
    return d->key();
}

#ifndef QT_NO_STRINGLIST
/*! Returns a description of this font.  The description is a comma-separated
  list of the various attributes, perfectly suited for use in QSettings.

  \sa fromString()
 */
QString QFont::toString() const
{
    QStringList l;
    l.append(family());
    l.append(QString::number(pointSize()));
    l.append(QString::number((int)styleHint()));
    l.append(QString::number(weight()));
    l.append(QString::number((int)italic()));
    l.append(QString::number((int)underline()));
    l.append(QString::number((int)strikeOut()));
    l.append(QString::number((int)fixedPitch()));
    l.append(QString::number((int)rawMode()));
    return l.join(",");
}


/*! Sets this font to match the description \a descrip.  The description
  is a comma-separated list of the font attributes, as returned by toString().
 */
bool QFont::fromString(const QString &descrip)
{
    QStringList l(QStringList::split(',', descrip));

    if (l.count() != 9) {

#ifdef QT_CHECK_STATE
	qWarning("QFont::fromString: invalid description '%s'", descrip.latin1());
#endif

	return FALSE;
    }

    setFamily(l[0]);
    setPointSize(l[1].toInt());
    setStyleHint((StyleHint) l[2].toInt());
    setWeight(l[3].toInt());
    setItalic(l[4].toInt());
    setUnderline(l[5].toInt());
    setStrikeOut(l[6].toInt());
    setFixedPitch(l[7].toInt());
    setRawMode(l[8].toInt());

    return TRUE;
}
#endif // QT_NO_STRINGLIST

#if !defined( Q_WS_QWS ) // && !defined( Q_WS_MAC )
/*! \internal

  Internal function that dumps font cache statistics.
*/
void QFont::cacheStatistics()
{

#if defined(QT_DEBUG)

    QFontPrivate::fontCache->statistics();

    QFontCacheIterator it(*QFontPrivate::fontCache);
    QFontStruct *qfs;
    qDebug( "{" );
    while ( (qfs = it.current()) ) {
	++it;
#ifdef Q_WS_X11
	qDebug( "   [%s]", (const char *) qfs->name );
#elif defined(Q_WS_MAC)
	qDebug( "   [we need to implement this]"); //XXX
#else
	qDebug( "   [%s]", (const char *) qfs->key() );
#endif
    }
    qDebug( "}" );

#endif

}
#endif



/*****************************************************************************
  QFont stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM

/*! \relates QFont

  Writes the font \a font to the stream \a s.

  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/
QDataStream &operator<<( QDataStream &s, const QFont &font )
{
    if ( s.version() == 1 ) {
	QCString fam( font.d->request.family.latin1() );
	s << fam;
    } else {
	s << font.d->request.family;
    }

    return s << (Q_INT16) font.d->request.pointSize
	     << (Q_UINT8) font.d->request.styleHint
	     << (Q_UINT8) 0
	     << (Q_UINT8) font.d->request.weight
	     << get_font_bits(font.d->request);
}


/*! \relates QFont
  Reads the font \a font from the stream \a s.

  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/
QDataStream &operator>>( QDataStream &s, QFont &font )
{
    if (font.d->deref()) delete font.d;

    font.d = new QFontPrivate;

    Q_INT16 pointSize;
    Q_UINT8 styleHint, charSet, weight, bits;

    if ( s.version() == 1 ) {
	QCString fam;
	s >> fam;
	font.d->request.family = QString( fam );
    } else {
	s >> font.d->request.family;
    }

    s >> pointSize;
    s >> styleHint;
    s >> charSet;
    s >> weight;
    s >> bits;

    font.d->request.pointSize = pointSize;
    font.d->request.pixelSize = font.pixelSize();
    font.d->request.styleHint = styleHint;
    font.d->request.weight = weight;
    font.d->request.dirty = TRUE;

    set_font_bits( bits, &(font.d->request) );

    return s;
}

#endif // QT_NO_DATASTREAM




/*****************************************************************************
  QFontMetrics member functions
 *****************************************************************************/

// invariant: this list contains pointers to ALL QFontMetrics objects
// with non-null painter pointers, and no other objects.  Callers of
// these functions must maintain this invariant.

typedef QPtrList<QFontMetrics> QFontMetricsList;
static QFontMetricsList *fm_list = 0;

QCleanupHandler<QFontMetricsList> qfont_cleanup_fontmetricslist;

static void insertFontMetrics( QFontMetrics *fm ) {
    if ( !fm_list ) {
	fm_list = new QFontMetricsList;
	Q_CHECK_PTR( fm_list );
	qfont_cleanup_fontmetricslist.add( fm_list );
    }
    fm_list->append( fm );
}

static void removeFontMetrics( QFontMetrics *fm )
{
    if ( !fm_list ) {
#if defined(CHECK_NULL)
	qWarning( "QFontMetrics::~QFontMetrics: Internal error" );
#endif
	return;
    }
    fm_list->removeRef( fm );
}


/*!
  Resets all pointers to \a painter in all font metrics objects in the
  application.
*/
void QFontMetrics::reset( const QPainter *painter )
{
    if ( fm_list ) {
	QPtrListIterator<QFontMetrics> it( *fm_list );
	QFontMetrics * fm;
	while( (fm=it.current()) != 0 ) {
	    ++it;
	    if ( fm->painter == painter ) {
		fm->painter = 0;                // detach from painter
		removeFontMetrics( fm );
	    }
	}
    }
}


/*! \class QFontMetrics qfontmetrics.h
  \brief The QFontMetrics class provides font metrics information about fonts.

  \ingroup environment
  \ingroup shared

  QFontMetrics functions calculate size of characters and strings for
  a given font. There are three ways you can create a QFontMetrics object:

  The QFontMetrics constructor with a QFont creates a font metrics
  object for a screen-compatible font, i.e. the font can not be a
  printer font.

  QWidget::fontMetrics() returns the font metrics for a widget's font.
  This is equivalent to QFontMetrics(widget->font()).  Setting a new
  font for the widget later does not affect the font metrics object.

  QPainter::fontMetrics() returns the font metrics for a painter's
  current font. The font metrics object is automatically updated if
  somebody sets a new painter font (unlike the two above cases, which
  take a "snapshot" of a font).

  Once created, the object provides functions to access the individual
  metrics of the font, its characters, and for strings rendered in
  this font.

  There are several functions that operate on the font: ascent(),
  descent(), height(), leading() and lineSpacing() return the basic
  size properties of the font, and underlinePos(), strikeOutPos() and
  lineWidth() return properties of the line that underlines or strikes
  out the characters.  These functions are all fast.

  There are also some functions that operate on the set of glyphs in
  the font: minLeftBearing(), minRightBearing() and maxWidth().  These
  are by necessity slow, and we recommend avoiding them if possible.

  For each character, you can get its width(), leftBearing() and
  rightBearing() and find out whether it is in the font using
  inFont().  You can also treat the character as a string, and use the
  string functions on it.

  The string functions include width(), to return the width of a
  string in pixels (or points, for a printer), boundingRect(), to
  return the rectangle necessary to render a string, and size(), to
  return the size of that rectangle.

  Example:
  \code
    QFont font("times",24);
    QFontMetrics fm(font);
    int w = fm.width("What's the width of this text");
    int h = fm.height();
  \endcode

  \sa QFont QFontInfo QFontDatabase
*/


/*! Constructs a font metrics object for \a font.

  The font must be screen-compatible, i.e. a font you use when drawing
  text in QWidget or QPixmap objects, not QPicture or QPrinter.
  If \a font is a printer font, you'll probably get wrong results.

  Use QPainter::fontMetrics() to get the font metrics when painting.
  This is a little slower than using this constructor, but it always
  gives correct results.
*/
QFontMetrics::QFontMetrics( const QFont &font )
{
    d = font.d;
    d->ref();

    d->load();

    painter = 0;
    flags = 0;

    if (font.underline())
	setUnderlineFlag();
    if (font.strikeOut())
	setStrikeOutFlag();
}


/*! \internal

  Constructs a font metrics object for the painter \a p.
*/
QFontMetrics::QFontMetrics( const QPainter *p )
{
    painter = (QPainter *) p;

#if defined(CHECK_STATE)
    if ( !painter->isActive() )
	qWarning( "QFontMetrics: Get font metrics between QPainter::begin() "
		  "and QPainter::end()" );
#endif

    painter->setf(QPainter::FontMet);
    d = painter->cfont.d;
    d->ref();

    d->load();

    flags = 0;

    insertFontMetrics( this );
}


/*! Constructs a copy of \a fm.
*/
QFontMetrics::QFontMetrics( const QFontMetrics &fm )
    : d(fm.d), painter(fm.painter), flags(fm.flags)
{
    d->ref();
    if ( painter )
	insertFontMetrics( this );
}


/*! Destroys the font metrics object and frees all allocated resources.
*/
QFontMetrics::~QFontMetrics()
{
    if ( painter )
	removeFontMetrics( this );
    if ( d->deref() )
	delete d;
}


/*! Assigns the font metrics \a fm.
*/
QFontMetrics &QFontMetrics::operator=( const QFontMetrics &fm )
{
    if ( painter )
	removeFontMetrics( this );
    if ( d != fm.d ) {
	if ( d->deref() )
	    delete d;
	d = fm.d;
	d->ref();
    }
    painter = fm.painter;
    flags = fm.flags;
    if ( painter )
	insertFontMetrics( this );
    return *this;
}


/*! \overload

  Returns the bounding rectangle of \a ch relative to the leftmost
  point on the base line.

  Note that the bounding rectangle may extend to the left of (0,0),
  e.g. for italicized fonts, and that the text output may cover \e all
  pixels in the bounding rectangle.

  Note that the rectangle usually extends both above and below the
  base line.

  \sa width()
*/
QRect QFontMetrics::boundingRect( QChar ch ) const
{
    return d->boundingRect( ch );
}


/*! \overload

  Returns the bounding rectangle of the first \e len characters of \e str,
  which is the set of pixels the text would cover if drawn at (0,0). The
  drawing, and hence the bounding rectangle, is constrained to the rectangle
  \a (x,y,w,h).

  If \a len is negative (default value), the whole string is used.

  The \a flgs argument is
  the bitwise OR of the following flags:  <ul>
  <li> \c AlignAuto aligns to the left border for all languages except hebrew and arabic where it aligns to the right..
  <li> \c AlignLeft aligns to the left border.
  <li> \c AlignRight aligns to the right border.
  <li> \c AlignJustify produces justified text.
  <li> \c AlignHCenter aligns horizontally centered.
  <li> \c AlignTop aligns to the top border.
  <li> \c AlignBottom aligns to the bottom border.
  <li> \c AlignVCenter aligns vertically centered
  <li> \c AlignCenter (= \c AlignHCenter | AlignVCenter)
  <li> \c SingleLine ignores newline characters in the text.
  <li> \c ExpandTabs expands tabulators.
  <li> \c ShowPrefix interprets "&x" as "x" underlined.
  <li> \c WordBreak breaks the text to fit the rectangle.
  </ul>

  Horizontal alignment defaults to AlignAuto and vertical alignment
  defaults to AlignTop.

  If several of the horizontal or several of the vertical alignment flags
  are set, the resulting alignment is undefined.

  These flags are defined in qnamespace.h.

  If \c ExpandTabs is set in \a flgs, then:
  if \a tabarray is non.zero, it specifies a 0-terminated sequence
  of pixel-positions for tabs; otherwise
  if \a tabstops is non-zero, it is used as the tab spacing (in pixels).

  Note that the bounding rectangle may extend to the left of (0,0),
  e.g. for italicized fonts, and that the text output may cover \e all
  pixels in the bounding rectangle.

  Newline characters are processed as linebreaks.

  Despite the different actual character heights, the heights of the
  bounding rectangles of "Yes" and "yes" are the same.

  The bounding rectangle given by this function is somewhat larger
  than that calculated by the simpler boundingRect() function.  This
  function uses the \link minLeftBearing() maximum left \endlink and
  \link minRightBearing() right \endlink font bearings as is necessary
  for multi-line text to align correctly.  Also, fontHeight() and
  lineSpacing() are used to calculate the height, rather than
  individual character heights.

  The \a intern argument is for internal purposes.

  \sa width(), QPainter::boundingRect(), Qt::AlignmentFlags
*/
QRect QFontMetrics::boundingRect( int x, int y, int w, int h, int flgs,
				  const QString& str, int len, int tabstops,
				  int *tabarray, QTextParag **intern ) const
{
    if ( len < 0 )
	len = str.length();

    int tabarraylen=0;
    if (tabarray)
	while (tabarray[tabarraylen])
	    tabarraylen++;

    QRect rb;
    QRect r(x, y, w, h);
    qt_format_text( QFont( d, FALSE ), r, flgs, str, len, &rb,
		    tabstops, tabarray, tabarraylen, intern, 0 );

    return rb;
}


/*! Returns the size in pixels of the first \a len characters of \a str.

  If \a len is negative (default value), the whole string is used.

  The \a flgs argument is
  the bitwise OR of the following flags:  <ul>
  <li> \c SingleLine ignores newline characters in the text.
  <li> \c ExpandTabs expands tabulators.
  <li> \c ShowPrefix interprets "&x" as "x" underlined.
  <li> \c WordBreak breaks the text to fit the rectangle.
  </ul>

  These flags are defined in qnamespace.h.

  If \c ExpandTabs is set in \a flgs, then:
  if \a tabarray is non.zero, it specifies a 0-terminated sequence
  of pixel-positions for tabs; otherwise
  if \a tabstops is non-zero, it is used as the tab spacing (in pixels).

  Newline characters are processed as linebreaks.

  Despite the different actual character heights, the heights of the
  bounding rectangles of "Yes" and "yes" are the same.

  The \a intern argument is for internal purposes.

  \sa boundingRect()
*/
QSize QFontMetrics::size( int flgs, const QString &str, int len, int tabstops,
			  int *tabarray, QTextParag **intern ) const
{
    return boundingRect(0,0,1,1,flgs,str,len,tabstops,tabarray,intern).size();
}


/*****************************************************************************
  QFontInfo member functions
 *****************************************************************************/

// invariant: this list contains pointers to ALL QFontInfo objects
// with non-null painter pointers, and no other objects.  Callers of
// these functions must maintain this invariant.

typedef QPtrList<QFontInfo> QFontInfoList;
static QFontInfoList *fi_list = 0;

QCleanupHandler<QFontInfoList> qfont_cleanup_fontinfolist;

static void insertFontInfo( QFontInfo *fi )
{
    if ( !fi_list ) {
	fi_list = new QFontInfoList;
	Q_CHECK_PTR( fi_list );
	qfont_cleanup_fontinfolist.add( fi_list );
    }
    fi_list->append( fi );
}

static void removeFontInfo( QFontInfo *fi )
{
    if ( !fi_list ) {
#if defined(CHECK_NULL)
	qWarning( "QFontInfo::~QFontInfo: Internal error" );
#endif
	return;
    }
    fi_list->removeRef( fi );
}


/*!
  Resets all pointers to \a painter in all font metrics objects in the
  application.
*/
void QFontInfo::reset( const QPainter *painter )
{
    if ( fi_list ) {
	QPtrListIterator<QFontInfo> it( *fi_list );
	QFontInfo * fi;
	while( (fi=it.current()) != 0 ) {
	    ++it;
	    if ( fi->painter == painter ) {
		fi->painter = 0;                // detach from painter
		removeFontInfo( fi );
	    }
	}
    }
}


/*! \class QFontInfo qfontinfo.h

  \brief The QFontInfo class provides general information about fonts.

  \ingroup environment
  \ingroup shared

  The QFontInfo class mirrors QFont exactly, but where QFont access
  functions returns set values, QFontInfo returns the values that
  apply to the font in use.

  For example, when the program asks for a 25pt Courier font on a
  machine that has a 24pt Courier font but not a scalable one, QFont
  will (normally) use the 24pt Courier for rendering.  In this case,
  QFont::pointSize() returns 25 and QFontInfo::pointSize() 24.

  The access functions in QFontInfo mirror QFont exactly, except for
  this difference.

  There are three ways to create a QFontInfo object.

  The QFontInfo constructor with a QFont creates a font info object
  for a screen-compatible font, i.e. the font can not be a printer
  font.

  QWidget::fontInfo() returns the font info for a widget's font.  This
  is equivalent to QFontInfo(widget->font()).  Setting a new font for
  the widget later does not affect the font info object.

  QPainter::fontInfo() returns the font info for a painter's current
  font. The font info object is automatically updated if somebody sets
  a new painter font, unlike the two above cases, which take a
  "snapshot" of a font.

  \sa QFont QFontMetrics QFontDatabase
*/


/*! Constructs a font info object for \a font.

  The font must be screen-compatible, i.e. a font you use when drawing
  text in \link QWidget widgets\endlink or \link QPixmap pixmaps\endlink.
  If \a font is a printer font, you'll probably get wrong results.

  Use the QPainter::fontInfo() to get the font info when painting.
  This is a little slower than using this constructor, but it always
  gives correct results.
*/
QFontInfo::QFontInfo( const QFont &font )
{
    d = font.d;
    d->ref();

    d->load();

    painter = 0;
    flags = 0;

    if ( font.underline() )
	setUnderlineFlag();
    if ( font.strikeOut() )
	setStrikeOutFlag();
    if ( font.exactMatch() )
	setExactMatchFlag();
}


/*! \internal

  Constructs a font info object for the painter \a p.
*/
QFontInfo::QFontInfo( const QPainter *p )
{
    painter = (QPainter *) p;

#if defined(CHECK_STATE)
    if ( !painter->isActive() )
	qWarning( "QFontInfo: Get font info between QPainter::begin() "
		  "and QPainter::end()" );
#endif

    painter->setf( QPainter::FontInf );
    d = painter->cfont.d;
    d->ref();

    d->load();

    flags = 0;

    insertFontInfo( this );
}


/*! Constructs a copy of \a fi.
*/
QFontInfo::QFontInfo( const QFontInfo &fi )
    : d(fi.d), painter(fi.painter), flags(fi.flags)
{
    d->ref();
    if ( painter )
	insertFontInfo( this );
}


/*! Destroys the font info object.
*/
QFontInfo::~QFontInfo()
{
    if ( painter )
	removeFontInfo( this );
    if (d->deref())
	delete d;
}


/*! Assigns the font info in \a fi.
*/
QFontInfo &QFontInfo::operator=( const QFontInfo &fi )
{
    if ( painter )
	removeFontInfo( this );
    if (d != fi.d) {
	if (d->deref())
	    delete d;
	d = fi.d;
	d->ref();
    }
    painter = fi.painter;
    flags = fi.flags;
    if ( painter )
	insertFontInfo( this );
    return *this;
}


/*! Returns the family name of the matched window system font.

  \sa QFont::family()
*/
QString QFontInfo::family() const
{
    return d->actual.family;
}


/*! Returns the point size of the matched window system font.

  \sa QFont::pointSize()
*/
int QFontInfo::pointSize() const
{
    return d->actual.pointSize / 10;
}

/*! Returns the pixel size of the matched window system font.

  \sa QFont::pointSize()
*/
int QFontInfo::pixelSize() const
{
    return d->actual.pixelSize;
}


/*! Returns the italic value of the matched window system font.

  \sa QFont::italic()
*/
bool QFontInfo::italic() const
{
    return d->actual.italic;
}


/*! Returns the weight of the matched window system font.

  \sa QFont::weight(), bold()
*/
int QFontInfo::weight() const
{
    return d->actual.weight;
}


/*! \fn bool QFontInfo::bold() const

  Returns TRUE if weight() would return a greater than
  \c QFont::Normal, and FALSE otherwise.

  \sa weight(), QFont::bold()
*/

/*! Returns the underline value of the matched window system font.

  \sa QFont::underline()

  \internal

  Here we read the underline flag directly from the QFont.
  This is OK for X11 and for Windows because we always get what we want.
*/
bool QFontInfo::underline() const
{
    return painter ? painter->font().underline() : underlineFlag();
}


/*! Returns the strike out value of the matched window system font.

  \sa QFont::strikeOut()

  \internal Here we read the strikeOut flag directly from the QFont.
  This is OK for X11 and for Windows because we always get what we want.
*/
bool QFontInfo::strikeOut() const
{
    return painter ? painter->font().strikeOut() : strikeOutFlag();
}


/*! Returns the fixed pitch value of the matched window system font.
  A fixed pitch font is a font that has constant character pixel width.

  \sa QFont::fixedPitch()
*/
bool QFontInfo::fixedPitch() const
{
    return d->actual.fixedPitch;
}


/*! Returns the style of the matched window system font.

  Currently only returns the hint set in QFont.

  \sa QFont::styleHint()
*/
QFont::StyleHint QFontInfo::styleHint() const
{
    return (QFont::StyleHint) d->actual.styleHint;
}


/*! Returns TRUE if the font is a raw mode font.

  If it is a raw mode font, all other functions in QFontInfo will return the
  same values set in the QFont, regardless of the font actually used.

  \sa QFont::rawMode()
*/
bool QFontInfo::rawMode() const
{
    return d->actual.rawMode;
}


/*! Returns TRUE if the matched window system font is exactly the one specified
  by the font.

  \sa QFont::exactMatch()
*/
bool QFontInfo::exactMatch() const
{
    return painter ? painter->font().exactMatch() : exactMatchFlag();
}



#ifndef Q_WS_QWS
// **********************************************************************
// QFontCache
// **********************************************************************

static const int qtFontCacheMin = 2*1024*1024;
static const int qtFontCacheSize = 61;

// when debugging the font cache - clean it out more aggressively
#ifndef QFONTCACHE_DEBUG
static const int qtFontCacheFastTimeout =  30000;
static const int qtFontCacheSlowTimeout = 300000;
#else // !QFONTCACHE_DEBUG
static const int qtFontCacheFastTimeout = 10000;
static const int qtFontCacheSlowTimeout = 30000;
#endif // QFONTCACHE_DEBUG

QFontCache *QFontPrivate::fontCache = 0;


QFontCache::QFontCache() :
    QObject(0, "global font cache"),
    QCache<QFontStruct>(qtFontCacheMin, qtFontCacheSize, FALSE),
    timer_id(0), fast(FALSE)
{
    setAutoDelete(TRUE);
}


QFontCache::~QFontCache()
{
    // remove negative cache items
    QFontCacheIterator it(*this);
    QString key;
    QFontStruct *qfs;

    while ((qfs = it.current())) {
	key = it.currentKey();
	++it;

	if (qfs == (QFontStruct *) -1)
	    take(key);
    }
}


bool QFontCache::insert(const QString &key, const QFontStruct *qfs, int cost)
{

#ifdef QFONTCACHE_DEBUG
    qDebug("QFC::insert: inserting %p w/ cost %d\n%s", qfs, cost,
	   (qfs == (QFontStruct *) -1) ? "negative cache item" : qfs->name.data());
#endif // QFONTCACHE_DEBUG

    if (totalCost() + cost > maxCost()) {

#ifdef QFONTCACHE_DEBUG
	qDebug("QFC::insert: adjusting max cost to %d (%d %d)",
	       totalCost() + cost, totalCost(), maxCost());
#endif // QFONTCACHE_DEBUG

	setMaxCost(totalCost() + cost);
    }

    bool ret = QCache<QFontStruct>::insert(key, qfs, cost);

    if (ret && (! timer_id || ! fast)) {
	if (timer_id) {

#ifdef QFONTCACHE_DEBUG
	    qDebug("QFC::insert: killing old timer");
#endif // QFONTCACHE_DEBUG

	    killTimer(timer_id);
	}

#ifdef QFONTCACHE_DEBUG
	qDebug("QFC::insert: starting timer");
#endif // QFONTCACHE_DEBUG

	timer_id = startTimer(qtFontCacheFastTimeout);
	fast = TRUE;
    }

    return ret;
}


void QFontCache::deleteItem(Item d)
{
    QFontStruct *qfs = (QFontStruct *) d;

    // don't try to delete negative cache items
    if (qfs == (QFontStruct *) -1)
	return;

    if (qfs->count == 0) {

#ifdef QFONTCACHE_DEBUG
	qDebug("QFC::deleteItem: removing %p from cache\n%s", qfs,
	       (qfs == (QFontStruct *) -1) ? "negative cache item" : qfs->name.data());
#endif // QFONTCACHE_DEBUG

	delete qfs;
    }
}


void QFontCache::timerEvent(QTimerEvent *)
{
    if (maxCost() <= qtFontCacheMin) {

#ifdef QFONTCACHE_DEBUG
	qDebug("QFC::timerEvent: cache max cost is less than min, killing timer");
#endif // QFONTCACHE_DEBUG

	setMaxCost(qtFontCacheMin);

	killTimer(timer_id);
	timer_id = 0;
	fast = TRUE;

	return;
    }

    QFontCacheIterator it(*this);
    QString key;
    QFontStruct *qfs;
    int tqcost = maxCost() * 3 / 4;
    int nmcost = 0;

    while ((qfs = it.current())) {
	key = it.currentKey();
	++it;

	if (qfs != (QFontStruct *) -1) {
	    if (qfs->count > 0)
		nmcost += qfs->cache_cost;
	} else
	    // keep negative cache items in the cache
	    nmcost++;
    }

    nmcost = QMAX(tqcost, nmcost);
    if (nmcost < qtFontCacheMin)
	nmcost = qtFontCacheMin;

    if (nmcost == totalCost()) {
	if (fast) {

#ifdef QFONTCACHE_DEBUG
	    qDebug("QFC::timerEvent: slowing timer");
#endif // QFONTCACHE_DEBUG

	    killTimer(timer_id);

	    timer_id = startTimer(qtFontCacheSlowTimeout);
	    fast = FALSE;
	}
    } else if (! fast) {
	// cache size is changing now, but we're still on the slow timer... time to
	// drop into passing gear

#ifdef QFONTCACHE_DEBUG
	qDebug("QFC::timerEvent: dropping into passing gear");
#endif // QFONTCACHE_DEBUG

	killTimer(timer_id);
	timer_id = startTimer(qtFontCacheFastTimeout);
	fast = TRUE;
    }

#ifdef QFONTCACHE_DEBUG
    qDebug("QFC::timerEvent: before cache cost adjustment: %d %d",
	   totalCost(), maxCost());
#endif // QFONTCACHE_DEBUG

    setMaxCost(nmcost);

#ifdef QFONTCACHE_DEBUG
    qDebug("QFC::timerEvent:  after cache cost adjustment: %d %d",
	   totalCost(), maxCost());
#endif // QFONTCACHE_DEBUG

}
#endif




// **********************************************************************
// QFontPrivate member methods
// **********************************************************************

// Converts a weight string to a value
int QFontPrivate::getFontWeight(const QCString &weightString, bool adjustScore)
{
    // Test in decreasing order of commonness
    if ( weightString == "medium" )       return QFont::Normal;
    else if ( weightString == "bold" )    return QFont::Bold;
    else if ( weightString == "demibold") return QFont::DemiBold;
    else if ( weightString == "black" )   return QFont::Black;
    else if ( weightString == "light" )   return QFont::Light;

    QCString s(weightString.lower());

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

    return (int) QFont::Normal;
}

QString QFontPrivate::key() const
{
    if (request.rawMode)
	return request.family;

    int len = (request.family.length() * 2) +
	      2 +  // point size
	      1 +  // font bits
	      1 +  // weight
	      1;   // hint

    QByteArray buf(len);
    uchar *p = (uchar *) buf.data();

    memcpy((char *) p, (char *) request.family.unicode(),
	   (request.family.length() * 2));

    p += request.family.length() * 2;

    *((Q_UINT16 *) p) = request.pointSize; p += 2;
    *p++ = get_font_bits( request );
    *p++ = request.weight;
    *p++ = (request.hintSetByUser ?
	    (int) request.styleHint : (int) QFont::AnyStyle);

    return QString((QChar *) buf.data(), buf.size() / 2);
}

QFont::Script QFontPrivate::scriptForChar( const QChar &c )
{
#ifndef Q_WS_QWS
    uchar row = c.row();
    uchar cell = c.cell();

    // Thankfully LatinBasic is more or less == ISO-8859-1
    if (! row)
	return QFont::LatinBasic;

    switch ( row ) {
    case 0x01:
	// There are no typos here... really...
	switch (cell) {
	case 0x00: return QFont::LatinExtendedA_4;
	case 0x01: return QFont::LatinExtendedA_4;
	case 0x02: return QFont::LatinExtendedA_2;
	case 0x03: return QFont::LatinExtendedA_2;
	case 0x04: return QFont::LatinExtendedA_2;
	case 0x05: return QFont::LatinExtendedA_2;
	case 0x06: return QFont::LatinExtendedA_2;
	case 0x07: return QFont::LatinExtendedA_2;
	case 0x08: return QFont::LatinExtendedA_3;
	case 0x09: return QFont::LatinExtendedA_3;
	case 0x0A: return QFont::LatinExtendedA_3;
	case 0x0B: return QFont::LatinExtendedA_3;
	case 0x0C: return QFont::LatinExtendedA_2;
	case 0x0D: return QFont::LatinExtendedA_2;
	case 0x0E: return QFont::LatinExtendedA_2;
	case 0x0F: return QFont::LatinExtendedA_2;
	case 0x10: return QFont::LatinExtendedA_2;
	case 0x11: return QFont::LatinExtendedA_2;
	case 0x12: return QFont::LatinExtendedA_4;
	case 0x13: return QFont::LatinExtendedA_4;
	case 0x16: return QFont::LatinExtendedA_4;
	case 0x17: return QFont::LatinExtendedA_4;
	case 0x18: return QFont::LatinExtendedA_2;
	case 0x19: return QFont::LatinExtendedA_2;
	case 0x1A: return QFont::LatinExtendedA_2;
	case 0x1B: return QFont::LatinExtendedA_2;
	case 0x1C: return QFont::LatinExtendedA_3;
	case 0x1D: return QFont::LatinExtendedA_3;
	case 0x1E: return QFont::LatinExtendedA_3;
	case 0x1F: return QFont::LatinExtendedA_3;
	case 0x20: return QFont::LatinExtendedA_3;
	case 0x21: return QFont::LatinExtendedA_3;
	case 0x22: return QFont::LatinExtendedA_4;
	case 0x23: return QFont::LatinExtendedA_4;
	case 0x24: return QFont::LatinExtendedA_3;
	case 0x25: return QFont::LatinExtendedA_3;
	case 0x26: return QFont::LatinExtendedA_3;
	case 0x27: return QFont::LatinExtendedA_3;
	case 0x28: return QFont::LatinExtendedA_4;
	case 0x29: return QFont::LatinExtendedA_4;
	case 0x2A: return QFont::LatinExtendedA_4;
	case 0x2B: return QFont::LatinExtendedA_4;
	case 0x2E: return QFont::LatinExtendedA_4;
	case 0x2F: return QFont::LatinExtendedA_4;
	case 0x30: return QFont::LatinExtendedA_3;
	case 0x31: return QFont::LatinExtendedA_3;
	case 0x34: return QFont::LatinExtendedA_3;
	case 0x35: return QFont::LatinExtendedA_3;
	case 0x36: return QFont::LatinExtendedA_4;
	case 0x37: return QFont::LatinExtendedA_4;
	case 0x38: return QFont::LatinExtendedA_4;
	case 0x39: return QFont::LatinExtendedA_2;
	case 0x3A: return QFont::LatinExtendedA_2;
	case 0x3B: return QFont::LatinExtendedA_4;
	case 0x3C: return QFont::LatinExtendedA_4;
	case 0x3D: return QFont::LatinExtendedA_2;
	case 0x3E: return QFont::LatinExtendedA_2;
	case 0x41: return QFont::LatinExtendedA_2;
	case 0x42: return QFont::LatinExtendedA_2;
	case 0x43: return QFont::LatinExtendedA_2;
	case 0x44: return QFont::LatinExtendedA_2;
	case 0x45: return QFont::LatinExtendedA_4;
	case 0x46: return QFont::LatinExtendedA_4;
	case 0x47: return QFont::LatinExtendedA_2;
	case 0x48: return QFont::LatinExtendedA_2;
	case 0x4A: return QFont::LatinExtendedA_4;
	case 0x4B: return QFont::LatinExtendedA_4;
	case 0x4C: return QFont::LatinExtendedA_4;
	case 0x4D: return QFont::LatinExtendedA_4;
	case 0x50: return QFont::LatinExtendedA_2;
	case 0x51: return QFont::LatinExtendedA_2;
	case 0x52: return QFont::LatinExtendedA_15;
	case 0x53: return QFont::LatinExtendedA_15;
	case 0x54: return QFont::LatinExtendedA_2;
	case 0x55: return QFont::LatinExtendedA_2;
	case 0x56: return QFont::LatinExtendedA_4;
	case 0x57: return QFont::LatinExtendedA_4;
	case 0x58: return QFont::LatinExtendedA_2;
	case 0x59: return QFont::LatinExtendedA_2;
	case 0x5A: return QFont::LatinExtendedA_2;
	case 0x5B: return QFont::LatinExtendedA_2;
	case 0x5C: return QFont::LatinExtendedA_3;
	case 0x5D: return QFont::LatinExtendedA_3;
	case 0x5E: return QFont::LatinExtendedA_2;
	case 0x5F: return QFont::LatinExtendedA_2;
	case 0x60: return QFont::LatinExtendedA_2;
	case 0x61: return QFont::LatinExtendedA_2;
	case 0x62: return QFont::LatinExtendedA_2;
	case 0x63: return QFont::LatinExtendedA_2;
	case 0x64: return QFont::LatinExtendedA_2;
	case 0x65: return QFont::LatinExtendedA_2;
	case 0x66: return QFont::LatinExtendedA_4;
	case 0x67: return QFont::LatinExtendedA_4;
	case 0x68: return QFont::LatinExtendedA_4;
	case 0x69: return QFont::LatinExtendedA_4;
	case 0x6A: return QFont::LatinExtendedA_4;
	case 0x6B: return QFont::LatinExtendedA_4;
	case 0x6C: return QFont::LatinExtendedA_3;
	case 0x6D: return QFont::LatinExtendedA_3;
	case 0x6E: return QFont::LatinExtendedA_2;
	case 0x6F: return QFont::LatinExtendedA_2;
	case 0x70: return QFont::LatinExtendedA_2;
	case 0x71: return QFont::LatinExtendedA_2;
	case 0x72: return QFont::LatinExtendedA_4;
	case 0x73: return QFont::LatinExtendedA_4;
	case 0x74: return QFont::LatinExtendedA_14;
	case 0x75: return QFont::LatinExtendedA_14;
	case 0x76: return QFont::LatinExtendedA_14;
	case 0x77: return QFont::LatinExtendedA_14;
	case 0x78: return QFont::LatinExtendedA_15;
	case 0x79: return QFont::LatinExtendedA_2;
	case 0x7A: return QFont::LatinExtendedA_2;
	case 0x7B: return QFont::LatinExtendedA_2;
	case 0x7C: return QFont::LatinExtendedA_2;
	case 0x7D: return QFont::LatinExtendedA_2;
	case 0x7E: return QFont::LatinExtendedA_2;
	}
	return QFont::Latin;

    case 0x02:
	if (cell <= 0xaf)
	    return QFont::Latin;
	return QFont::SpacingModifiers;

    case 0x03:
	if (cell <= 0x6f)
	    return QFont::CombiningMarks;
	return QFont::Greek;

    case 0x04:
	return QFont::Cyrillic;

    case 0x05:
	if (cell <= 0x2f)
	    break;
	if (cell <= 0x8f)
	    return QFont::Armenian;
	return QFont::Hebrew;

    case 0x06:
	return QFont::Arabic;

    case 0x07:
	if (cell <= 0x4f)
	    return QFont::Syriac;
	if (cell <= 0x7f)
	    break;
	if (cell <= 0xbf)
	    return QFont::Thaana;
	break;

    case 0x09:
	if (cell <= 0x7f)
	    return QFont::Devanagari;
	return QFont::Bengali;

    case 0x0a:
	if (cell <= 0x7f)
	    return QFont::Gurmukhi;
	return QFont::Gujarati;

    case 0x0b:
	if ( cell <= 0x7f )
	    return QFont::Oriya;
	return QFont::Tamil;

    case 0x0c:
	if (cell <= 0x7f)
	    return QFont::Telugu;
	return QFont::Kannada;

    case 0x0d:
	if (cell <= 0x7f)
	    return QFont::Malayalam;
	return QFont::Sinhala;

    case 0x0e:
	if (cell <= 0x7f)
	    return QFont::Thai;
	return QFont::Lao;

    case 0x0f:
	if (cell <= 0xbf)
	    return QFont::Tibetan;
	break;

    case 0x10:
	if (cell <= 0x9f)
	    return QFont::Myanmar;
	return QFont::Georgian;

    case 0x11:
	return QFont::Hangul;

    case 0x12:
	return QFont::Ethiopic;

    case 0x13:
	if (cell <= 0x7f)
	    return QFont::Ethiopic;
	if (cell <= 0x8f)
	    break;
	return QFont::Cherokee;

    case 0x14:
    case 0x15:
	return QFont::CanadianAboriginal;

    case 0x16:
	if (cell <= 0x7f)
	    return QFont::CanadianAboriginal;
	if (cell <= 0x9f)
	    return QFont::Ogham;
	if (cell <= 0xf0)
	    return QFont::Runic;
	break;

    case 0x17:
	if (cell <= 0x7f)
	    break;
	return QFont::Khmer;

    case 0x18:
	if (cell <= 0xaf)
	    return QFont::Mongolian;
	break;

    case 0x1e:
	return QFont::Latin;

    case 0x1f:
	return QFont::Greek;

    case 0x20:
	if (cell <= 0x6f)
	    break;
	if (cell <= 0x9f)
	    return QFont::NumberForms;
	if (cell <= 0xcf)
	    return QFont::CurrencySymbols;
	return QFont::CombiningMarks;

    case 0x21:
	if (cell <= 0x4f)
	    return QFont::LetterlikeSymbols;
	if (cell <= 0x8f)
	    return QFont::NumberForms;
	return QFont::MathematicalOperators;

    case 0x22:
	return QFont::MathematicalOperators;

    case 0x23:
	return QFont::TechnicalSymbols;

    case 0x24:
	if (cell <= 0x5f)
	    return QFont::TechnicalSymbols;
	return QFont::EnclosedAndSquare;

    case 0x25:
	return QFont::GeometricSymbols;

    case 0x26:
    case 0x27:
	return QFont::MiscellaneousSymbols;

    case 0x28:
	return QFont::Braille;

    case 0x2e:
	if (cell <= 0x7f)
	    break;
#ifdef Q_WS_X11
	return hanHack( c );
#else
	return QFont::Han;
#endif

    case 0x2f:
	if (cell <= 0xd5) {
#ifdef Q_WS_X11
	    return hanHack( c );
#else
	    return QFont::Han;
#endif
	}
	if (cell <= 0xef)
	    break;
#ifdef Q_WS_X11
	return hanHack( c );
#else
	return QFont::Han;
#endif

    case 0x30:
	if (cell <= 0x3f) {
	    // Unified Han Symbols and Punctuation
#ifdef Q_WS_X11
	    return hanHack( c );
#else
	    return QFont::Han;
#endif
	}
	if (cell <= 0x9f)
	    return QFont::Hiragana;
	return QFont::Katakana;

    case 0x31:
	if (cell <= 0x2f)
	    return QFont::Bopomofo;

	// Hangul Compatibility Jamo
	if (cell <= 0x8f)
	    return QFont::Hangul;
	if (cell <= 0x9f) {
#ifdef Q_WS_X11
	    return hanHack( c );
#else
	    return QFont::Han;
#endif
	}
	break;

    case 0x32:
    case 0x33:
	return QFont::EnclosedAndSquare;

    case 0xa0:
    case 0xa1:
    case 0xa2:
    case 0xa3:
	return QFont::Yi;
    case 0xa4:
	if (cell <= 0xcf)
	    return QFont::Yi;
	break;

    case 0xfb:
	if (cell <= 0x06)
	    return QFont::Latin;
	if (cell <= 0x1c)
	    break;
	if (cell <= 0x4f)
	    return QFont::Hebrew;
	return QFont::Arabic;

    case 0xfc:
    case 0xfd:
	return QFont::Arabic;

    case 0xfe:
	if (cell <= 0x1f)
	    break;
	if (cell <= 0x2f)
	    return QFont::CombiningMarks;
	if (cell <= 0x6f)
	    break;
	return QFont::Arabic;

    case 0xff:
	// Hiragana half/full width forms block
	if (cell <= 0xef)
	    return QFont::Hiragana;
	break;
    }

    // Canadian Aboriginal Syllabics
    if (row >= 0x14 && (row < 0x16 || (row == 0x16 && cell <= 0x7f))) {
	return QFont::CanadianAboriginal;
    }

    // Hangul Syllables
    if (row >= 0xac && (row < 0xd7 || (row == 0xd7 && cell <= 0xa3))) {
	return QFont::Hangul;
    }

    if (// Unified Han + Extension-A
	(row >= 0x34 && row <= 0x9f) ||
	// Unified Han Compatibility
	(row >= 0xf9 && row <= 0xfa)
	) {
#ifdef Q_WS_X11
	return hanHack( c );
#else
	return QFont::Han;
#endif
    }

    // return QFont::UnknownScript;
#endif //Q_WS_QWS
    // "Qt/Embedded is Unicode throughout..."
    return QFont::Unicode;
}
