/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qspinbox.h#25 $
**
** Definition of QSpinBox widget class
**
** Created : 1997
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

#ifndef QSPINBOX_H
#define QSPINBOX_H

#ifndef QT_H
#include "qframe.h"
#include "qrangecontrol.h"
#endif // QT_H

class QPushButton;
class QLineEdit;
class QValidator;
struct QSpinBoxData;


class QSpinBox: public QFrame, public QRangeControl
{
    Q_OBJECT
public:
    QSpinBox( QWidget* parent = 0, QString name = 0 );
    QSpinBox( int minValue, int maxValue, int step = 1,
	      QWidget* parent = 0, QString name = 0 );
    ~QSpinBox();

    QString 	text() const;
    virtual QString 	prefix() const;
    virtual QString 	suffix() const;
    virtual QString 	cleanText() const;

    virtual void		setSpecialValueText( QString text );
    QString 	specialValueText() const;

    virtual void 		setWrapping( bool on );
    bool 		wrapping() const;

    virtual void		setValidator( QValidator* v );

    QSize 		sizeHint() const;

public slots:
    virtual void	setValue( int value );
    virtual void	setPrefix( QString text );
    virtual void	setSuffix( QString text );
    virtual void	stepUp();
    virtual void	stepDown();

signals:
    void		valueChanged( int value );
    void		valueChanged( QString valueText );

protected:
    virtual QString	mapValueToText( int value );
    virtual int		mapTextToValue( bool* ok );
    QString		currentValueText();

    virtual void	updateDisplay();
    virtual void	interpretText();

    QPushButton*	upButton() const;
    QPushButton*	downButton() const;
    QLineEdit*		editor() const;

    virtual void	valueChange();
    virtual void	rangeChange();

    bool		eventFilter( QObject* obj, QEvent* ev );
    void		resizeEvent( QResizeEvent* ev );
    void		wheelEvent( QWheelEvent * );

    void		paletteChange( const QPalette& );
    void		enabledChange( bool );
    void		fontChange( const QFont& );
    void		styleChange( GUIStyle );

protected slots:
    void		textChanged();

private:
    void initSpinBox();
    struct QSpinBoxData* extra;
    QPushButton* up;
    QPushButton* down;
    QLineEdit* vi;
    QValidator* validator;
    QString pfix;
    QString sfix;
    QString specText;
    bool wrap;
    bool edited;
};


#endif
