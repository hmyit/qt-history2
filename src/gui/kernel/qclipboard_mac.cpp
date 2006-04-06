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

#include "qclipboard.h"
#include "qapplication.h"
#include "qbitmap.h"
#include "qdatetime.h"
#include "qapplication_p.h"
#include <qdebug.h>
#include <private/qt_mac_p.h>
#include "qevent.h"
#include <stdlib.h>
#include <string.h>

/*****************************************************************************
  QClipboard debug facilities
 *****************************************************************************/
//#define DEBUG_PASTEBOARD

#ifndef QT_NO_CLIPBOARD

void qt_event_send_clipboard_changed(); //qapplication_mac.cpp

/*****************************************************************************
  QClipboard member functions for mac.
 *****************************************************************************/

static QMacPasteBoard *qt_mac_pasteboard = 0;
static void qt_mac_cleanupPasteboard() {
    delete qt_mac_pasteboard;
    qt_mac_pasteboard = 0;
}

static bool qt_mac_updateScrap()
{
    if(!qt_mac_pasteboard) {
        qt_mac_pasteboard = new QMacPasteBoard(kPasteboardClipboard, QMacMime::MIME_CLIP);
        qAddPostRoutine(qt_mac_cleanupPasteboard);
        return true;
    }
    return qt_mac_pasteboard->sync();
}

void QClipboard::clear(Mode mode)
{
    if(mode != Clipboard)
        return;
    qt_mac_updateScrap();
    qt_mac_pasteboard->clear();
}

void QClipboard::ownerDestroyed()
{
}

static int clipWatcherId = -1;

void QClipboard::connectNotify(const char *signal)
{
    if(qstrcmp(signal,SIGNAL(dataChanged())) == 0 && clipWatcherId == -1)
        clipWatcherId = startTimer(100);
}

bool QClipboard::event(QEvent *e)
{
    bool check_clip = false;
    if(e->type() == QEvent::Clipboard) {
        check_clip = true;
    } else if(clipWatcherId != -1  && e->type() == QEvent::Timer) {
        QTimerEvent *te = (QTimerEvent *)e;
        if(te->timerId() == clipWatcherId) {
            if(!receivers(SIGNAL(dataChanged()))) {
                killTimer(clipWatcherId);
                clipWatcherId = -1;
            } else {
                check_clip = true;
            }
        }
    }
    if(check_clip && qt_mac_updateScrap()) {
        qt_mac_pasteboard->setMimeData(0);
        emit dataChanged();
    }
    return QObject::event(e);
}

const QMimeData *QClipboard::mimeData(Mode mode) const
{
    if(mode != Clipboard)
        return 0;
    qt_mac_updateScrap();
    return qt_mac_pasteboard->mimeData();
}

void QClipboard::setMimeData(QMimeData *src, Mode mode)
{
    if(mode != Clipboard)
        return;
    qt_mac_updateScrap();
    qt_mac_pasteboard->setMimeData(src);
    emit dataChanged();
    qt_event_send_clipboard_changed();
}

bool QClipboard::supportsSelection() const
{
    return false; //nei takk
}

bool QClipboard::ownsSelection() const
{
    return false;
}

bool QClipboard::ownsClipboard() const
{
    qWarning("Qt: QClipboard::ownsClipboard: UNIMPLEMENTED!");
    return false;
}
#endif // QT_NO_CLIPBOARD

/*****************************************************************************
   QMacPasteBoard code
*****************************************************************************/

QMacPasteBoard::QMacPasteBoard(PasteboardRef p, uchar mt)
{
    mac_mime_source = false;
    mime_type = mt ? mt : uchar(QMacMime::MIME_ALL);
    paste = p;
    CFRetain(paste);
}

QMacPasteBoard::QMacPasteBoard(uchar mt)
{
    mac_mime_source = false;
    mime_type = mt ? mt : uchar(QMacMime::MIME_ALL);
    OSStatus err = PasteboardCreate(0, &paste);
    if(err == noErr) {
        PasteboardSetPromiseKeeper(paste, promiseKeeper, this);
    } else {
        qDebug("PasteBoard: Error creating pasteboard: [%d]", (int)err);
    }
}

QMacPasteBoard::QMacPasteBoard(CFStringRef name, uchar mt)
{
    mac_mime_source = false;
    mime_type = mt ? mt : uchar(QMacMime::MIME_ALL);
    OSStatus err = PasteboardCreate(name, &paste);
    if(err == noErr) {
        PasteboardSetPromiseKeeper(paste, promiseKeeper, this);
    } else {
        qDebug("PasteBoard: Error creating pasteboard: %s [%d]", QCFString::toQString(name).toLatin1().constData(), (int)err);
    }
}

QMacPasteBoard::~QMacPasteBoard()
{
    if(paste)
        CFRelease(paste);
}

PasteboardRef
QMacPasteBoard::pasteBoard() const
{
    return paste;
}

OSStatus QMacPasteBoard::promiseKeeper(PasteboardRef paste, PasteboardItemID id, CFStringRef f, void *data)
{
    QCFString flavor(f);
    QMacPasteBoard *qpaste = (QMacPasteBoard*) data;
    const int promise_id = (int)id;

    { //protect the marker!
        extern int qt_mac_mime_type; //qmime_mac.cpp
        extern CFStringRef qt_mac_mime_typeUTI; //qmime_mac.cpp
        if(promise_id == qt_mac_mime_type && QString(flavor) == QCFString(qt_mac_mime_typeUTI))
            return noErr;
    }

    if(promise_id < 0 || promise_id >= qpaste->promises.size()) {
        qDebug("Pasteboard: %d: Unexpected [%d]!", __LINE__, promise_id); //shouldn't happen
        return cantGetFlavorErr;
    }

    QMacPasteBoard::Promise promise = qpaste->promises[promise_id];
#ifdef DEBUG_PASTEBOARD
    qDebug("PasteBoard: Calling in promise for %s[%d] [%s] (%s)", qPrintable(promise.mime), promise_id, qPrintable(QString(flavor)),
           qPrintable(promise.convertor->convertorName()));
#endif
    if(!promise.convertor->canConvert(promise.mime, flavor)) {
        qDebug("Pasteboard: %d: Unexpected [%d]!", __LINE__, promise_id); //shouldn't happen
        return cantGetFlavorErr;
    }
    QList<QByteArray> md = promise.convertor->convertFromMime(promise.mime, promise.data, flavor);
    for(int i = 0; i < md.size(); ++i) {
        const QByteArray &ba = md[i];
        QCFType<CFDataRef> data = CFDataCreate(0, (UInt8*)ba.constData(), ba.size());
        PasteboardPutItemFlavor(paste, id, flavor, data, kPasteboardFlavorNoFlags);
    }
    return noErr;
}

bool
QMacPasteBoard::hasFlavor(QString c_flavor) const
{
    sync();

    UInt32 cnt = 0;
    if(PasteboardGetItemCount(paste, &cnt) || !cnt)
        return false;

#ifdef DEBUG_PASTEBOARD
    qDebug("PasteBoard: hasFlavor [%s]", qPrintable(c_flavor));
#endif
    for(uint index = 1; index <= cnt; ++index) {

        PasteboardItemID id;
        if(PasteboardGetItemIdentifier(paste, index, &id) != noErr)
            return false;

        QCFType<CFArrayRef> types;
        if(PasteboardCopyItemFlavors(paste, id, &types ) != noErr)
            return false;

        const int type_count = CFArrayGetCount(types);
        for(int i = 0; i < type_count; ++i) {
            QCFString flavor((CFStringRef)CFArrayGetValueAtIndex(types, i));
#ifdef DEBUG_PASTEBOARD
            qDebug("  - %s", qPrintable(QString(flavor)));
#endif
            if(c_flavor == flavor)
                return true;
        }
    }
#ifdef DEBUG_PASTEBOARD
    qDebug("  - NotFound!");
#endif
    return false;
}

class QMacPasteBoardMimeSource : public QMimeData {
    const QMacPasteBoard *paste;
public:
    QMacPasteBoardMimeSource(const QMacPasteBoard *p) : QMimeData(), paste(p) { }
    ~QMacPasteBoardMimeSource() { }
    virtual QStringList formats() const { return paste->formats(); }
    virtual QVariant retrieveData(const QString &format, QVariant::Type type) const { return paste->retrieveData(format, type); }
};

QMimeData
*QMacPasteBoard::mimeData() const
{
    if(!mime) {
        mac_mime_source = true;
        mime = new QMacPasteBoardMimeSource(this);

    }
    return mime;
}

class QMacMimeData : public QMimeData
{
public:
    QVariant variantData(const QString &mime) { return retrieveData(mime, QVariant::Invalid); }
private:
    QMacMimeData();
};

void
QMacPasteBoard::setMimeData(QMimeData *mime_src)
{
    if(mime == mime_src || (!mime_src && mime && mac_mime_source))
        return;
    mac_mime_source = false;
    delete mime;
    mime = 0;

    if((mime = mime_src)) {
        clear();
#ifdef DEBUG_PASTEBOARD
        qDebug("PasteBoard: setMimeData [%p]", mime_src);
#endif
        extern int qt_mac_mime_type; //qmime_mac.cpp
        extern CFStringRef qt_mac_mime_typeUTI; //qmime_mac.cpp
        PasteboardPutItemFlavor(paste, (PasteboardItemID)qt_mac_mime_type, qt_mac_mime_typeUTI, 0, kPasteboardFlavorNoFlags);

        QList<QMacMime*> all = QMacMime::all(mime_type);
        QStringList formats = mime_src->formats();
        for (int i = 0, count = 0; i < formats.size(); ++i) {
            QString mimeType = formats.at(i);
            for(QList<QMacMime *>::Iterator it = all.begin(); it != all.end(); ++it) {
                QMacMime *c = (*it);
                QString flavor(c->flavorUTIFor(mimeType));
                if(!flavor.isEmpty()) {
                    promises.append(QMacPasteBoard::Promise(c, mimeType,
                                                            static_cast<QMacMimeData*>(mime_src)->variantData(mimeType)));
#ifdef DEBUG_PASTEBOARD
                    qDebug(" -adding %s[%d] [%s] <%s>", qPrintable(mimeType), count, qPrintable(flavor),
                           qPrintable(c->convertorName()));
#endif
                    PasteboardPutItemFlavor(paste, (PasteboardItemID)count++, QCFString(flavor), 0, kPasteboardFlavorNoFlags);
                }
            }
        }
    }
}

QStringList
QMacPasteBoard::formats() const
{
    sync();

    QStringList ret;
    UInt32 cnt = 0;
    if(PasteboardGetItemCount(paste, &cnt) || !cnt)
        return ret;

    uchar qmt = mime_type;
    {
        extern CFStringRef qt_mac_mime_typeUTI; //qmime_mac.cpp
        if(hasFlavor(QCFString(qt_mac_mime_typeUTI)))
            qmt = QMacMime::MIME_QT_CONVERTOR;
    }

#ifdef DEBUG_PASTEBOARD
    qDebug("PasteBoard: Formats [%d]", (int)cnt);
#endif
    for(uint index = 1; index <= cnt; ++index) {

        PasteboardItemID id;
        if(PasteboardGetItemIdentifier(paste, index, &id) != noErr)
            continue;

        QCFType<CFArrayRef> types;
        if(PasteboardCopyItemFlavors(paste, id, &types ) != noErr)
            continue;

        const int type_count = CFArrayGetCount(types);
        for(int i = 0; i < type_count; ++i) {
            QCFString flavor((CFStringRef)CFArrayGetValueAtIndex(types, i));
#ifdef DEBUG_PASTEBOARD
                qDebug(" -%s", qPrintable(QString(flavor)));
#endif
            QString mimeType = QMacMime::flavorToMime(qmt, flavor);
            if(!mimeType.isEmpty()) {
#ifdef DEBUG_PASTEBOARD
                qDebug("   -<%d> %s [%s]", ret.size(), qPrintable(mimeType), qPrintable(QString(flavor)));
#endif
                ret << mimeType;
            }
        }
    }
    return ret;
}

bool
QMacPasteBoard::hasFormat(const QString &format) const
{
    sync();

    UInt32 cnt = 0;
    if(PasteboardGetItemCount(paste, &cnt) || !cnt)
        return false;

    uchar qmt = mime_type;
    {
        extern CFStringRef qt_mac_mime_typeUTI; //qmime_mac.cpp
        if(hasFlavor(QCFString(qt_mac_mime_typeUTI)))
            qmt = QMacMime::MIME_QT_CONVERTOR;
    }

#ifdef DEBUG_PASTEBOARD
    qDebug("PasteBoard: hasFormat [%s]", qPrintable(format));
#endif
    for(uint index = 1; index <= cnt; ++index) {

        PasteboardItemID id;
        if(PasteboardGetItemIdentifier(paste, index, &id) != noErr)
            continue;

        QCFType<CFArrayRef> types;
        if(PasteboardCopyItemFlavors(paste, id, &types ) != noErr)
            continue;

        const int type_count = CFArrayGetCount(types);
        for(int i = 0; i < type_count; ++i) {
            QCFString flavor((CFStringRef)CFArrayGetValueAtIndex(types, i));
#ifdef DEBUG_PASTEBOARD
                qDebug(" -%s", qPrintable(QString(flavor)));
#endif
            QString mimeType = QMacMime::flavorToMime(qmt, flavor);
#ifdef DEBUG_PASTEBOARD
            if(!mimeType.isEmpty())
                qDebug("   - %s", qPrintable(mimeType));
#endif
            if(mimeType == format)
                return true;
        }
    }
    return false;
}

QVariant
QMacPasteBoard::retrieveData(const QString &format, QVariant::Type) const
{
    sync();

    UInt32 cnt = 0;
    if(PasteboardGetItemCount(paste, &cnt) || !cnt)
        return QByteArray();

    uchar qmt = mime_type;
    {
        extern CFStringRef qt_mac_mime_typeUTI; //qmime_mac.cpp
        if(hasFlavor(QCFString(qt_mac_mime_typeUTI)))
            qmt = QMacMime::MIME_QT_CONVERTOR;
    }

#ifdef DEBUG_PASTEBOARD
    qDebug("Pasteboard: retrieveData [%s]", qPrintable(format));
#endif
    const QList<QMacMime *> mimes = QMacMime::all(qmt);
    for(int mime = 0; mime < mimes.size(); ++mime) {
        QMacMime *c = mimes.at(mime);
        QString c_flavor = c->flavorUTIFor(format);
        if(!c_flavor.isEmpty()) {
            // Handle text/plain a little differently. Try handling Unicode first.
            if(QCFString(c_flavor) == kUTTypeUTF8PlainText && hasFlavor(QCFString(kUTTypeUTF16PlainText)))
                c_flavor = QCFString(kUTTypeUTF16PlainText);

            for(uint index = 1; index <= cnt; ++index) {
                PasteboardItemID id;
                if(PasteboardGetItemIdentifier(paste, index, &id) != noErr)
                    continue;

                QCFType<CFArrayRef> types;
                if(PasteboardCopyItemFlavors(paste, id, &types ) != noErr)
                    continue;

                const int type_count = CFArrayGetCount(types);
                for(int i = 0; i < type_count; ++i) {
                    QCFString flavor((CFStringRef)CFArrayGetValueAtIndex(types, i));
                    if(c_flavor == flavor) {
                        QVariant ret;
                        QCFType<CFDataRef> macBuffer;
                        if(PasteboardCopyItemFlavorData(paste, id, flavor, &macBuffer) == noErr) {
                            QByteArray buffer((const char *)CFDataGetBytePtr(macBuffer), CFDataGetLength(macBuffer));
                            if(!buffer.isEmpty()) {
#ifdef DEBUG_PASTEBOARD
                                qDebug("  - %s [%s] (%s)", qPrintable(format), qPrintable(QString(flavor)), qPrintable(c->convertorName()));
#endif

                                QList<QByteArray> lst;
                                lst.append(buffer);
                                ret = c->convertToMime(format, lst, c_flavor);
                                ret.detach();
                                return ret;
                            }
                        }
                        return ret;
                    } else {
#ifdef DEBUG_PASTEBOARD
                        qDebug("  - NoMatch %s [%s] (%s)", qPrintable(c_flavor), qPrintable(QString(flavor)), qPrintable(c->convertorName()));
#endif
                    }
                }
            }


        }
    }
    return QByteArray();
}

void
QMacPasteBoard::clear()
{
#ifdef DEBUG_PASTEBOARD
    qDebug("PasteBoard: clear!");
#endif
    PasteboardClear(paste);
    promises.clear();
}

bool
QMacPasteBoard::sync() const
{
    const bool ret = PasteboardSynchronize(paste) & kPasteboardModified;
#ifdef DEBUG_PASTEBOARD
    if(ret)
        qDebug("Pasteboard: Syncronize!");
#endif
    return ret;
}



