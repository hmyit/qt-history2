/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qpalette.h#37 $
**
** Definition of QColorGroup and QPalette classes
**
** Created : 950323
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

#ifndef QPALETTE_H
#define QPALETTE_H

#ifndef QT_H
#include "qwindowdefs.h"
#include "qcolor.h"
#include "qshared.h"
#include "qbrush.h"
#endif // QT_H


class Q_EXPORT QColorGroup				// color group class
{
public:
    QColorGroup();				// all colors black
    QColorGroup( const QColor &foreground, const QColor &button,
		 const QColor &light, const QColor &dark, const QColor &mid,
		 const QColor &text, const QColor &base );
    QColorGroup( const QBrush &foreground, const QBrush &button,
		 const QBrush &light, const QBrush &dark, const QBrush &mid,
		 const QBrush &text,  const QBrush &bright_text, const QBrush &base,
		 const QBrush &background);

   ~QColorGroup();

    const QColor &foreground()	const	{ return foreground_brush.color(); }
    const QColor &button()	const	{ return button_brush.color(); }
    const QColor &light()	const	{ return light_brush.color(); }
    const QColor &midlight()	const	{ return midlight_brush.color(); }
    const QColor &dark()	const	{ return dark_brush.color(); }
    const QColor &mid()		const	{ return mid_brush.color(); }
    const QColor &text()	const	{ return text_brush.color(); }
    const QColor &brightText()	const	{ return bright_text_brush.color(); }
    const QColor &buttonText()	const	{ return button_text_brush.color(); }
    const QColor &base()	const	{ return base_brush.color(); }
    const QColor &background()	const	{ return background_brush.color(); }
    const QColor &shadow()	const	{ return shadow_brush.color(); }
    const QColor &highlight()	const	{ return highlight_brush.color(); }
    const QColor &highlightedText()	const	{ return highlighted_text_brush.color(); }

    const QBrush &fillForeground() const {return foreground_brush; }
    const QBrush &fillButton() const {return button_brush; }
    const QBrush &fillLight() const {return light_brush; }
    const QBrush &fillMidlight() const {return midlight_brush; }
    const QBrush &fillDark() const {return dark_brush; }
    const QBrush &fillMid() const {return mid_brush; }
    const QBrush &fillText() const {return text_brush; }
    const QBrush &fillBrightText() const {return bright_text_brush; }
    const QBrush &fillButtonText() const {return button_text_brush; }
    const QBrush &fillBase() const {return base_brush; }
    const QBrush &fillBackground() const {return background_brush; }
    const QBrush &fillShadow() const {return shadow_brush; }
    const QBrush &fillHighlight() const {return highlight_brush; }
    const QBrush &fillHighlightedText() const {return highlighted_text_brush; }

    void setForeground( const QBrush& b) { foreground_brush = b; }
    void setButton( const QBrush& b) { button_brush = b; }
    void setLight( const QBrush& b) { light_brush = b; }
    void setMidlight( const QBrush& b) { midlight_brush = b; }
    void setDark( const QBrush& b) { dark_brush = b; }
    void setMid( const QBrush& b) { mid_brush = b; }
    void setText( const QBrush& b) { text_brush = b; }
    void setBrightText( const QBrush& b) { bright_text_brush = b; }
    void setButtonText( const QBrush& b) { button_text_brush = b; }
    void setBase( const QBrush& b) { base_brush = b; }
    void setBackground( const QBrush& b) { background_brush = b; }
    void setShadow( const QBrush& b) { shadow_brush = b; }
    void setHighlight( const QBrush& b) { highlight_brush = b; }
    void setHighlightedText( const QBrush& b) { highlighted_text_brush = b; }

    bool	operator==( const QColorGroup &g ) const;
    bool	operator!=( const QColorGroup &g ) const
	{ return !(operator==(g)); }

private:
    QBrush foreground_brush;
    QBrush button_brush;
    QBrush light_brush;
    QBrush dark_brush;
    QBrush mid_brush;
    QBrush text_brush;
    QBrush bright_text_brush;
    QBrush button_text_brush;
    QBrush base_brush;
    QBrush background_brush;
    QBrush midlight_brush;
    QBrush shadow_brush;
    QBrush highlight_brush;
    QBrush highlighted_text_brush;
};


class Q_EXPORT QPalette					// palette class
{
public:
    QPalette();
    QPalette( const QColor &button );
    QPalette( const QColor &button, const QColor &background );
    QPalette( const QColorGroup &normal, const QColorGroup &disabled,
	      const QColorGroup &active );
    QPalette( const QPalette & );
   ~QPalette();
    QPalette &operator=( const QPalette & );

    QPalette	copy() const;

    const QColorGroup &normal()	  const { return data->normal; }
    const QColorGroup &disabled() const { return data->disabled; }
    const QColorGroup &active()	  const { return data->active; }

    void	setNormal( const QColorGroup & );
    void	setDisabled( const QColorGroup & );
    void	setActive( const QColorGroup & );

    bool	operator==( const QPalette &p ) const;
    bool	operator!=( const QPalette &p ) const
					{ return !(operator==(p)); }
    bool	isCopyOf( const QPalette & );

    int		serialNumber() const	{ return data->ser_no; }

private:
    void	detach();

    struct QPalData : public QShared {
	QColorGroup normal;
	QColorGroup disabled;
	QColorGroup active;
	int	    ser_no;
    } *data;
};


/*****************************************************************************
  QColorGroup/QPalette stream functions
 *****************************************************************************/

Q_EXPORT QDataStream &operator<<( QDataStream &, const QColorGroup & );
Q_EXPORT QDataStream &operator>>( QDataStream &, QColorGroup & );

Q_EXPORT QDataStream &operator<<( QDataStream &, const QPalette & );
Q_EXPORT QDataStream &operator>>( QDataStream &, QPalette & );


#endif // QPALETTE_H
