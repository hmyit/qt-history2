#ifndef QTEXTIMAGEHANDLER_P_H
#define QTEXTIMAGEHANDLER_P_H

#ifndef QT_H
#include <qobject.h>
#include <qabstracttextdocumentlayout.h>

#include "qtextglobal_p.h"
#endif // QT_H

class QTextImageFormat;

class QTextImageHandler : public QObject,
                          public QTextObjectInterface
{
    Q_OBJECT
public:
    QTextImageHandler(QObject *parent = 0);

    virtual QSize intrinsicSize(QTextObject item, const QTextFormat &format);
    virtual void drawObject(QPainter *p, const QRect &rect, QTextObject item, const QTextFormat &format, QTextLayout::SelectionType selType);
};

#endif // QTEXTIMAGEHANDLER_P_H
