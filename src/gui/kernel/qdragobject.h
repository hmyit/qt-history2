/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QDRAGOBJECT_H
#define QDRAGOBJECT_H

class QWidget;
class QTextDragPrivate;
class QDragObjectPrivate;
class QStoredDragPrivate;
class QImageDragPrivate;

#include "qobject.h"
#include "qcolor.h"
#include "qmime.h"
# ifndef QT_INCLUDE_COMPAT
#  include "qimage.h"
#  include "qlist.h"
# endif

class QImage;
template <class T> class QList;

#ifndef QT_NO_MIME

class Q_GUI_EXPORT QDragObject: public QObject, public QMimeSource {
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDragObject)
public:
    QDragObject(QWidget *dragSource = 0);
#ifdef QT_COMPAT
    QT_COMPAT_CONSTRUCTOR QDragObject(QWidget * dragSource, const char *name);
#endif
    virtual ~QDragObject();

#ifndef QT_NO_DRAGANDDROP
    bool drag();
    bool dragMove();
    void dragCopy();
    void dragLink();

    virtual void setPixmap(QPixmap);
    virtual void setPixmap(QPixmap, const QPoint& hotspot);
    QPixmap pixmap() const;
    QPoint pixmapHotSpot() const;
#endif

    QWidget * source();
    static QWidget * target();

    static void setTarget(QWidget*);

#ifndef QT_NO_DRAGANDDROP
    enum DragMode { DragDefault, DragCopy, DragMove, DragLink, DragCopyOrMove };

protected:
    QDragObject(QDragObjectPrivate &, QWidget *dragSource = 0);
    virtual bool drag(DragMode);
#endif

private:
    Q_DISABLE_COPY(QDragObject)
};

class Q_GUI_EXPORT QStoredDrag: public QDragObject {
    Q_OBJECT
    Q_DECLARE_PRIVATE(QStoredDrag)
public:
    QStoredDrag(const char *mimeType, QWidget *dragSource = 0);
#ifdef QT_COMPAT
    QT_COMPAT_CONSTRUCTOR QStoredDrag(const char *mimeType, QWidget *dragSource, const char *name);
#endif
    ~QStoredDrag();

    virtual void setEncodedData(const QByteArray &);

    const char * format(int i) const;
    virtual QByteArray encodedData(const char*) const;

protected:
    QStoredDrag(QStoredDragPrivate &, const char *mimeType, QWidget *dragSource = 0);

private:
    Q_DISABLE_COPY(QStoredDrag)
};

class Q_GUI_EXPORT QTextDrag: public QDragObject {
    Q_OBJECT
    Q_DECLARE_PRIVATE(QTextDrag)
public:
    QTextDrag(const QString &, QWidget *dragSource = 0);
    QTextDrag(QWidget *dragSource = 0);
#ifdef QT_COMPAT
    QT_COMPAT_CONSTRUCTOR QTextDrag(const QString &, QWidget *dragSource, const char *name);
    QT_COMPAT_CONSTRUCTOR QTextDrag(QWidget * dragSource, const char * name);
#endif
    ~QTextDrag();

    virtual void setText(const QString &);
    virtual void setSubtype(const QString &);

    const char * format(int i) const;
    virtual QByteArray encodedData(const char*) const;

    static bool canDecode(const QMimeSource* e);
    static bool decode(const QMimeSource* e, QString& s);
    static bool decode(const QMimeSource* e, QString& s, QString& subtype);

protected:
    QTextDrag(QTextDragPrivate &, QWidget * dragSource = 0);

private:
    Q_DISABLE_COPY(QTextDrag)
};

class Q_GUI_EXPORT QImageDrag: public QDragObject {
    Q_OBJECT
    Q_DECLARE_PRIVATE(QImageDrag)
public:
    QImageDrag(QImage image, QWidget * dragSource = 0);
    QImageDrag(QWidget * dragSource = 0);
#ifdef QT_COMPAT
    QT_COMPAT_CONSTRUCTOR QImageDrag(QImage image, QWidget * dragSource, const char * name);
    QT_COMPAT_CONSTRUCTOR QImageDrag(QWidget * dragSource, const char * name);
#endif
    ~QImageDrag();

    virtual void setImage(QImage image);

    const char * format(int i) const;
    virtual QByteArray encodedData(const char*) const;

    static bool canDecode(const QMimeSource* e);
    static bool decode(const QMimeSource* e, QImage& i);
    static bool decode(const QMimeSource* e, QPixmap& i);

protected:
    QImageDrag(QImageDragPrivate &, QWidget * dragSource = 0);

private:
    Q_DISABLE_COPY(QImageDrag)
};


class Q_GUI_EXPORT QUriDrag: public QStoredDrag {
    Q_OBJECT

public:
    QUriDrag(const QList<QByteArray> &uris, QWidget * dragSource = 0);
    QUriDrag(QWidget * dragSource = 0);
#ifdef QT_COMPAT
    QT_COMPAT_CONSTRUCTOR QUriDrag(const QList<QByteArray> &uris, QWidget * dragSource,
                                   const char * name);
    QT_COMPAT_CONSTRUCTOR QUriDrag(QWidget * dragSource, const char * name);
#endif
    ~QUriDrag();

    void setFilenames(const QStringList & fnames) { setFileNames(fnames); }
    void setFileNames(const QStringList & fnames);
    void setUnicodeUris(const QStringList & uuris);
    virtual void setUris(const QList<QByteArray> &uris);

    static QString uriToLocalFile(const char*);
    static QByteArray localFileToUri(const QString&);
    static QString uriToUnicodeUri(const char*);
    static QByteArray unicodeUriToUri(const QString&);
    static bool canDecode(const QMimeSource* e);
    static bool decode(const QMimeSource* e, QList<QByteArray>& i);
    static bool decodeToUnicodeUris(const QMimeSource* e, QStringList& i);
    static bool decodeLocalFiles(const QMimeSource* e, QStringList& i);

private:
    Q_DISABLE_COPY(QUriDrag)
};

class Q_GUI_EXPORT QColorDrag : public QStoredDrag
{
    Q_OBJECT
    QColor color;

public:
    QColorDrag(const QColor &col, QWidget *dragsource = 0);
    QColorDrag(QWidget * dragSource = 0);
#ifdef QT_COMPAT
    QT_COMPAT_CONSTRUCTOR QColorDrag(const QColor &col, QWidget *dragsource, const char *name);
    QT_COMPAT_CONSTRUCTOR QColorDrag(QWidget * dragSource, const char * name);
#endif
    void setColor(const QColor &col);

    static bool canDecode(QMimeSource *);
    static bool decode(QMimeSource *, QColor &col);

private:
    Q_DISABLE_COPY(QColorDrag)
};

#ifdef QT_COMPAT
typedef QUriDrag QUrlDrag;
#endif

#ifndef QT_NO_DRAGANDDROP

// QDragManager is not part of the public API.  It is defined in a
// header file simply so different .cpp files can implement different
// member functions.
//

class Q_GUI_EXPORT QDragManager: public QObject {
    Q_OBJECT

private:
    QDragManager();
    ~QDragManager();
    // only friend classes can use QDragManager.
    friend class QDragObject;
    friend class QDragMoveEvent;
    friend class QDropEvent;
    friend class QApplication;

    bool eventFilter(QObject *, QEvent *);
    void timerEvent(QTimerEvent*);

    bool drag(QDragObject *, QDragObject::DragMode);

    void cancel(bool deleteSource = true);
    void move(const QPoint &);
    void drop();
    void updatePixmap();

private:
    Q_DISABLE_COPY(QDragManager)

    QDragObject * object;
    void updateMode(Qt::KeyboardModifiers newstate);
    void updateCursor();

    QWidget * dragSource;
    QWidget * dropWidget;
    bool beingCancelled;
    bool restoreCursor;
    bool willDrop;

    QPixmap *pm_cursor;
    int n_cursor;
};

#endif

#endif // QT_NO_MIME

#endif // QDRAGOBJECT_H
