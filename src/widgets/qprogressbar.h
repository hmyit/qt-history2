/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qprogressbar.h#23 $
**
** Definition of QProgressBar class
**
** Created : 970520
**
** Copyright (C) 1992-2000 Troll Tech AS.  All rights reserved.
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
** See http://www.troll.no/pricing.html or email sales@troll.no for
** information about the Professional Edition licensing, or see
** http://www.troll.no/qpl/ for QPL licensing information.
**
*****************************************************************************/

#ifndef QPROGRESSBAR_H
#define QPROGRESSBAR_H

#ifndef QT_H
#include "qframe.h"
#endif // QT_H


class QProgressBarPrivate;


class Q_EXPORT QProgressBar : public QFrame
{
    Q_OBJECT
    Q_PROPERTY( int totalSteps READ totalSteps WRITE setTotalSteps )
    Q_PROPERTY( int progress READ progress WRITE setProgress )
    Q_PROPERTY( bool centerIndicator READ centerIndicator WRITE setCenterIndicator )
    Q_PROPERTY( bool indicatorFollowsStyle READ indicatorFollowsStyle WRITE setIndicatorFollowsStyle )
	
public:
    QProgressBar( QWidget *parent=0, const char *name=0, WFlags f=0 );
    QProgressBar( int totalSteps, QWidget *parent=0, const char *name=0,
		  WFlags f=0 );

    int		totalSteps() const;
    int		progress()   const;

    QSize	sizeHint() const;
    QSizePolicy sizePolicy() const;
    QSize	minimumSizeHint() const;

    void	setCenterIndicator( bool on );
    bool	centerIndicator() const;

    void        setIndicatorFollowsStyle( bool );
    bool	indicatorFollowsStyle() const;

    void	show();

public slots:
    void	reset();
    virtual void setTotalSteps( int totalSteps );
    virtual void setProgress( int progress );

protected:
    void	drawContents( QPainter * );
    void	drawContentsMask( QPainter * );
    virtual bool setIndicator( QString & progress_str, int progress,
			       int totalSteps );
    void styleChange( QStyle& );

private:
    int		total_steps;
    int		progress_val;
    int		percentage;
    QString	progress_str;
    bool        center_indicator;
    bool        auto_indicator;
    QProgressBarPrivate * d;
    void         initFrame();

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QProgressBar( const QProgressBar & );
    QProgressBar &operator=( const QProgressBar & );
#endif
};


inline int QProgressBar::totalSteps() const
{
    return total_steps;
}

inline int QProgressBar::progress() const
{
    return progress_val;
}

inline bool QProgressBar::centerIndicator() const
{
    return center_indicator;
}

inline bool QProgressBar::indicatorFollowsStyle() const
{
    return auto_indicator;
}


#endif // QPROGRESSBAR_H
