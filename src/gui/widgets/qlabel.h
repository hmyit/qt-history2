/****************************************************************************
**
** Definition of QLabel widget class.
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QLABEL_H
#define QLABEL_H

#ifndef QT_H
#include "qframe.h"
#endif // QT_H

#ifndef QT_NO_LABEL

class QLabelPrivate;

class Q_GUI_EXPORT QLabel : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(Qt::TextFormat textFormat READ textFormat WRITE setTextFormat)
    Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap)
    Q_PROPERTY(bool scaledContents READ hasScaledContents WRITE setScaledContents)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
    Q_PROPERTY(int margin READ margin WRITE setMargin)
    Q_PROPERTY(int indent READ indent WRITE setIndent)
    Q_OVERRIDE(Qt::BackgroundMode backgroundMode DESIGNABLE true)
    Q_DECLARE_PRIVATE(QLabel)

public:
#ifdef QT_COMPAT
    QLabel(QWidget *parent, const char* name, Qt::WFlags f=0);
    QLabel(const QString &text, QWidget *parent, const char* name,
           Qt::WFlags f=0);
    QLabel(QWidget *buddy, const QString &,
           QWidget *parent=0, const char* name=0, Qt::WFlags f=0);
#endif
    QLabel(QWidget *parent=0, Qt::WFlags f=0);
    QLabel(const QString &text, QWidget *parent=0, Qt::WFlags f=0);
    ~QLabel();

    QString text() const;
    QPixmap *pixmap() const;
#ifndef QT_NO_PICTURE
    QPicture *picture() const;
#endif
#ifndef QT_NO_MOVIE
    QMovie *movie() const;
#endif

    Qt::TextFormat textFormat() const;
    void setTextFormat(Qt::TextFormat);

    int alignment() const;
    void setAlignment(int);

    int indent() const;
    void setIndent(int);

    int margin() const;
    void setMargin(int);

#ifndef QT_NO_IMAGE_SMOOTHSCALE
    bool hasScaledContents() const;
    void setScaledContents(bool);
#endif
    QSize sizeHint() const;
    QSize minimumSizeHint() const;
#ifndef QT_NO_ACCEL
    void setBuddy(QWidget *);
    QWidget *buddy() const;
#endif
    int heightForWidth(int) const;

public slots:
    void setText(const QString &);
    void setPixmap(const QPixmap &);
#ifndef QT_NO_PICTURE
    void setPicture(const QPicture &);
#endif
#ifndef QT_NO_MOVIE
    void setMovie(const QMovie &);
#endif
    void setNum(int);
    void setNum(double);
    void clear();

protected:
    bool event(QEvent *e);
    void paintEvent(QPaintEvent *);
    void changeEvent(QEvent *);

private slots:
#ifndef QT_NO_ACCEL
    void mnemonicSlot();
#endif
#ifndef QT_NO_MOVIE
    void movieUpdated(const QRect&);
    void movieResized(const QSize&);
#endif

private: // Disabled copy constructor and operator=
    friend class QTipLabel;

#if defined(Q_DISABLE_COPY)
    QLabel(const QLabel &);
    QLabel &operator=(const QLabel &);
#endif
};


#endif // QT_NO_LABEL

#endif // QLABEL_H
