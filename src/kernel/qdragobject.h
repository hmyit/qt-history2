/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qdragobject.h#16 $
**
** Definition of QDragObject
**
** Copyright (C) 1997 by Troll Tech AS.  All rights reserved.
**
****************************************************************************/

#ifndef QDRAGOBJECT_H
#define QDRAGOBJECT_H

struct QDragData;
struct QStoredDragData;
class QWidget;

#ifndef QT_H
#include "qobject.h"
#include "qimage.h"
#include "qstrlist.h"
#endif // QT_H


class QDragObject: public QObject {
    Q_OBJECT
public:
    QDragObject( QWidget * dragSource = 0, const char * name = 0 );
    ~QDragObject();

    virtual void setAutoDelete( bool );
    bool autoDelete() const;

    virtual void startDrag();

    virtual bool provides(const char*) const;
    virtual const char * format(int) const=0;
    virtual QByteArray encodedData(const char*) const=0;

    QWidget * source();

private:
    QDragData * d;
};

class QStoredDrag: public QDragObject {
    Q_OBJECT
    QStoredDragData * d;

public:
    QStoredDrag( const char * mimeType,
	QWidget * dragSource = 0, const char * name = 0 );
    ~QStoredDrag();

    void setEncodedData( QByteArray & );

    const char * format(int i) const;
    virtual QByteArray encodedData(const char*) const;
};

class QTextDrag: public QStoredDrag {
    Q_OBJECT
public:
    QTextDrag( const char *,
		     QWidget * parent = 0, const char * name = 0 );
    QTextDrag( QWidget * parent = 0, const char * name = 0 );
    ~QTextDrag();

    void setText( const char * );

    static bool canConvert( QDragMoveEvent* e );
    static bool convert( QDropEvent* e, QString& s );
};


class QImageDrag: public QDragObject {
    Q_OBJECT
    QImage img;
    QStrList ofmts;

public:
    QImageDrag( QImage image,
		      QWidget * parent = 0, const char * name = 0 );
    QImageDrag( QWidget * parent = 0, const char * name = 0 );
    ~QImageDrag();

    void setImage( QImage image );

    const char * format(int i) const;
    virtual QByteArray encodedData(const char*) const;

    static bool canConvert( QDragMoveEvent* e );
    static bool convert( QDropEvent* e, QImage& i );
};


// QDragManager is not part of the public API.  It is defined in a
// header file simply so different .cpp files can implement different
// member functions.
//

class QDragManager: public QObject {
    Q_OBJECT

private:
    QDragManager();
    ~QDragManager();
    // only friend classes can use QDragManager.
    friend class QDragObject;

    bool eventFilter( QObject *, QEvent * );

    void startDrag( QDragObject * );

    void cancel();
    void move( const QPoint & );
    void drop();

private:
    QDragObject * object;

    QWidget * dragSource;
    QWidget * dropWidget;
    bool beingCancelled;
    bool restoreCursor;
    bool willDrop;
};


#endif
