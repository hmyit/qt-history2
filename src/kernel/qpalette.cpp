/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qpalette.cpp#29 $
**
** Implementation of QColorGroup and QPalette classes
**
** Created : 950323
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

#include "qpalette.h"
#include "qdatastream.h"
#include "qpixmap.h"

/*****************************************************************************
  QColorGroup member functions
 *****************************************************************************/

/*!
  \class QColorGroup qpalette.h
  \brief The QColorGroup class contains a group of widget colors.

  \ingroup color
  \ingroup drawing

  A color group contains a group of colors used by widgets for drawing
  themselves.  Widgets should not use colors like "red" and "turqoise"
  but rather "foreground" and "base", where possible.

  We have identified fourty-two distinct color roles:
  <ol>
  <li>Foreground (graphics foreground color)
  <li>Button (general button color)
  <li>Light (lighter than button color, for shadow effects)
  <li>Midlight (between Button and Light, for shadow effects)
  <li>Dark (darker than the button color, for shadow effects)
  <li>Medium (between button color  and dark, used for shadow and contrast
    effects)
  <li>Text (usually the same as the foreground color, but sometimes text
    and other foreground are not the same)
  <li>BrightText (a text color that contrasts to the Dark color)
  <li>ButtonText (a text color that contrasts to the Button  color)
  <li>Base (used as background color for some widgets). Usually white or
    another light color.
  <li>Background (general background color)
  <li>Shadow (a very dark color used for shadow effects, usually black)
  <li>Highlight  (a color to indicate a selected or highlighted item)
  <li>HighlightedText  (a text color that contrasts to Highlight)
  </ol>

  We have not seen any good, well-made and usable widgets that use more
  than these eight color roles.

  A QPalette contains three color groups.

  The current widget color group is returned by QWidget::colorGroup().

  \sa QColor, QPalette
*/


/*!
  Constructs a color group with all colors set to black.
*/

QColorGroup::QColorGroup()
{						// all colors become black
}


/*!
Constructs a color group. You can pass either brushes, pixmaps or
plain colors for each parameter.

\sa QBrush
*/
 QColorGroup::QColorGroup( const QBrush &foreground, const QBrush &button,
 			  const QBrush &light, const QBrush &dark, const QBrush &mid,
 			  const QBrush &text,  const QBrush &bright_text, const QBrush &base,
 			  const QBrush &background)
 {
     foreground_brush = foreground;
     button_brush = button;
     light_brush = light;
     dark_brush = dark;
     mid_brush = mid;
     text_brush = text;
     bright_text_brush = bright_text;
     button_text_brush = text;
     base_brush = base;
     background_brush = background;
     midlight_brush = QBrush(button_brush.color().light(115));
     highlight_brush = Qt::darkBlue;
     highlighted_text_brush = Qt::white;
 }


/*!\obsolete
  Constructs a color group with the specified colors. The background
  color will be set to the button color.
*/

QColorGroup::QColorGroup( const QColor &foreground, const QColor &button,
			  const QColor &light, const QColor &dark,
			  const QColor &mid,
			  const QColor &text, const QColor &base )
{
    foreground_brush    = QBrush(foreground);
    dark_brush  = QBrush(dark);
    text_brush  = QBrush(text);
    bright_text_brush  = text_brush;
    button_text_brush  = text_brush;
    background_brush    = QBrush(button);
    button_brush = QBrush( button);
    light_brush = QBrush( light);
    mid_brush = QBrush( mid);
    base_brush = QBrush( base);
    midlight_brush = QBrush(button_brush.color().light(115));
     highlight_brush = Qt::darkBlue;
     highlighted_text_brush = Qt::white;
}

/*!
  Destroys the color group.
*/

QColorGroup::~QColorGroup()
{
}


/*!
  \fn const QColor & QColorGroup::foreground() const
  Returns the foreground color of the color group.
*/

/*!
  \fn const QColor & QColorGroup::button() const
  Returns the button color of the color group.
*/

/*!
  \fn const QColor & QColorGroup::light() const
  Returns the light color of the color group.
*/

/*!
  \fn const QColor& QColorGroup::midlight() const
  Returns the midlight color of the color group. Currently, this is
  a lightened version of the button color, but this may change
  in the future, to return a <tt>const QColor &</tt> from the
  palette.
*/

/*!
  \fn const QColor & QColorGroup::dark() const
  Returns the dark color of the color group.
*/

/*!
  \fn const QColor & QColorGroup::mid() const
  Returns the medium color of the color group.
*/

/*!
  \fn const QColor & QColorGroup::text() const
  Returns the text foreground color of the color group.
*/

/*!
  \fn const QColor & QColorGroup::brightText() const
  Returns the bright text foreground color of the color group.
*/

/*!
  \fn const QColor & QColorGroup::buttonText() const
  Returns the button text foreground color of the color group.
*/

/*!
  \fn const QColor & QColorGroup::base() const
  Returns the base color of the color group.
*/

/*!
  \fn const QColor & QColorGroup::background() const
  Returns the background color of the color group.
*/

/*!
  \fn const QBrush & QColorGroup::fillForeground() const
  Returns the foreground brush of the color group.
*/

/*!
  \fn const QBrush & QColorGroup::fillButton() const
  Returns the button brush of the color group.
*/

/*!
  \fn const QBrush & QColorGroup::fillLight() const
  Returns the light brush of the color group.
*/

/*!
  \fn const QBrush& QColorGroup::fillMidlight() const
  Returns the midlight brush of the color group. Currently, this is
  a lightened version of the button brush, but this may change
  in the future, to return a <tt>const QBrush &</tt> from the
  palette.
*/

/*!
  \fn const QBrush & QColorGroup::fillDark() const
  Returns the dark brush of the color group.
*/

/*!
  \fn const QBrush & QColorGroup::fillMid() const
  Returns the medium brush of the color group.
*/

/*!
  \fn const QBrush & QColorGroup::fillText() const
  Returns the text foreground brush of the color group.
*/

/*!
  \fn const QBrush & QColorGroup::fillBrightText() const
  Returns the bright text foreground brush of the color group.
*/

/*!
  \fn const QBrush & QColorGroup::fillButtonText() const
  Returns the button text foreground brush of the color group.
*/

/*!
  \fn const QBrush & QColorGroup::fillBase() const
  Returns the base brush of the color group.
*/

/*!
  \fn const QBrush & QColorGroup::fillBackground() const
  Returns the background brush of the color group.
*/

/*!
  \fn bool QColorGroup::operator!=( const QColorGroup &g ) const
  Returns TRUE if this color group is different from \e g, or FALSE if
  it is equal to \e g.
  \sa operator!=()
*/

/*!
  Returns TRUE if this color group is equal to \e g, or FALSE if
  it is different from \e g.
  \sa operator==()
*/

bool QColorGroup::operator==( const QColorGroup &g ) const
{
    return foreground_brush == g.foreground_brush    &&
	   background_brush == g.background_brush   &&
		light_brush == g.light_brush && dark_brush == g.dark_brush &&
		mid_brush   == g.mid_brush   && text_brush == g.text_brush &&
	  bright_text_brush == g.bright_text_brush && button_text_brush == g.button_text_brush &&
		base_brush  == g.base_brush &&
	     midlight_brush == g.midlight_brush && shadow_brush == g.shadow_brush &&
	    highlight_brush == g.highlight_brush && highlighted_text_brush == g.highlighted_text_brush;
}


/*****************************************************************************
  QPalette member functions
 *****************************************************************************/

/*!
  \class QPalette qpalette.h

  \brief The QPalette class contains color groups for each widget state.

  \ingroup color
  \ingroup shared
  \ingroup drawing

  A palette consists of three color groups: a \e normal, a \e disabled
  and an \e active color group.	 All \link QWidget widgets\endlink
  contain a palette, and all the widgets in Qt use their palette to draw
  themselves.  This makes the user interface consistent and easily
  configurable.

  If you make a new widget you are strongly advised to use the colors in
  the palette rather than hard-coding specific colors.

  The \e active group is used for the widget in focus.	Normally it
  contains the same colors as \e normal so as not to overwhelm the user
  with bright and flashing colors, but if you need to you can change it.

  The \e disabled group is used for widgets that are currently
  inactive or not usable.

  The \e normal color group is used in all other cases.

  \sa QApplication::setPalette(), QWidget::setPalette(), QColorGroup, QColor
*/


static int palette_count = 1;

/*!
  Constructs a palette that consists of color groups with only black colors.
*/

QPalette::QPalette()
{
    data = new QPalData;
    CHECK_PTR( data );
    data->ser_no = palette_count++;
}

/*!\obsolete
  Constructs a palette from the \e button color. The other colors are
  automatically calculated, based on this color. Background will be
  the button color as well.
*/

QPalette::QPalette( const QColor &button )
{
    data = new QPalData;
    CHECK_PTR( data );
    data->ser_no = palette_count++;
    QColor bg = button, btn = button, fg, base, disfg;
    int h, s, v;
    bg.hsv( &h, &s, &v );
    if ( v > 128 ) {				// light background
	fg   = Qt::black;
	base = Qt::white;
	disfg = Qt::darkGray;
    } else {					// dark background
	fg   = Qt::white;
	base = Qt::black;
	disfg = Qt::darkGray;
    }
    data->normal   = QColorGroup( fg, btn, btn.light(150), btn.dark(),
				  btn.dark(150), fg, Qt::white, base, bg );
    data->active   = data->normal;
    data->disabled = QColorGroup( disfg, btn, btn.light(150), btn.dark(),
				  btn.dark(150), disfg, Qt::white, base, bg );
}

/*!
  Constructs a palette from a \e button color and a background. The other colors are
  automatically calculated, based on this color.
*/

QPalette::QPalette( const QColor &button, const QColor &background )
{
    data = new QPalData;
    CHECK_PTR( data );
    data->ser_no = palette_count++;
    QColor bg = background, btn = button, fg, base, disfg;
    int h, s, v;
    bg.hsv( &h, &s, &v );
    if ( v > 128 ) {				// light background
	fg   = Qt::black;
	base = Qt::white;
	disfg = Qt::darkGray;
    } else {					// dark background
	fg   = Qt::white;
	base = Qt::black;
	disfg = Qt::darkGray;
    }
    data->normal   = QColorGroup( fg, btn, btn.light(150), btn.dark(),
				  btn.dark(150), fg, Qt::white, base, bg );
    data->active   = data->normal;
    data->disabled = QColorGroup( disfg, btn, btn.light(150), btn.dark(),
				  btn.dark(150), disfg, Qt::white, base, bg );
}

/*!
  Constructs a palette that consists of the three color groups \e normal,
  \e disabled and \e active.
*/

QPalette::QPalette( const QColorGroup &normal, const QColorGroup &disabled,
		    const QColorGroup &active )
{
    data = new QPalData;
    CHECK_PTR( data );
    data->ser_no = palette_count++;
    data->normal = normal;
    data->disabled = disabled;
    data->active = active;
}

/*!
  Constructs a palette that is a
  \link shclass.html shallow copy\endlink of \e p.
  \sa copy()
*/

QPalette::QPalette( const QPalette &p )
{
    data = p.data;
    data->ref();
}

/*!
  Destroys the palette.
*/

QPalette::~QPalette()
{
    if ( data->deref() )
	delete data;
}

/*!
  Assigns \e p to this palette and returns a reference to this palette.
  Note that a \e shallow copy of \a p is used.
  \sa copy()
*/

QPalette &QPalette::operator=( const QPalette &p )
{
    p.data->ref();
    if ( data->deref() )
	delete data;
    data = p.data;
    return *this;
}


/*!
  Returns a
  \link shclass.html deep copy\endlink of the palette.
*/

QPalette QPalette::copy() const
{
    QPalette p( data->normal, data->disabled, data->active );
    return p;
}


/*!
  Detaches this palette from any other QPalette objects with which
  it might implicitly share \link QColorGroup QColorGroups. \endlink

  Calling this should generally not be necessary; QPalette calls this
  itself when necessary.
*/

void QPalette::detach()
{
    if ( data->count != 1 )
	*this = copy();
}


/*!
  \fn const QColorGroup & QPalette::normal() const
  Returns the normal color group of this palette.
  \sa QColorGroup, disabled(), active(), setNormal()
*/

/*!
  Sets the \c normal color group to \e g.
  \sa normal()
*/

void QPalette::setNormal( const QColorGroup &g )
{
    detach();
    data->ser_no = palette_count++;
    data->normal = g;
}

/*!
  \fn const QColorGroup & QPalette::disabled() const
  Returns the disabled color group of this palette.
  \sa QColorGroup, normal(), active(), setDisabled()
*/

/*!
  Sets the \c disabled color group to \e g.
  \sa disabled()
*/

void QPalette::setDisabled( const QColorGroup &g )
{
    detach();
    data->ser_no = palette_count++;
    data->disabled = g;
}

/*!
  \fn const QColorGroup & QPalette::active() const
  Returns the active color group of this palette.
  \sa QColorGroup, normal(), disabled(), setActive()
*/

/*!
  Sets the \c active color group to \e g.
  \sa active()
*/

void QPalette::setActive( const QColorGroup &g )
{
    detach();
    data->ser_no = palette_count++;
    data->active = g;
}


/*!
  \fn bool QPalette::operator!=( const QPalette &p ) const
  Returns TRUE if this palette is different from \e p, or FALSE if they
  are equal.
*/

/*!
  Returns TRUE if this palette is equal to \e p, or FALSE if they
  are different.
*/

bool QPalette::operator==( const QPalette &p ) const
{
    return data->normal == p.data->normal &&
	   data->disabled == p.data->disabled &&
	   data->active == p.data->active;
}


/*!
  \fn int QPalette::serialNumber() const

  Returns a number that uniquely identifies this QPalette object. The
  serial number is very useful for caching.

  \sa QPixmap, QPixmapCache
*/


/*****************************************************************************
  QColorGroup/QPalette stream functions
 *****************************************************************************/

/*!
  \relates QColorGroup
  Writes a color group to the stream.

  Serialization format:
  <ol>
  <li> QColor foreground
  <li> QColor button
  <li> QColor light
  <li> QColor dark
  <li> QColor mid
  <li> QColor text
  <li> QColor base
  <li> QColor background
  </ol>
  The colors are serialized in the listed order.
*/

QDataStream &operator<<( QDataStream &s, const QColorGroup &g )
{
    return s << g.foreground()
	     << g.button()
	     << g.light()
	     << g.dark()
	     << g.mid()
	     << g.text()
	     << g.base()
	     << g.background();
}

/*!
  \related QColorGroup
  Reads a color group from the stream.
*/

QDataStream &operator>>( QDataStream &s, QColorGroup &g )
{
    QColor fg;
    QColor button;
    QColor light;
    QColor dark;
    QColor mid;
    QColor text;
    QColor bright_text;
    QColor base;
    QColor bg;
    s >> fg >> button >> light >> dark >> mid >> text >> bright_text >> base >> bg;
    QColorGroup newcg( fg, button, light, dark, mid, text, bright_text, base, bg );
    g = newcg;
    return s;
}


/*!
  \relates QPalette
  Writes a palette to the stream and returns a reference to the stream.

  Serialization format:
  <ol>
  <li> QColorGroup normal
  <li> QColorGroup disabled
  <li> QColorGroup active
  </ol>
  The color groups are serialized in the listed order.
*/

QDataStream &operator<<( QDataStream &s, const QPalette &p )
{
    return s << p.normal()
	     << p.disabled()
	     << p.active();
}

/*!
  \relates QPalette
  Reads a palette from the stream and returns a reference to the stream.
*/

QDataStream &operator>>( QDataStream &s, QPalette &p )
{
    QColorGroup normal, disabled, active;
    s >> normal >> disabled >> active;
    QPalette newpal( normal, disabled, active );
    p = newpal;
    return s;
}


/*!  Returns TRUE if this palette and \a p are copies of each other,
  ie. one of them was created as a copy of the other and neither was
  subsequently modified.  This is much stricter than equality.

  \sa operator=, operator==
*/

bool QPalette::isCopyOf( const QPalette & p )
{
    return data && data == p.data;
}
