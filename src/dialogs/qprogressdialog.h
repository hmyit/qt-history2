/****************************************************************************
** $Id: //depot/qt/main/src/dialogs/qprogressdialog.h#16 $
**
** Definition of QProgressDialog class
**
** Created : 970520
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

#ifndef QPROGRESSDIALOG_H
#define QPROGRESSDIALOG_H

#ifndef QT_H
#include "qsemimodal.h"
#include "qpushbutton.h"
#include "qlabel.h"
#include "qprogressbar.h"
#endif // QT_H

struct QProgressData;


class QProgressDialog : public QSemiModal
{
    Q_OBJECT
public:
    QProgressDialog( QWidget *parent=0, const char *name=0, bool modal=FALSE,
		     WFlags f=0 );
    QProgressDialog( QString labelText, QString cancelButtonText,
		     int totalSteps, QWidget *parent=0, const char *name=0,
		     bool modal=FALSE, WFlags f=0 );
   ~QProgressDialog();

    void	setLabel( QLabel * );
    void	setCancelButton( QPushButton * );
    void	setBar( QProgressBar * );

    bool	wasCancelled() const;

    int		totalSteps() const;
    int		progress()   const;

    QSize	sizeHint() const;

public slots:
    void	cancel();
    void	reset();
    void	setTotalSteps( int totalSteps );
    void	setProgress( int progress );
    void	setLabelText( QString );
    void	setCancelButtonText( QString );

signals:
    void	cancelled();

protected:
    void	resizeEvent( QResizeEvent * );
    void	styleChange(GUIStyle);

private:
    void	   init( QWidget *creator, QString lbl, QString canc,
		         int totstps);
    void	   center();
    void	   layout();
    QLabel	  *label()  const;
    QProgressBar  *bar()    const;
    QProgressData *d;

private:	// Disabled copy constructor and operator=
    QProgressDialog( const QProgressDialog & );
    QProgressDialog &operator=( const QProgressDialog & );
};


#endif // QPROGRESSDIALOG_H
