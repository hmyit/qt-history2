/****************************************************************************
** $Id: //depot/qt/main/examples/widgets/widgets.h#1 $
**
** Definition of something or other
**
** Created : 979899
**
** Copyright (C) 1997 by Troll Tech AS.  All rights reserved.
**
****************************************************************************/

#ifndef WIDGETS_H
#define WIDGETS_H

#include <qmainwindow.h>
#include <qmovie.h>
class QLabel;
class QCheckBox;
class QProgressBar;

//
// WidgetView contains lots of Qt widgets.
//

class WidgetView : public QMainWindow
{
    Q_OBJECT
public:
    WidgetView( QWidget *parent=0, const char *name=0 );

public slots:
    void	setStatus(const QString&);

protected slots:
   virtual void button1Clicked();
private slots:
    void	checkBoxClicked( int );
    void	radioButtonClicked( int );
    void	sliderValueChanged( int );
    void	listBoxItemSelected( int );
    void	comboBoxItemActivated( int );
    void	edComboBoxItemActivated( const QString& );
    void	lineEditTextChanged( const QString& );
    void	movieStatus( int );
    void	movieUpdate( const QRect& );
    void	spinBoxValueChanged( const QString& );

    void	open();
    void	dummy();
    
private:
    bool	eventFilter( QObject *, QEvent * );
    QLabel     *msg;
    QCheckBox  *cb[3];
    QLabel     *movielabel;
    QMovie      movie;
    QWidget *central;
    QProgressBar *prog;
    int progress;
};


#endif
