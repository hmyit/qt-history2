/****************************************************************************
**
** Implementation of QUtf{8,16}Codec class.
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the tools module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qutfcodec.h"

#ifndef QT_NO_TEXTCODEC

int QUtf8Codec::mibEnum() const
{
    return 106;
}

QByteArray QUtf8Codec::fromUnicode(const QString& uc, int& lenInOut) const
{
    int l = uc.length();
    if (lenInOut > 0)
        l = qMin(l, lenInOut);
    int rlen = l*3+1;
    QByteArray rstr;
    rstr.resize(rlen);
    uchar* cursor = (uchar*)rstr.data();
    const QChar *ch = uc.unicode();
    for (int i=0; i < l; i++) {
        uint u = ch->unicode();
        if (u < 0x80) {
            *cursor++ = (uchar)u;
        } else {
            if (u < 0x0800) {
                *cursor++ = 0xc0 | ((uchar) (u >> 6));
            } else {
                if (u >= 0xd800 && u < 0xdc00 && i < l-1) {
                    unsigned short low = ch[1].unicode();
                    if (low >= 0xdc00 && low < 0xe000) {
                        ++ch;
                        ++i;
                        u = (u - 0xd800)*0x400 + (low - 0xdc00) + 0x10000;
                    }
                }
                if (u > 0xffff) {
                    // see QString::fromUtf8() and QString::utf8() for explanations
                    if (u > 0x10fe00 && u < 0x10ff00) {
                        *cursor++ = (u - 0x10fe00);
                        ++ch;
                        continue;
                    } else {
                        *cursor++ = 0xf0 | ((uchar) (u >> 18));
                        *cursor++ = 0x80 | (((uchar) (u >> 12)) & 0x3f);
                    }
                } else {
                    *cursor++ = 0xe0 | ((uchar) (u >> 12));
                }
                *cursor++ = 0x80 | (((uchar) (u >> 6)) & 0x3f);
            }
            *cursor++ = 0x80 | ((uchar) (u&0x3f));
        }
        ++ch;
    }
    *cursor = 0;
    lenInOut = cursor - (const uchar*)rstr.constData();
    ((QByteArray&)rstr).resize(lenInOut+1);
    return rstr;
}

QString QUtf8Codec::toUnicode(const char* chars, int len) const
{
    if (len > 3 && (uchar)chars[0] == 0xef && (uchar)chars[1] == 0xbb && (uchar)chars[2] == 0xbf) {
        // starts with a byte order mark
        chars += 3;
        len -= 3;
    }
    return QString::fromUtf8(chars, len);
}


const char* QUtf8Codec::name() const
{
    return "UTF-8";
}

int QUtf8Codec::heuristicContentMatch(const char* chars, int len) const
{
    int score = 0;
    for (int i=0; i<len; i++) {
        uchar ch = chars[i];
        // No nulls allowed.
        if (!ch)
            return -1;
        if (ch < 128) {
            // Inconclusive
            score++;
        } else if ((ch&0xe0) == 0xc0) {
            if (i < len-1) {
                uchar c2 = chars[++i];
                if ((c2&0xc0) != 0x80)
                    return -1;
                score+=3;
            }
        } else if ((ch&0xf0) == 0xe0) {
            if (i < len-1) {
                uchar c2 = chars[++i];
                if ((c2&0xc0) != 0x80) {
                    return -1;
#if 0
                    if (i < len-1) {
                        uchar c3 = chars[++i];
                        if ((c3&0xc0) != 0x80)
                            return -1;
                        score+=3;
                    }
#endif
                }
                score+=2;
            }
        }
    }
    return score;
}




class QUtf8Decoder : public QTextDecoder {
    uint uc;
    int need;
    bool headerDone;
public:
    QUtf8Decoder() : need(0), headerDone(false)
    {
    }

    QString toUnicode(const char* chars, int len)
    {
        QString result;
        result.resize(len); // worst case
        QChar *qch = (QChar *)result.unicode();
        uchar ch;
        for (int i=0; i<len; i++) {
            ch = *chars++;
            if (need) {
                if ((ch&0xc0) == 0x80) {
                    uc = (uc << 6) | (ch & 0x3f);
                    need--;
                    if (!need) {
                        if (uc > 0xffff) {
                            // surrogate pair
                            uc -= 0x10000;
                            unsigned short high = uc/0x400 + 0xd800;
                            unsigned short low = uc%0x400 + 0xdc00;
                            *qch++ = QChar(high);
                            *qch++ = QChar(low);
                            headerDone = true;
                        } else {
                            if (headerDone || QChar(uc) != QChar(QChar::byteOrderMark))
                                *qch++ = uc;
                            headerDone = true;
                        }
                    }
                } else {
                    // error
                    *qch++ = QChar::replacement;
                    need = 0;
                }
            } else {
                if (ch < 128) {
                    *qch++ = ch;
                    headerDone = true;
                } else if ((ch & 0xe0) == 0xc0) {
                    uc = ch & 0x1f;
                    need = 1;
                } else if ((ch & 0xf0) == 0xe0) {
                    uc = ch & 0x0f;
                    need = 2;
                } else if ((ch&0xf8) == 0xf0) {
                    uc = ch & 0x07;
                    need = 3;
                }
            }
        }
        result.truncate(qch - result.unicode());
        return result;
    }
};

QTextDecoder* QUtf8Codec::makeDecoder() const
{
    return new QUtf8Decoder;
}






int QUtf16Codec::mibEnum() const
{
    return 1000;
}

const char* QUtf16Codec::name() const
{
    return "ISO-10646-UCS-2";
}

int QUtf16Codec::heuristicContentMatch(const char* chars, int len) const
{
    uchar* uchars = (uchar*)chars;
    if (len >= 2 && (uchars[0] == 0xff && uchars[1] == 0xfe ||
                      uchars[1] == 0xff && uchars[0] == 0xfe))
        return len;
    else
        return 0;
}




class QUtf16Encoder : public QTextEncoder {
    bool headerdone;
public:
    QUtf16Encoder() : headerdone(false)
    {
    }

    QByteArray fromUnicode(const QString& uc, int& lenInOut)
    {
        if (headerdone) {
            lenInOut = uc.length()*sizeof(QChar);
            QByteArray d;
            d.resize(lenInOut);
            memcpy(d.data(),uc.unicode(),lenInOut);
            return d;
        } else {
            headerdone = true;
            lenInOut = (1+uc.length())*sizeof(QChar);
            QByteArray d;
            d.resize(lenInOut);
            QChar bom(QChar::byteOrderMark);
            memcpy(d.data(),&bom,sizeof(QChar));
            memcpy(d.data()+sizeof(QChar),uc.unicode(),uc.length()*sizeof(QChar));
            return d;
        }
    }
};

class QUtf16Decoder : public QTextDecoder {
    uchar buf;
    bool half;
    bool swap;
    bool headerdone;

public:
    QUtf16Decoder() : half(false), swap(false), headerdone(false)
    {
    }

    QString toUnicode(const char* chars, int len)
    {
        QString result;
        result.resize(len); // worst case
        QChar *qch = (QChar *)result.unicode();
        QChar ch;
        while (len--) {
            if (half) {
                if (swap) {
                    ch.setRow(*chars++);
                    ch.setCell(buf);
                } else {
                    ch.setRow(buf);
                    ch.setCell(*chars++);
                }
                if (!headerdone) {
                    if (ch == QChar::byteOrderSwapped) {
                        swap = !swap;
                    } else if (ch == QChar::byteOrderMark) {
                        // Ignore ZWNBSP
                    } else {
                        *qch++ = ch;
                    }
                    headerdone = true;
                } else
                    *qch++ = ch;
                half = false;
            } else {
                buf = *chars++;
                half = true;
            }
        }
        result.truncate(qch - result.unicode());
        return result;
    }
};

QTextDecoder* QUtf16Codec::makeDecoder() const
{
    return new QUtf16Decoder;
}

QTextEncoder* QUtf16Codec::makeEncoder() const
{
    return new QUtf16Encoder;
}

#endif //QT_NO_TEXTCODEC
