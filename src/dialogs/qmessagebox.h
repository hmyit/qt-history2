/****************************************************************************
** $Id: //depot/qt/main/src/dialogs/qmessagebox.h#19 $
**
** Definition of QMessageBox class
**
** Created : 950503
**
** Copyright (C) 1995-1996 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QMSGBOX_H
#define QMSGBOX_H

#include "qdialog.h"

class  QLabel;
class  QPushButton;
struct QMBData;


class QMessageBox : public QDialog
{
    Q_OBJECT
public:
    enum { OK = 1, Cancel = 2, Yes = 3, No = 4, Abort = 5, Retry = 6,
	   Ignore = 7, ButtonMask = 0x07,
	   Default = 0x100, Escape = 0x200, FlagMask = 0x300 };
    enum Icon { NoIcon = 0, Information = 1, Warning = 2, Critical = 3 };

    static int information( QWidget *parent, const char *caption,
			    const char *text,
			    int button1, int button2=0, int button3=0 );
    static int warning( QWidget *parent, const char *caption,
			const char *text,
			int button1, int button2, int button3=0 );
    static int critical( QWidget *parent, const char *caption,
			 const char *text,
			 int button1, int button2, int button3=0 );

#if 1 /* OBSOLETE */
    static int message( const char *caption,
			const char *text,  const char *buttonText=0,
			QWidget *parent=0, const char *name=0 );

    static bool query( const char *caption,
		       const char *text,  const char *yesButtonText=0,
		       const char *noButtonText=0,
		       QWidget *parent=0, const char *name=0 );
#endif

    QMessageBox( QWidget *parent=0, const char *name=0 );
    QMessageBox( int button1, int button2, int button3,
		 QWidget *parent=0, const char *name=0 );
   ~QMessageBox();

    const char *text() const;
    void	setText( const char * );

    QMessageBox::Icon icon() const;
    void	setIcon( Icon );

#if 1 /* OBSOLETE */
    const char *buttonText() const;
    void	setButtonText( const char * );
#endif

    const char *buttonText( int button ) const;
    void	setButtonText( int button, const char * );

    void	adjustSize();

protected:
    void	resizeEvent( QResizeEvent * );
    void	keyPressEvent( QKeyEvent * );

private slots:
    void	buttonClicked();

private:
    void	init( int, int, int );
    int		indexOf( int ) const;
    void	resizeButtons();
    QLabel     *label;
    QMBData    *mbd;
    void       *reserved1;
    void       *reserved2;

private:	// Disabled copy constructor and operator=
    QMessageBox( const QMessageBox & ) {}
    QMessageBox &operator=( const QMessageBox & ) { return *this; }
};


#endif // QMSGBOX_H
