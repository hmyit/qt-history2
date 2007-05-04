/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qscreenvnc_qws.h"

#ifndef QT_NO_QWS_VNC

#include "qscreenvnc_p.h"
#include "qwindowsystem_qws.h"
#include "qwsdisplay_qws.h"
#include "qscreendriverfactory_qws.h"
#include <QtCore/qtimer.h>
#include <QtCore/qregexp.h>
#include <QtGui/qwidget.h>
#include <QtGui/qpolygon.h>
#include <QtGui/qpainter.h>
#include <qdebug.h>
#include <private/qwindowsurface_qws_p.h>
#include <private/qwssignalhandler_p.h>
#include <private/qwidget_p.h>

#include <stdlib.h>

//#define QT_QWS_VNC_DEBUG

extern QString qws_qtePipeFilename();

#ifndef QT_NO_QWS_CURSOR

inline void QVNCCursor::setDirty(const QRect &r) const
{
    screen->d_ptr->setDirty(r, true);
}

void QVNCCursor::hide()
{
    QScreenCursor::hide();
    if (enable)
        setDirty(boundingRect());
}

void QVNCCursor::show()
{
    QScreenCursor::show();
    if (!enable)
        setDirty(boundingRect());
}

void QVNCCursor::set(const QImage &image, int hotx, int hoty)
{
    QRegion dirty = boundingRect();
    QScreenCursor::set(image, hotx, hoty);
    dirty |= boundingRect();
    if (enable) {
        const QVector<QRect> rects = dirty.rects();
        for (int i = 0; i < rects.size(); ++i)
            setDirty(rects.at(i));
    }
}

void QVNCCursor::move(int x, int y)
{
    QRegion dirty = boundingRect();
    QScreenCursor::move(x, y);
    dirty |= boundingRect();
    if (enable) {
        const QVector<QRect> rects = dirty.rects();
        for (int i = 0; i < rects.size(); ++i)
            setDirty(rects.at(i));
    }
}

#endif // QT_NO_QWS_CURSOR

QVNCScreenPrivate::QVNCScreenPrivate(QVNCScreen *parent)
    : dpiX(72), dpiY(72), doOnScreenSurface(false), refreshRate(25),
      vncServer(0), subscreen(0), q_ptr(parent)
{
#ifndef QT_NO_QWS_MULTIPROCESS
    shm = 0;
#endif
#ifndef QT_NO_QWS_SIGNALHANDLER
    QWSSignalHandler::instance()->addObject(this);
#endif
}

QVNCScreenPrivate::~QVNCScreenPrivate()
{
#ifndef QT_NO_QWS_MULTIPROCESS
    if (shm) {
        if (QApplication::type() == QApplication::GuiServer)
            shm->destroy();
        delete shm;
    }
#else
    if (!subscreen) {
        delete[] q_ptr->data;
        q_ptr->data = 0;
    }
#endif
    delete subscreen;
}

void QVNCScreenPrivate::configure()
{
    if (subscreen) {
        q_ptr->d = subscreen->depth();
        q_ptr->w = subscreen->width();
        q_ptr->h = subscreen->height();
        q_ptr->dw = subscreen->deviceWidth();
        q_ptr->dh = subscreen->deviceHeight();
        q_ptr->lstep = subscreen->linestep();
        q_ptr->data = subscreen->base();
        q_ptr->physWidth = subscreen->physicalWidth();
        q_ptr->physHeight = subscreen->physicalHeight();

        q_ptr->setPixelFormat(subscreen->pixelFormat());
        q_ptr->setOffset(subscreen->offset());
    } else {
        q_ptr->lstep = (q_ptr->dw * q_ptr->d + 7) / 8;
        q_ptr->size = q_ptr->h * q_ptr->lstep;
        q_ptr->mapsize = q_ptr->size;
        q_ptr->physWidth = qRound(q_ptr->dw * 25.4 / dpiX);
        q_ptr->physHeight = qRound(q_ptr->dh * 25.4 / dpiY);

        switch (q_ptr->d) {
        case 1:
            q_ptr->setPixelFormat(QImage::Format_Mono); //### LSB???
            break;
        case 8:
            q_ptr->setPixelFormat(QImage::Format_Indexed8);
            break;
        case 16:
            q_ptr->setPixelFormat(QImage::Format_RGB16);
            break;
        case 32:
            q_ptr->setPixelFormat(QImage::Format_ARGB32_Premultiplied);
            break;
        }

#ifndef QT_NO_QWS_MULTIPROCESS
        if (shm) {
            if (QApplication::type() == QApplication::GuiServer)
                shm->destroy();
            delete shm;
        }
        shm = new QSharedMemory(q_ptr->size, qws_qtePipeFilename(),
                                q_ptr->displayId);
        if (!shm->create())
            qDebug("QVNCScreen could not create shared memory");
        if (!shm->attach())
            qDebug("QVNCScreen could not attach to shared memory");
        q_ptr->data = reinterpret_cast<uchar*>(shm->base());
#else
        if (q_ptr->data)
            delete[] q_ptr->data;
        q_ptr->data = new uchar[q_ptr->size];
#endif
    }

}

//===========================================================================

static const struct {
    int keysym;
    int keycode;
} keyMap[] = {
    { 0xff08, Qt::Key_Backspace },
    { 0xff09, Qt::Key_Tab       },
    { 0xff0d, Qt::Key_Return    },
    { 0xff1b, Qt::Key_Escape    },
    { 0xff63, Qt::Key_Insert    },
    { 0xffff, Qt::Key_Delete    },
    { 0xff50, Qt::Key_Home      },
    { 0xff57, Qt::Key_End       },
    { 0xff55, Qt::Key_PageUp    },
    { 0xff56, Qt::Key_PageDown  },
    { 0xff51, Qt::Key_Left      },
    { 0xff52, Qt::Key_Up        },
    { 0xff53, Qt::Key_Right     },
    { 0xff54, Qt::Key_Down      },
    { 0xffbe, Qt::Key_F1        },
    { 0xffbf, Qt::Key_F2        },
    { 0xffc0, Qt::Key_F3        },
    { 0xffc1, Qt::Key_F4        },
    { 0xffc2, Qt::Key_F5        },
    { 0xffc3, Qt::Key_F6        },
    { 0xffc4, Qt::Key_F7        },
    { 0xffc5, Qt::Key_F8        },
    { 0xffc6, Qt::Key_F9        },
    { 0xffc7, Qt::Key_F10       },
    { 0xffc8, Qt::Key_F11       },
    { 0xffc9, Qt::Key_F12       },
    { 0xffe1, Qt::Key_Shift     },
    { 0xffe2, Qt::Key_Shift     },
    { 0xffe3, Qt::Key_Control   },
    { 0xffe4, Qt::Key_Control   },
    { 0xffe7, Qt::Key_Meta      },
    { 0xffe8, Qt::Key_Meta      },
    { 0xffe9, Qt::Key_Alt       },
    { 0xffea, Qt::Key_Alt       },
    { 0, 0 }
};

void QRfbRect::read(QTcpSocket *s)
{
    quint16 buf[4];
    s->read((char*)buf, 8);
    x = ntohs(buf[0]);
    y = ntohs(buf[1]);
    w = ntohs(buf[2]);
    h = ntohs(buf[3]);
}

void QRfbRect::write(QTcpSocket *s) const
{
    quint16 buf[4];
    buf[0] = htons(x);
    buf[1] = htons(y);
    buf[2] = htons(w);
    buf[3] = htons(h);
    s->write((char*)buf, 8);
}

void QRfbPixelFormat::read(QTcpSocket *s)
{
    char buf[16];
    s->read(buf, 16);
    bitsPerPixel = buf[0];
    depth = buf[1];
    bigEndian = buf[2];
    trueColor = buf[3];

    quint16 a = ntohs(*(quint16 *)(buf + 4));
    redBits = 0;
    while (a) { a >>= 1; redBits++; }

    a = ntohs(*(quint16 *)(buf + 6));
    greenBits = 0;
    while (a) { a >>= 1; greenBits++; }

    a = ntohs(*(quint16 *)(buf + 8));
    blueBits = 0;
    while (a) { a >>= 1; blueBits++; }

    redShift = buf[10];
    greenShift = buf[11];
    blueShift = buf[12];
}

void QRfbPixelFormat::write(QTcpSocket *s)
{
    char buf[16];
    buf[0] = bitsPerPixel;
    buf[1] = depth;
    buf[2] = bigEndian;
    buf[3] = trueColor;

    quint16 a = 0;
    for (int i = 0; i < redBits; i++) a = (a << 1) | 1;
    *(quint16 *)(buf + 4) = htons(a);

    a = 0;
    for (int i = 0; i < greenBits; i++) a = (a << 1) | 1;
    *(quint16 *)(buf + 6) = htons(a);

    a = 0;
    for (int i = 0; i < blueBits; i++) a = (a << 1) | 1;
    *(quint16 *)(buf + 8) = htons(a);

    buf[10] = redShift;
    buf[11] = greenShift;
    buf[12] = blueShift;
    s->write(buf, 16);
}


void QRfbServerInit::setName(const char *n)
{
    delete[] name;
    name = new char [strlen(n) + 1];
    strcpy(name, n);
}

void QRfbServerInit::read(QTcpSocket *s)
{
    s->read((char *)&width, 2);
    width = ntohs(width);
    s->read((char *)&height, 2);
    height = ntohs(height);
    format.read(s);

    quint32 len;
    s->read((char *)&len, 4);
    len = ntohl(len);

    name = new char [len + 1];
    s->read(name, len);
    name[len] = '\0';
}

void QRfbServerInit::write(QTcpSocket *s)
{
    quint16 t = htons(width);
    s->write((char *)&t, 2);
    t = htons(height);
    s->write((char *)&t, 2);
    format.write(s);
    quint32 len = strlen(name);
    len = htonl(len);
    s->write((char *)&len, 4);
    s->write(name, strlen(name));
}

bool QRfbSetEncodings::read(QTcpSocket *s)
{
    if (s->bytesAvailable() < 3)
        return false;

    char tmp;
    s->read(&tmp, 1);        // padding
    s->read((char *)&count, 2);
    count = ntohs(count);

    return true;
}

bool QRfbFrameBufferUpdateRequest::read(QTcpSocket *s)
{
    if (s->bytesAvailable() < 9)
        return false;

    s->read(&incremental, 1);
    rect.read(s);

    return true;
}

bool QRfbKeyEvent::read(QTcpSocket *s)
{
    if (s->bytesAvailable() < 7)
        return false;

    s->read(&down, 1);
    quint16 tmp;
    s->read((char *)&tmp, 2);  // padding

    quint32 key;
    s->read((char *)&key, 4);
    key = ntohl(key);

    unicode = 0;
    keycode = 0;
    int i = 0;
    while (keyMap[i].keysym && !keycode) {
        if (keyMap[i].keysym == (int)key)
            keycode = keyMap[i].keycode;
        i++;
    }
    if (!keycode) {
        if (key <= 0xff) {
            unicode = key;
            if (key >= 'a' && key <= 'z')
                keycode = Qt::Key_A + key - 'a';
            else if (key >= ' ' && key <= '~')
                keycode = Qt::Key_Space + key - ' ';
        }
    }

    return true;
}

bool QRfbPointerEvent::read(QTcpSocket *s)
{
    if (s->bytesAvailable() < 5)
        return false;

    char buttonMask;
    s->read(&buttonMask, 1);
    buttons = 0;
    if (buttonMask & 1)
        buttons |= Qt::LeftButton;
    if (buttonMask & 2)
        buttons |= Qt::MidButton;
    if (buttonMask & 4)
        buttons |= Qt::RightButton;

    quint16 tmp;
    s->read((char *)&tmp, 2);
    x = ntohs(tmp);
    s->read((char *)&tmp, 2);
    y = ntohs(tmp);

    return true;
}

bool QRfbClientCutText::read(QTcpSocket *s)
{
    if (s->bytesAvailable() < 7)
        return false;

    char tmp[3];
    s->read(tmp, 3);        // padding
    s->read((char *)&length, 4);
    length = ntohl(length);

    return true;
}

//===========================================================================

QVNCServer::QVNCServer(QVNCScreen *screen)
    : qvnc_screen(screen)
{
    init(5900);
}

QVNCServer::QVNCServer(QVNCScreen *screen, int id)
    : qvnc_screen(screen)
{
    init(5900 + id);
}

void QVNCServer::init(uint port)
{
    handleMsg = false;
    client = 0;
    encodingsPending = 0;
    cutTextPending = 0;
    keymod = 0;
    state = Unconnected;

    refreshRate = 25;
    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));

    serverSocket = new QTcpServer(this);
    if (!serverSocket->listen(QHostAddress::Any, port))
        qDebug() << "QVNCServer could not connect:" << serverSocket->errorString();
    else
        qDebug("QVNCServer created on port %d", port);

    connect(serverSocket, SIGNAL(newConnection()), this, SLOT(newConnection()));

    encoder = 0;
}

QVNCServer::~QVNCServer()
{
    delete encoder;
    encoder = 0;
    delete client;
    client = 0;
}

void QVNCServer::setDirty()
{
    if (state == Connected && !timer->isActive() && dirtyMap()->numDirty > 0)
        timer->start();
}

void QVNCServer::newConnection()
{
    if (client)
        delete client;

    client = serverSocket->nextPendingConnection();
    connect(client,SIGNAL(readyRead()),this,SLOT(readClient()));
    connect(client,SIGNAL(disconnected()),this,SLOT(discardClient()));
    handleMsg = false;
    encodingsPending = 0;
    cutTextPending = 0;
    supportHextile = false;
    wantUpdate = false;

    timer->start(1000 / refreshRate);
    dirtyMap()->reset();

    // send protocol version
    char *proto = "RFB 003.003\n";
    client->write(proto, 12);
    state = Protocol;

    if (!qvnc_screen->d_ptr->subscreen)
        QWSServer::instance()->enablePainting(true);
}

void QVNCServer::readClient()
{
    switch (state) {
        case Protocol:
            if (client->bytesAvailable() >= 12) {
                char proto[13];
                client->read(proto, 12);
                proto[12] = '\0';
                qDebug("Client protocol version %s", proto);
                // No authentication
                quint32 auth = htonl(1);
                client->write((char *) &auth, sizeof(auth));
                state = Init;
            }
            break;

        case Init:
            if (client->bytesAvailable() >= 1) {
                quint8 shared;
                client->read((char *) &shared, 1);

                // Server Init msg
                QRfbServerInit sim;
                QRfbPixelFormat &format = sim.format;
                switch (qvnc_screen->depth()) {
                case 32:
                    format.bitsPerPixel = 32;
                    format.depth = 32;
                    format.bigEndian = 0;
                    format.trueColor = true;
                    format.redBits = 8;
                    format.greenBits = 8;
                    format.blueBits = 8;
                    format.redShift = 16;
                    format.greenShift = 8;
                    format.blueShift = 0;
                    break;

                case 16:
                    format.bitsPerPixel = 16;
                    format.depth = 16;
                    format.bigEndian = 0;
                    format.trueColor = true;
                    format.redBits = 5;
                    format.greenBits = 6;
                    format.blueBits = 5;
                    format.redShift = 11;
                    format.greenShift = 5;
                    format.blueShift = 0;
                    break;


                case 8:
                case 4:
                    format.bitsPerPixel = 8;
                    format.depth = 8;
                    format.bigEndian = 0;
                    format.trueColor = false;
                    format.redBits = 0;
                    format.greenBits = 0;
                    format.blueBits = 0;
                    format.redShift = 0;
                    format.greenShift = 0;
                    format.blueShift = 0;
                    break;

                default:
                    qDebug("QVNC cannot drive depth %d", qvnc_screen->depth());
                    discardClient();
                    return;
                }
                sim.width = qvnc_screen->deviceWidth();
                sim.height = qvnc_screen->deviceHeight();
                sim.setName("Qtopia Core VNC Server");
                sim.write(client);
                state = Connected;
            }
            break;

        case Connected:
            do {
                if (!handleMsg) {
                    client->read((char *)&msgType, 1);
                    handleMsg = true;
                }
                if (handleMsg) {
                    switch (msgType ) {
                    case SetPixelFormat:
                        setPixelFormat();
                        break;
                    case FixColourMapEntries:
                        qDebug("Not supported: FixColourMapEntries");
                        handleMsg = false;
                        break;
                    case SetEncodings:
                        setEncodings();
                        break;
                    case FramebufferUpdateRequest:
                        frameBufferUpdateRequest();
                        break;
                    case KeyEvent:
                        keyEvent();
                        break;
                    case PointerEvent:
                        pointerEvent();
                        break;
                    case ClientCutText:
                        clientCutText();
                        break;
                    default:
                        qDebug("Unknown message type: %d", (int)msgType);
                        handleMsg = false;
                    }
                }
            } while (!handleMsg && client->bytesAvailable());
            break;
    default:
        break;
    }
}

void QVNCServer::setPixelFormat()
{
    if (client->bytesAvailable() >= 19) {
        char buf[3];
        client->read(buf, 3); // just padding
        pixelFormat.read(client);
#ifdef QT_QWS_VNC_DEBUG
        qDebug("Want format: %d %d %d %d %d %d %d %d %d %d",
            int(pixelFormat.bitsPerPixel),
            int(pixelFormat.depth),
            int(pixelFormat.bigEndian),
            int(pixelFormat.trueColor),
            int(pixelFormat.redBits),
            int(pixelFormat.greenBits),
            int(pixelFormat.blueBits),
            int(pixelFormat.redShift),
            int(pixelFormat.greenShift),
            int(pixelFormat.blueShift));
#endif
        if (!pixelFormat.trueColor) {
            qDebug("Can only handle true color clients");
            discardClient();
        }
        handleMsg = false;
        sameEndian = (QSysInfo::ByteOrder == QSysInfo::BigEndian) == !!pixelFormat.bigEndian;
        needConversion = pixelConversionNeeded();
    }
}

void QVNCServer::setEncodings()
{
    QRfbSetEncodings enc;

    if (!encodingsPending && enc.read(client)) {
        encodingsPending = enc.count;
        if (!encodingsPending)
            handleMsg = false;
    }

    if (encoder) {
        delete encoder;
        encoder = 0;
    }

    enum Encodings {
        Raw = 0,
        CopyRect = 1,
        RRE = 2,
        CoRRE = 4,
        Hextile = 5,
        ZRLE = 16,
        Cursor = -239,
        DesktopSize = -223
    };

    if (encodingsPending && (unsigned)client->bytesAvailable() >=
                                encodingsPending * sizeof(quint32)) {
        for (int i = 0; i < encodingsPending; ++i) {
            qint32 enc;
            client->read((char *)&enc, sizeof(qint32));
            enc = ntohl(enc);
#ifdef QT_QWS_VNC_DEBUG
            qDebug("QVNCServer::setEncodings: %d", enc);
#endif
            switch (enc) {
            case Raw:
                if (!encoder) {
                    encoder = new QRfbRawEncoder(this);
#ifdef QT_QWS_VNC_DEBUG
                    qDebug("QVNCServer::setEncodings: using raw");
#endif
                }
               break;
            case CopyRect:
                supportCopyRect = true;
                break;
            case RRE:
                supportRRE = true;
                break;
            case CoRRE:
                supportCoRRE = true;
                break;
            case Hextile:
                supportHextile = true;
                if (encoder)
                    break;
                switch (qvnc_screen->depth()) {
#ifdef QT_QWS_DEPTH_8
                case 8:
                    encoder = new QRfbHextileEncoder<quint8>(this);
                    break;
#endif
#ifdef QT_QWS_DEPTH_16
                case 16:
                    encoder = new QRfbHextileEncoder<quint16>(this);
                    break;
#endif
#ifdef QT_QWS_DEPTH_32
                case 32:
                    encoder = new QRfbHextileEncoder<quint32>(this);
                    break;
#endif
                default:
                    break;
                }
#ifdef QT_QWS_VNC_DEBUG
                qDebug("QVNCServer::setEncodings: using hextile");
#endif
                break;
            case ZRLE:
                supportZRLE = true;
                break;
            case Cursor:
                supportCursor = true;
                break;
            case DesktopSize:
                supportDesktopSize = true;
                break;
            default:
                break;
            }
        }
        handleMsg = false;
        encodingsPending = 0;
    }

    if (!encoder) {
        encoder = new QRfbRawEncoder(this);
#ifdef QT_QWS_VNC_DEBUG
        qDebug("QVNCServer::setEncodings: fallback using raw");
#endif
    }
}

void QVNCServer::frameBufferUpdateRequest()
{
    QRfbFrameBufferUpdateRequest ev;

    if (ev.read(client)) {
        if (!ev.incremental) {
            QRect r(ev.rect.x, ev.rect.y, ev.rect.w, ev.rect.h);
            r.translated(qvnc_screen->offset());
            qvnc_screen->d_ptr->setDirty(r, true);
        }
        wantUpdate = true;
        checkUpdate();
        handleMsg = false;
    }
}

void QVNCServer::pointerEvent()
{
    QRfbPointerEvent ev;
    if (ev.read(client)) {
        const QPoint offset = qvnc_screen->offset();
        QWSServer::sendMouseEvent(offset + QPoint(ev.x, ev.y), ev.buttons);
        handleMsg = false;
    }
}

void QVNCServer::keyEvent()
{
    QRfbKeyEvent ev;

    if (ev.read(client)) {
        if (ev.keycode == Qt::Key_Shift)
            keymod = ev.down ? keymod | Qt::ShiftModifier :
                               keymod & ~Qt::ShiftModifier;
        else if (ev.keycode == Qt::Key_Control)
            keymod = ev.down ? keymod | Qt::ControlModifier :
                               keymod & ~Qt::ControlModifier;
        else if (ev.keycode == Qt::Key_Alt)
            keymod = ev.down ? keymod | Qt::AltModifier :
                               keymod & ~Qt::AltModifier;
        if (ev.unicode || ev.keycode)
            QWSServer::sendKeyEvent(ev.unicode, ev.keycode, keymod, ev.down, false);
        handleMsg = false;
    }
}

void QVNCServer::clientCutText()
{
    QRfbClientCutText ev;

    if (ev.read(client)) {
        cutTextPending = ev.length;
        if (!cutTextPending)
            handleMsg = false;
    }

    if (cutTextPending && client->bytesAvailable() >= cutTextPending) {
        char *text = new char [cutTextPending+1];
        client->read(text, cutTextPending);
        delete [] text;
        cutTextPending = 0;
        handleMsg = false;
    }
}

// stride in bytes
template <class SRC>
bool QRfbSingleColorHextile<SRC>::read(const uchar *data,
                                       int width, int height, int stride)
{
    const int depth = encoder->server->screen()->depth();
    if (width % (depth / 8)) // hw: should rather fallback to simple loop
        return false;

    static int alwaysFalse = qgetenv("QT_VNC_NOCHECKFILL").toInt();
    if (alwaysFalse)
        return false;

    switch (depth) {
    case 4: {
        const quint8 *data8 = reinterpret_cast<const quint8*>(data);
        if ((data8[0] & 0xf) != (data8[0] >> 4))
            return false;
        width /= 2;
    } // fallthrough
    case 8: {
        const quint8 *data8 = reinterpret_cast<const quint8*>(data);
        if (data8[0] != data8[1])
            return false;
        width /= 2;
    } // fallthrough
    case 16: {
        const quint16 *data16 = reinterpret_cast<const quint16*>(data);
        if (data16[0] != data16[1])
            return false;
        width /= 2;
    } // fallthrough
    case 32: {
        const quint32 *data32 = reinterpret_cast<const quint32*>(data);
        const quint32 first = data32[0];
        const int linestep = (stride / sizeof(quint32)) - width;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (*(data32++) != first)
                    return false;
            }
            data32 += linestep;
        }
        break;
    }
    default:
        return false;
    }

    SRC color = reinterpret_cast<const SRC*>(data)[0];
    encoder->newBg |= (color != encoder->bg);
    encoder->bg = color;
    return true;
}

template <class SRC>
void QRfbSingleColorHextile<SRC>::write(QTcpSocket *socket) const
{
    if (true || encoder->newBg) {
        const int bpp = encoder->server->clientBytesPerPixel();
        const int padding = 3;
        QVarLengthArray<char> buffer(padding + 1 + bpp);
        buffer[padding] = 2; // BackgroundSpecified
        encoder->server->convertPixels(buffer.data() + padding + 1,
                                       reinterpret_cast<char*>(&encoder->bg),
                                       1);
        socket->write(buffer.data() + padding, bpp + 1);
//        encoder->newBg = false;
    } else {
        char subenc = 0;
        socket->write(&subenc, 1);
    }
}

template <class SRC>
bool QRfbDualColorHextile<SRC>::read(const uchar *data,
                                     int width, int height, int stride)
{
    const SRC *ptr = reinterpret_cast<const SRC*>(data);
    const int linestep = (stride / sizeof(SRC)) - width;

    SRC c1;
    SRC c2 = 0;
    int n1 = 0;
    int n2 = 0;
    int x = 0;
    int y = 0;

    c1 = *ptr;

    // find second color
    while (y < height) {
        while (x < width) {
            if (*ptr == c1) {
                ++n1;
            } else {
                c2 = *ptr;
                goto found_second_color;
            }
            ++ptr;
            ++x;
        }
        x = 0;
        ptr += linestep;
        ++y;
    }

found_second_color:
    // finish counting
    while (y < height) {
        while (x < width) {
            if (*ptr == c1) {
                ++n1;
            } else if (*ptr == c2) {
                ++n2;
            } else {
                return false;
            }
            ++ptr;
            ++x;
        }
        x = 0;
        ptr += linestep;
        ++y;
    }

    if (n2 > n1) {
        const quint32 tmpC = c1;
        c1 = c2;
        c2 = tmpC;
    }

    encoder->newBg |= (c1 != encoder->bg);
    encoder->newFg |= (c2 != encoder->fg);

    encoder->bg = c1;
    encoder->fg = c2;

    // create map
    bool inRect = false;
    numRects = 0;
    ptr = reinterpret_cast<const SRC*>(data);
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            if (inRect && *ptr == encoder->bg) {
                // rect finished
                setWidth(x - lastx());
                next();
                inRect = false;
            } else if (!inRect && *ptr == encoder->fg) {
                // rect start
                setX(x);
                setY(y);
                setHeight(1);
                inRect = true;
            }
            ++ptr;
        }
        if (inRect) {
            // finish rect
            setWidth(width - lastx());
            next();
            inRect = false;
        }
        ptr += linestep;
    }

    return true;
}

template <class SRC>
void QRfbDualColorHextile<SRC>::write(QTcpSocket *socket) const
{
    const int bpp = encoder->server->clientBytesPerPixel();
    const int padding = 3;
    QVarLengthArray<char> buffer(padding + 2 * bpp + sizeof(char) + sizeof(numRects));
    char &subenc = buffer[padding];
    int n = padding + sizeof(subenc);

    subenc = 0x8; // AnySubrects

    if (encoder->newBg) {
        subenc |= 0x2; // Background
        encoder->server->convertPixels(buffer.data() + n, (char*)&encoder->bg, 1);
        n += bpp;
//        encoder->newBg = false;
    }

    if (encoder->newFg) {
        subenc |= 0x4; // Foreground
        encoder->server->convertPixels(buffer.data() + n, (char*)&encoder->fg, 1);
        n += bpp;
//        encoder->newFg = false;
    }
    buffer[n] = numRects;
    n += sizeof(numRects);

    socket->write(buffer.data() + padding, n - padding);
    socket->write((char*)rects, numRects * sizeof(Rect));
}

template <class SRC>
void QRfbDualColorHextile<SRC>::next()
{
    for (int r = numRects - 1; r >= 0; --r) {
        if (recty(r) == lasty())
            continue;
        if (recty(r) < lasty() - 1) // only search previous scanline
            break;
        if (rectx(r) == lastx() && width(r) == width(numRects)) {
            ++rects[r].wh;
            return;
        }
    }
    ++numRects;
}

template <class SRC>
inline void QRfbMultiColorHextile<SRC>::setColor(SRC color)
{
    encoder->server->convertPixels(reinterpret_cast<char*>(rect(numRects)),
                                   (const char*)&color, 1);
}

template <class SRC>
inline bool QRfbMultiColorHextile<SRC>::beginRect()
{
    if ((rects.size() + bpp + 2) > maxRectsSize)
        return false;
    rects.resize(rects.size() + bpp + 2);
    return true;
}

template <class SRC>
inline void QRfbMultiColorHextile<SRC>::endRect()
{
    setHeight(numRects, 1);
    ++numRects;
}

template <class SRC>
bool QRfbMultiColorHextile<SRC>::read(const uchar *data,
                                      int width, int height, int stride)
{
    const SRC *ptr = reinterpret_cast<const SRC*>(data);
    const int linestep = (stride / sizeof(SRC)) - width;

    bpp = encoder->server->clientBytesPerPixel();

    if (encoder->newBg)
        encoder->bg = ptr[0];

    const SRC bg = encoder->bg;
    SRC color = bg;
    bool inRect = false;

    numRects = 0;
    rects.clear();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (inRect && *ptr != color) { // end rect
                setWidth(numRects, x - rectx(numRects));
                endRect();
                inRect = false;
            }

            if (!inRect && *ptr != bg) { // begin rect
                if (!beginRect())
                    return false;
                inRect = true;
                color = *ptr;
                setColor(color);
                setX(numRects, x);
                setY(numRects, y);
            }
            ++ptr;
        }
        if (inRect) { // end rect
            setWidth(numRects, width - rectx(numRects));
            endRect();
            inRect = false;
        }
        ptr += linestep;
    }

    return true;
}

template <class SRC>
void QRfbMultiColorHextile<SRC>::write(QTcpSocket *socket) const
{
    const int padding = 3;
    QVarLengthArray<quint8> buffer(bpp + padding + sizeof(quint8) + sizeof(numRects));

    quint8 &subenc = buffer[padding];
    int n = padding + sizeof(quint8);

    subenc = 8 | 16; // AnySubrects | SubrectsColoured

    if (encoder->newBg) {
        subenc |= 0x2; // Background
        encoder->server->convertPixels(reinterpret_cast<char*>(buffer.data() + n),
                                       reinterpret_cast<const char*>(&encoder->bg),
                                       1);
        n += bpp;
//        encoder->newBg = false;
    }

    buffer[n] = numRects;
    n += sizeof(numRects);

    socket->write(reinterpret_cast<const char*>(buffer.data() + padding),
                  n - padding);
    socket->write(reinterpret_cast<const char*>(rects.constData()),
                  rects.size());
}

bool QVNCServer::pixelConversionNeeded() const
{
    if (!sameEndian)
        return true;

    const int screendepth = qvnc_screen->depth();
    if (screendepth != pixelFormat.bitsPerPixel)
        return true;

    switch (screendepth) {
    case 32:
        return false;
    case 16:
        return (pixelFormat.redBits == 5
                && pixelFormat.greenBits == 6
                && pixelFormat.blueBits == 5);
    }
    return true;
}

// count: number of pixels
void QVNCServer::convertPixels(char *dst, const char *src, int count) const
{
    const int screendepth = qvnc_screen->depth();

    // cutoffs
    if (sameEndian) {
        if (screendepth == pixelFormat.bitsPerPixel) { // memcpy cutoffs

            switch (screendepth) {
            case 32:
                memcpy(dst, src, count * sizeof(quint32));
                return;
            case 16:
                if (pixelFormat.redBits == 5
                    && pixelFormat.greenBits == 6
                    && pixelFormat.blueBits == 5)
                {
                    memcpy(dst, src, count * sizeof(quint16));
                    return;
                }
            }
        } else if (screendepth == 16 && pixelFormat.bitsPerPixel == 32) {
#if defined(__i386__) // Currently fails on ARM if dst is not 4 byte aligned
            const quint32 *src32 = reinterpret_cast<const quint32*>(src);
            quint32 *dst32 = reinterpret_cast<quint32*>(dst);
            int count32 = count * sizeof(quint16) / sizeof(quint32);
            while (count32--) {
                const quint32 s = *src32++;
                quint32 result1;
                quint32 result2;

                // red
                result1 = ((s & 0xf8000000) | ((s & 0xe0000000) >> 5)) >> 8;
                result2 = ((s & 0x0000f800) | ((s & 0x0000e000) >> 5)) << 8;

                // green
                result1 |= ((s & 0x07e00000) | ((s & 0x06000000) >> 6)) >> 11;
                result2 |= ((s & 0x000007e0) | ((s & 0x00000600) >> 6)) << 5;

                // blue
                result1 |= ((s & 0x001f0000) | ((s & 0x001c0000) >> 5)) >> 13;
                result2 |= ((s & 0x0000001f) | ((s & 0x0000001c) >> 5)) << 3;

                *dst32++ = result2;
                *dst32++ = result1;
            }
            if (count & 0x1) {
                const quint16 *src16 = reinterpret_cast<const quint16*>(src);
                dst32[count - 1] = qt_conv16ToRgb(src16[count - 1]);
            }
            return;
#endif
        }
    }

    const int bytesPerPixel = pixelFormat.bitsPerPixel / 8;

//    nibble = 0;

    for (int i = 0; i < count; ++i) {
        int r, g, b;

        switch (screendepth) {
#if 0
        case 4: {
            if (!nibble) {
                r = ((*src) & 0x0f) << 4;
            } else {
                r = (*src) & 0xf0;
                src++;
            }
            nibble = !nibble;
            g = b = r;
            break;
        }
#endif
        case 8: {
            QRgb rgb = qvnc_screen->clut()[*src];
            r = qRed(rgb);
            g = qGreen(rgb);
            b = qBlue(rgb);
            src++;
            break;
        }
        case 16: {
            quint16 p = *reinterpret_cast<const quint16*>(src);
            r = (p >> 11) & 0x1f;
            g = (p >> 5) & 0x3f;
            b = p & 0x1f;
            r <<= 3;
            g <<= 2;
            b <<= 3;
            src += sizeof(quint16);
            break;
        }
        case 32: {
            quint32 p = *reinterpret_cast<const quint32*>(src);
            r = (p >> 16) & 0xff;
            g = (p >> 8) & 0xff;
            b = p & 0xff;
            src += sizeof(quint32);
            break;
        }
        default: {
            r = g = b = 0;
            qDebug("QVNCServer: don't support %dbpp display", screendepth);
            return;
        }
        }

        r >>= (8 - pixelFormat.redBits);
        g >>= (8 - pixelFormat.greenBits);
        b >>= (8 - pixelFormat.blueBits);

        int pixel = (r << pixelFormat.redShift) |
                    (g << pixelFormat.greenShift) |
                    (b << pixelFormat.blueShift);

        if (sameEndian || pixelFormat.bitsPerPixel == 8) {
            memcpy(dst, &pixel, bytesPerPixel); // XXX: do a simple for-loop instead?
            dst += bytesPerPixel;
            continue;
        }


        if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
            switch (pixelFormat.bitsPerPixel) {
            case 16:
                pixel = (((pixel & 0x0000ff00) << 8)  |
                         ((pixel & 0x000000ff) << 24));
                break;
            case 32:
                pixel = (((pixel & 0xff000000) >> 24) |
                         ((pixel & 0x00ff0000) >> 8)  |
                         ((pixel & 0x0000ff00) << 8)  |
                         ((pixel & 0x000000ff) << 24));
                break;
            default:
                qDebug("Cannot handle %d bpp client", pixelFormat.bitsPerPixel);
            }
        } else { // QSysInfo::ByteOrder == QSysInfo::LittleEndian
            switch (pixelFormat.bitsPerPixel) {
            case 16:
                pixel = (((pixel & 0xff000000) >> 8) |
                         ((pixel & 0x00ff0000) << 8));
                break;
            case 32:
                pixel = (((pixel & 0xff000000) >> 24) |
                         ((pixel & 0x00ff0000) >> 8)  |
                         ((pixel & 0x0000ff00) << 8)  |
                         ((pixel & 0x000000ff) << 24));
                break;
            default:
                qDebug("Cannot handle %d bpp client",
                       pixelFormat.bitsPerPixel);
                break;
            }
        }
        memcpy(dst, &pixel, bytesPerPixel); // XXX: simple for-loop instead?
        dst += bytesPerPixel;
    }
}

// XXX
static inline QImage::Format formatForDepth(int depth)
{
    switch (depth) {
    case 8:
        return QImage::Format_Indexed8;
    case 16:
        return QImage::Format_RGB16;
    case 32:
        return QImage::Format_RGB32;
    default:
        return QImage::Format_Invalid;
    }
}

#ifndef QT_NO_QWS_CURSOR
static void blendCursor(QImage &image, const QRect &imageRect)
{
    const QRect cursorRect = qt_screencursor->boundingRect();
    const QRect intersection = (cursorRect & imageRect);
    const QRect destRect = intersection.translated(-imageRect.topLeft());
    const QRect srcRect = intersection.translated(-cursorRect.topLeft());

    QPainter painter(&image);
    painter.drawImage(destRect, qt_screencursor->image(), srcRect);
    painter.end();
}
#endif // QT_NO_QWS_CURSOR

QVNCDirtyMap::QVNCDirtyMap(QScreen *s)
    : bytesPerPixel(0), numDirty(0), screen(s)
{
    bytesPerPixel = screen->depth() / 8;
    bufferWidth = screen->deviceWidth();
    bufferHeight = screen->deviceHeight();
    bufferStride = bufferWidth * bytesPerPixel;
    buffer = new uchar[bufferHeight * bufferStride];

    mapWidth = (bufferWidth + MAP_TILE_SIZE - 1) / MAP_TILE_SIZE;
    mapHeight = (bufferHeight + MAP_TILE_SIZE - 1) / MAP_TILE_SIZE;
    map = new uchar[mapWidth * mapHeight];

    numTiles = mapWidth * mapHeight;
}

QVNCDirtyMap::~QVNCDirtyMap()
{
    delete[] map;
    delete[] buffer;
}

void QVNCDirtyMap::reset()
{
    memset(map, 1, numTiles);
    memset(buffer, 0, bufferHeight * bufferStride);
    numDirty = numTiles;
}

inline bool QVNCDirtyMap::dirty(int x, int y) const
{
    return map[y * mapWidth + x];
}

inline void QVNCDirtyMap::setClean(int x, int y)
{
    map[y * mapWidth + x] = 0;
    --numDirty;
}

template <class T>
void QVNCDirtyMapOptimized<T>::setDirty(int tileX, int tileY, bool force)
{
    static bool alwaysForce = qgetenv("QT_VNC_NO_COMPAREBUFFER").toInt();
    if (alwaysForce)
        force = true;

    bool changed = false;

    if (!force) {
        const int lstep = screen->linestep();
        const int startX = tileX * MAP_TILE_SIZE;
        const int startY = tileY * MAP_TILE_SIZE;
        const uchar *scrn = screen->base()
                            + startY * lstep + startX * bytesPerPixel;
        uchar *old = buffer + startY * bufferStride + startX * sizeof(T);

        const int tileHeight = (startY + MAP_TILE_SIZE > bufferHeight ?
                                bufferHeight - startY : MAP_TILE_SIZE);
        const int tileWidth = (startX + MAP_TILE_SIZE > bufferWidth ?
                               bufferWidth - startX : MAP_TILE_SIZE);
        const bool doInlines = (tileWidth == MAP_TILE_SIZE);

        int y = tileHeight;

        if (doInlines) { // hw: memcmp/memcpy is inlined when using constants
            while (y) {
                if (memcmp(old, scrn, sizeof(T) * MAP_TILE_SIZE)) {
                    changed = true;
                    break;
                }
                scrn += lstep;
                old += bufferStride;
                --y;
            }

            while (y) {
                memcpy(old, scrn, sizeof(T) * MAP_TILE_SIZE);
                scrn += lstep;
                old += bufferStride;
                --y;
            }
        } else {
            while (y) {
                if (memcmp(old, scrn, sizeof(T) * tileWidth)) {
                    changed = true;
                    break;
                }
                scrn += lstep;
                old += bufferStride;
                --y;
            }

            while (y) {
                memcpy(old, scrn, sizeof(T) * tileWidth);
                scrn += lstep;
                old += bufferStride;
                --y;
            }
        }
    }

    const int mapIndex = tileY * mapWidth + tileX;
    if ((force || changed) && !map[mapIndex]) {
        map[mapIndex] = 1;
        ++numDirty;
    }
}

template <class SRC>
QRfbHextileEncoder<SRC>::QRfbHextileEncoder(QVNCServer *s)
    : QRfbEncoder(s),
      singleColorHextile(this), dualColorHextile(this), multiColorHextile(this)
{
}

/*
    \internal
    Send dirty rects using hextile encoding.
*/
template <class SRC>
void QRfbHextileEncoder<SRC>::write()
{
    QWSDisplay::grab(true);

    QVNCDirtyMap *map = server->dirtyMap();
    QTcpSocket *socket = server->clientSocket();

    const quint32 encoding = htonl(5); // hextile encoding
    const int bytesPerPixel = server->clientBytesPerPixel();

    {
        const char tmp[2] = { 0, 0 }; // msg type, padding
        socket->write(tmp, sizeof(tmp));
    }
    {
        const quint16 count = htons(map->numDirty);
        socket->write((char *)&count, sizeof(count));
    }

    if (map->numDirty <= 0) {
        QWSDisplay::ungrab();
        return;
    }

    newBg = true;
    newFg = true;

    const QImage screenImage = server->screenImage();
    QRfbRect rect(0, 0, MAP_TILE_SIZE, MAP_TILE_SIZE);

    for (int y = 0; y < map->mapHeight; ++y) {
        if (rect.y + MAP_TILE_SIZE > server->screen()->height())
            rect.h = server->screen()->height() - rect.y;
        rect.w = MAP_TILE_SIZE;
        for (int x = 0; x < map->mapWidth; ++x) {
            if (!map->dirty(x, y))
                continue;
            map->setClean(x, y);

            rect.x = x * MAP_TILE_SIZE;
            if (rect.x + MAP_TILE_SIZE > server->screen()->deviceWidth())
                rect.w = server->screen()->deviceWidth() - rect.x;
            rect.write(socket);

            socket->write((char *)&encoding, sizeof(encoding));

            const uchar *screendata = screenImage.scanLine(rect.y)
                                      + rect.x * screenImage.depth() / 8;
            int linestep = screenImage.bytesPerLine();

#ifndef QT_NO_QWS_CURSOR
    // hardware cursors must be blended with the screen memory
            QImage tileImage;
            if (qt_screencursor->isAccelerated()) {
                const QRect tileRect(rect.x, rect.y, rect.w, rect.h);
                const QRect cursorRect = qt_screencursor->boundingRect()
                                         .translated(-server->screen()->offset());
                if (tileRect.intersects(cursorRect)) {
                    tileImage = screenImage.copy(tileRect);
                    blendCursor(tileImage,
                                tileRect.translated(server->screen()->offset()));
                    screendata = tileImage.bits();
                    linestep = tileImage.bytesPerLine();
                }
            }
#endif // QT_NO_QWS_CURSOR

            if (singleColorHextile.read(screendata, rect.w, rect.h, linestep)) {
                singleColorHextile.write(socket);
            } else if (dualColorHextile.read(screendata, rect.w, rect.h, linestep)) {
                dualColorHextile.write(socket);
            } else if (multiColorHextile.read(screendata, rect.w, rect.h, linestep)) {
                multiColorHextile.write(socket);
            } else if (server->doPixelConversion()) {
                const int bufferSize = rect.w * rect.h * bytesPerPixel + 1;
                const int padding = sizeof(quint32) - sizeof(char);
                buffer.resize(bufferSize + padding);

                buffer[padding] = 1; // Raw subencoding

                // convert pixels
                char *b = buffer.data() + padding + 1;
                const int bstep = rect.w * bytesPerPixel;
                for (int i = 0; i < rect.h; ++i) {
                    server->convertPixels(b, (const char*)screendata, rect.w);
                    screendata += linestep;
                    b += bstep;
                }
                socket->write(buffer.constData() + padding, bufferSize);
            } else {
                quint8 subenc = 1; // Raw subencoding
                socket->write((char *)&subenc, 1);

                // send pixels
                for (int i = 0; i < rect.h; ++i) {
                    socket->write((const char*)screendata,
                                  rect.w * bytesPerPixel);
                    screendata += linestep;
                }
            }
        }
        if (socket->state() == QAbstractSocket::UnconnectedState)
            break;
        rect.y += MAP_TILE_SIZE;
    }
    socket->flush();
    Q_ASSERT(map->numDirty == 0);

    QWSDisplay::ungrab();
}

void QRfbRawEncoder::write()
{
    QWSDisplay::grab(false);

    QVNCDirtyMap *map = server->dirtyMap();
    QTcpSocket *socket = server->clientSocket();

    const int bytesPerPixel = server->clientBytesPerPixel();

    // create a region from the dirty rects and send the region's merged rects.
    QRegion rgn;
    if (map) {
        for (int y = 0; y < map->mapHeight; ++y) {
            for (int x = 0; x < map->mapWidth; ++x) {
                if (!map->dirty(x, y))
                    continue;
                rgn += QRect(x * MAP_TILE_SIZE, y * MAP_TILE_SIZE,
                             MAP_TILE_SIZE, MAP_TILE_SIZE);
                map->setClean(x, y);
            }
        }

        rgn &= QRect(0, 0, server->screen()->deviceWidth(),
                     server->screen()->deviceHeight());
    }
    const QVector<QRect> rects = rgn.rects();

    {
        const char tmp[2] = { 0, 0 }; // msg type, padding
        socket->write(tmp, sizeof(tmp));
    }

    {
        const quint16 count = htons(rects.size());
        socket->write((char *)&count, sizeof(count));
    }

    if (rects.size() <= 0) {
        QWSDisplay::ungrab();
        return;
    }

    const QImage screenImage = server->screenImage();

    for (int i = 0; i < rects.size(); ++i) {
        const QRect tileRect = rects.at(i);
        const QRfbRect rect(tileRect.x(), tileRect.y(),
                            tileRect.width(), tileRect.height());
        rect.write(socket);

        const quint32 encoding = htonl(0); // raw encoding
        socket->write((char *)&encoding, sizeof(encoding));

        int linestep = screenImage.bytesPerLine();
        const uchar *screendata = screenImage.scanLine(rect.y)
                                  + rect.x * screenImage.depth() / 8;

#ifndef QT_NO_QWS_CURSOR
        // hardware cursors must be blended with the screen memory
        QImage tileImage;
        if (qt_screencursor->isAccelerated()) {
            const QRect cursorRect = qt_screencursor->boundingRect()
                                     .translated(-server->screen()->offset());
            if (tileRect.intersects(cursorRect)) {
                tileImage = screenImage.copy(tileRect);
                blendCursor(tileImage,
                            tileRect.translated(server->screen()->offset()));
                screendata = tileImage.bits();
                linestep = tileImage.bytesPerLine();
            }
        }
#endif // QT_NO_QWS_CURSOR

        if (server->doPixelConversion()) {
            const int bufferSize = rect.w * rect.h * bytesPerPixel;
            if (bufferSize > buffer.size())
                buffer.resize(bufferSize);

            // convert pixels
            char *b = buffer.data();
            const int bstep = rect.w * bytesPerPixel;
            for (int i = 0; i < rect.h; ++i) {
                server->convertPixels(b, (const char*)screendata, rect.w);
                screendata += linestep;
                b += bstep;
            }
            socket->write(buffer.constData(), bufferSize);
        } else {
            for (int i = 0; i < rect.h; ++i) {
                socket->write((const char*)screendata, rect.w * bytesPerPixel);
                screendata += linestep;
            }
        }
        if (socket->state() == QAbstractSocket::UnconnectedState)
            break;
    }
    socket->flush();

    QWSDisplay::ungrab();
}

inline QImage QVNCServer::screenImage() const
{
    return QImage(qvnc_screen->base(), qvnc_screen->deviceWidth(),
                  qvnc_screen->deviceHeight(),
                  formatForDepth(qvnc_screen->depth()));
}

void QVNCServer::checkUpdate()
{
    if (wantUpdate && dirtyMap()->numDirty > 0) {
        if (encoder)
            encoder->write();
        wantUpdate = false;
    }
}

void QVNCServer::discardClient()
{
    timer->stop();
    state = Unconnected;
    delete encoder;
    encoder = 0;
    if (!qvnc_screen->d_ptr->subscreen)
        QWSServer::instance()->enablePainting(false);
}


//===========================================================================

/*!
    \class QVNCScreen
    \internal
    \ingroup qws

    \brief The QVNCScreen class implements a screen driver for VNC
    servers.

    Note that this class is only available in \l {Qtopia Core}.
    Custom screen drivers can be added by subclassing the QScreen
    class, using the QScreenDriverFactory class to dynamically load
    the driver into the application.

    The VNC protocol allows you to view and interact with the
    computer's display from anywhere on the network. See the \l {The
    VNC Protocol and Qtopia Core}{VNC protocol} documentation for more details.

    The default implementation of QVNCScreen inherits QLinuxFbScreen,
    but any QScreen subclass, or QScreen itself, can serve as its base
    class. This is easily achieved by manipulating the \c
    VNCSCREEN_BASE definition in the header file.

    \sa QScreen, {Running Applications}
*/

/*!
    \fn QVNCScreen::QVNCScreen(int displayId)

    Constructs a QVNCScreen object. The \a displayId argument
    identifies the Qtopia Core server to connect to.
*/
QVNCScreen::QVNCScreen(int display_id)
    : QScreen(display_id)
{
    d_ptr = new QVNCScreenPrivate(this);
}

/*!
    Destroys this QVNCScreen object.
*/
QVNCScreen::~QVNCScreen()
{
    delete d_ptr;
}

/*!
    \reimp
*/
void QVNCScreen::setDirty(const QRect& rect)
{
    d_ptr->setDirty(rect);
}

void QVNCScreenPrivate::setDirty(const QRect& rect, bool force)
{
    if (rect.isEmpty())
        return;

    if (subscreen)
        subscreen->setDirty(rect);

    if (!vncServer || !vncServer->isConnected())
        return;

    const QRect r = rect.translated(-q_ptr->offset());
    int x1 = r.x() / MAP_TILE_SIZE;
    int y1 = r.y() / MAP_TILE_SIZE;
    for (int y = y1; y <= r.bottom() / MAP_TILE_SIZE && y < dirty->mapHeight; y++)
        for (int x = x1; x <= r.right() / MAP_TILE_SIZE && x < dirty->mapWidth; x++)
            dirty->setDirty(x, y, force);

    vncServer->setDirty();
}

static int getDisplayId(const QString &spec)
{
    QRegExp regexp(QLatin1String(":(\\d+)\\b"));
    if (regexp.lastIndexIn(spec) != -1) {
        const QString capture = regexp.cap(1);
        return capture.toInt();
    }
    return 0;
}

/*!
    \reimp
*/
bool QVNCScreen::connect(const QString &displaySpec)
{
    QString dspec = displaySpec;
    if (dspec.startsWith(QLatin1String("vnc:"), Qt::CaseInsensitive))
        dspec = dspec.mid(QString(QLatin1String("vnc:")).size());
    else if (dspec.compare(QLatin1String("vnc"), Qt::CaseInsensitive) == 0)
        dspec = QString();

    const QString displayIdSpec = QString(QLatin1String(" :%1")).arg(displayId);
    if (dspec.endsWith(displayIdSpec))
        dspec = dspec.left(dspec.size() - displayIdSpec.size());

    QStringList args = dspec.split(QLatin1Char(':'),
                                   QString::SkipEmptyParts);
    QRegExp refreshRegexp(QLatin1String("^refreshrate=(\\d+)$"));
    int index = args.indexOf(refreshRegexp);
    if (index >= 0) {
        d_ptr->refreshRate = refreshRegexp.cap(1).toInt();
        args.removeAt(index);
        dspec = args.join(QLatin1String(":"));
    }

    QString driver = dspec;
    int colon = driver.indexOf(QLatin1Char(':'));
    if (colon >= 0)
        driver.truncate(colon);

    QScreen *screen;
    if (QScreenDriverFactory::keys().contains(driver, Qt::CaseInsensitive)) {
        const int id = getDisplayId(dspec);
        screen = qt_get_screen(id, dspec.toLatin1().constData());
        d_ptr->subscreen = screen;

    } else { // create virtual screen
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        QScreen::setFrameBufferLittleEndian(false);
#endif

        d = qgetenv("QWS_DEPTH").toInt();
        if (!d)
            d = 16;

        QByteArray str = qgetenv("QWS_SIZE");
        if(!str.isEmpty()) {
            sscanf(str.constData(), "%dx%d", &w, &h);
            dw = w;
            dh = h;
        } else {
            dw = w = 640;
            dh = h = 480;
        }

        const QStringList args = displaySpec.split(QLatin1Char(':'),
                                                   QString::SkipEmptyParts);

        if (args.contains(QLatin1String("paintonscreen"), Qt::CaseInsensitive))
            d_ptr->doOnScreenSurface = true;

        QRegExp depthRegexp(QLatin1String("^depth=(\\d+)$"));
        if (args.indexOf(depthRegexp) != -1)
            d = depthRegexp.cap(1).toInt();

        QRegExp sizeRegexp(QLatin1String("^size=(\\d+)x(\\d+)$"));
        if (args.indexOf(sizeRegexp) != -1) {
            dw = w = sizeRegexp.cap(1).toInt();
            dh = h = sizeRegexp.cap(2).toInt();
        }

        // Handle display physical size spec.
        QRegExp mmWidthRegexp(QLatin1String("^mmWidth=?(\\d+)$"));
        if (args.indexOf(mmWidthRegexp) != -1) {
            const int mmWidth = mmWidthRegexp.cap(1).toInt();
            if (mmWidth > 0)
                d_ptr->dpiX = dw * 25.4 / mmWidth;
        }
        QRegExp mmHeightRegexp(QLatin1String("^mmHeight=?(\\d+)$"));
        if (args.indexOf(mmHeightRegexp) != -1) {
            const int mmHeight = mmHeightRegexp.cap(1).toInt();
            if (mmHeight > 0)
                d_ptr->dpiY = dh * 25.4 / mmHeight;
        }
        QRegExp dpiRegexp(QLatin1String("^dpi=(\\d+)(?:,(\\d+))?$"));
        if (args.indexOf(dpiRegexp) != -1) {
            const qreal dpiX = dpiRegexp.cap(1).toFloat();
            const qreal dpiY = dpiRegexp.cap(2).toFloat();
            if (dpiX > 0)
                d_ptr->dpiX = dpiX;
            d_ptr->dpiY = (dpiY > 0 ? dpiY : dpiX);
        }

        QWSServer::setDefaultMouse("None");
        QWSServer::setDefaultKeyboard("None");
    }

    d_ptr->configure();

    // XXX
    qt_screen = this;

    return true;
}

/*!
    \reimp
*/
void QVNCScreen::disconnect()
{
    if (d_ptr->subscreen) {
        d_ptr->subscreen->disconnect();
    } else {
#ifndef QT_NO_QWS_MULTIPROCESS
        if (d_ptr->shm)
            d_ptr->shm->detach();
#endif
    }
}

/*!
    \reimp
*/
bool QVNCScreen::initDevice()
{
    if (!d_ptr->subscreen && d == 4) {
        screencols = 16;
        int val = 0;
        for (int idx = 0; idx < 16; idx++, val += 17) {
            screenclut[idx] = qRgb(val, val, val);
        }
    }
    d_ptr->vncServer = new QVNCServer(this, displayId);
    d_ptr->vncServer->setRefreshRate(d_ptr->refreshRate);

    switch (depth()) {
#ifdef QT_QWS_DEPTH_32
    case 32:
        d_ptr->dirty = new QVNCDirtyMapOptimized<quint32>(this);
        break;
#endif
#ifdef QT_QWS_DEPTH_16
    case 16:
        d_ptr->dirty = new QVNCDirtyMapOptimized<quint16>(this);
        break;
#endif
#ifdef QT_QWS_DEPTH_8
    case 8:
        d_ptr->dirty = new QVNCDirtyMapOptimized<quint8>(this);
        break;
#endif
    default:
        qWarning("QVNCScreen::initDevice: No support for screen depth %d",
                 depth());
        d_ptr->dirty = 0;
        return false;
    }

    if (d_ptr->subscreen)
        return d_ptr->subscreen->initDevice();

#ifndef QT_NO_QWS_CURSOR
    qt_screencursor = new QVNCCursor(this);
#endif

#ifdef QT_BUILD_INTERNAL
    if (qgetenv("QT_VNC_NO_DISABLEPAINTING").toInt() <= 0)
#endif
    // No need to do painting while there's no clients attached
    QWSServer::instance()->enablePainting(false);

    return true;
}

/*!
    \reimp
*/
void QVNCScreen::shutdownDevice()
{
    delete d_ptr->vncServer;
    if (d_ptr->subscreen) {
        d_ptr->subscreen->shutdownDevice();
    } else {
#ifndef QT_NO_QWS_MULTIPROCESS
        if (d_ptr->shm)
            d_ptr->shm->destroy();
#endif
    }
    delete d_ptr->dirty;
}

/*!
    \reimp
*/
void QVNCScreen::setMode(int w, int h, int d)
{
    if (d_ptr->subscreen) {
        d_ptr->subscreen->setMode(w, h, d);
    } else {
        QScreen::dw = QScreen::w = w;
        QScreen::dh = QScreen::h = h;
        QScreen::d = d;
    }
    d_ptr->configure();
    setDirty(region().boundingRect());
}

/*!
    \reimp
*/
bool QVNCScreen::supportsDepth(int depth) const
{
    if (d_ptr->subscreen)
        return d_ptr->subscreen->supportsDepth(depth);
    return false;
}

/*!
    \reimp
*/
void QVNCScreen::save()
{
    if (d_ptr->subscreen)
        d_ptr->subscreen->save();
    QScreen::save();
}

/*!
    \reimp
*/
void QVNCScreen::restore()
{
    if (d_ptr->subscreen)
        d_ptr->subscreen->restore();
    QScreen::restore();
}

/*!
    \reimp
*/
void QVNCScreen::blank(bool on)
{
    if (d_ptr->subscreen)
        d_ptr->subscreen->blank(on);
}

/*!
    \reimp
*/
bool QVNCScreen::onCard(const unsigned char *ptr) const
{
    if (d_ptr->subscreen)
        return d_ptr->subscreen->onCard(ptr);
    return false;
}

/*!
    \reimp
*/
bool QVNCScreen::onCard(const unsigned char *ptr, ulong &offset) const
{
    if (d_ptr->subscreen)
        return d_ptr->subscreen->onCard(ptr, offset);
    return false;
}

/*!
    \reimp
*/
bool QVNCScreen::isInterlaced() const
{
    if (d_ptr->subscreen)
        return d_ptr->subscreen->isInterlaced();
    return false;
}

/*!
    \reimp
*/
int QVNCScreen::memoryNeeded(const QString &str)
{
    if (d_ptr->subscreen)
        return d_ptr->subscreen->memoryNeeded(str);
    else
        return QScreen::memoryNeeded(str);
}

/*!
    \reimp
*/
int QVNCScreen::sharedRamSize(void *ptr)
{
    if (d_ptr->subscreen)
        return d_ptr->subscreen->sharedRamSize(ptr);
    else
        return QScreen::sharedRamSize(ptr);
}

/*!
    \reimp
*/
void QVNCScreen::haltUpdates()
{
    if (d_ptr->subscreen)
        d_ptr->subscreen->haltUpdates();
    else
        QScreen::haltUpdates();
}

/*!
    \reimp
*/
void QVNCScreen::resumeUpdates()
{
    if (d_ptr->subscreen)
        d_ptr->subscreen->resumeUpdates();
}

/*!
    \reimp
*/
void QVNCScreen::exposeRegion(QRegion r, int changing)
{
    if (d_ptr->subscreen) {
        d_ptr->subscreen->exposeRegion(r, changing);

        const QVector<QRect> rects = (region() & r).rects();
        for (int i = 0; i < rects.size(); ++i)
            setDirty(rects.at(i));
    } else {
        QScreen::exposeRegion(r, changing);
    }
}

/*!
    \reimp
*/
void QVNCScreen::blit(const QImage &img, const QPoint &topLeft, const QRegion &region)
{
    if (d_ptr->subscreen)
        d_ptr->subscreen->blit(img, topLeft, region);
    else
        QScreen::blit(img, topLeft, region);
}

/*!
    \reimp
*/
void QVNCScreen::solidFill(const QColor &color, const QRegion &region)
{
    if (d_ptr->subscreen)
        d_ptr->subscreen->solidFill(color, region);
    else
        QScreen::solidFill(color, region);
}

/*!
    \reimp
*/
QWSWindowSurface* QVNCScreen::createSurface(QWidget *widget) const
{
    if (d_ptr->subscreen)
        return d_ptr->subscreen->createSurface(widget);

#ifndef QT_NO_PAINTONSCREEN
    // XXX: will not work together with transparent windows until full
    // compositioning is implemented
    if (d_ptr->doOnScreenSurface) {
        if (widget->d_func()->isOpaque() && (depth() == 16 || depth() == 32))
            return new QWSOnScreenSurface(widget);
    }
#endif

    return QScreen::createSurface(widget);
}

/*!
    \reimp
*/
QList<QScreen*> QVNCScreen::subScreens() const
{
    if (d_ptr->subscreen)
        return d_ptr->subscreen->subScreens();
    return QScreen::subScreens();
}

/*!
    \reimp
*/
QRegion QVNCScreen::region() const
{
    if (d_ptr->subscreen)
        return d_ptr->subscreen->region();
    return QScreen::region();
}

#endif // QT_NO_QWS_VNC


