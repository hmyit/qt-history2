/****************************************************************************
**
** Copyright (C) 2006-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QByteArray>
#include <QtGlobal>

#include "qbuiltintypes_p.h"
#include "qvalidationerror_p.h"

#include "qbase64binary_p.h"

QT_BEGIN_NAMESPACE

using namespace Patternist;

Base64Binary::Base64Binary(const QByteArray &val) : m_value(val)
{
}

static const char Base64DecMap[128] =
{
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x3F,
     0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
     0x3C, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
     0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
     0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
     0x17, 0x18, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
     0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
     0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
     0x31, 0x32, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00
};

/**
 * If @p instr is invalid Base64 content, @p ok is set to
 * false, and the returned QByteArray has an undefined value.
 */
void static base64Decode(const QByteArray &in, QByteArray &out, bool &ok)
{
    out.resize(0);

    if(in.isEmpty())
    {
        ok = false;
        return;
    }

    ok = true;
    int len = in.size(), tail = len;
    const char *const data = in.data();
    unsigned int eqCount = 0;

    // Find the tail end of the actual encoded data even if
    // there is/are trailing CR and/or LF.
    while(data[tail - 1] == '=')
    {
        --tail;
        if(data[tail] != '=')
            len = tail;
        else
            ++eqCount;
    }

    if(eqCount > 2)
    {
        qDebug() << "Exiting because of eqCount being: " << eqCount;
        ok = false;
        return;
    }

    unsigned int outIdx = 0;
    const int count = len; // We modify len below
    out.resize((count));

    for(int idx = 0; idx < count; ++idx)
    {
        const unsigned char ch = data[idx];
        if((ch > 47 && ch < 58) ||
            (ch > 64 && ch < 91) ||
            (ch > 96 && ch < 123) ||
            ch == '+' ||
            ch == '/')
        {
            out[outIdx++] = Base64DecMap[ch];
        }
        else if(ch == '=')
        {
            if((idx + 1) == count || data[idx + 1] == '=')
            {
                qDebug() << "Accepting '='..";
                out[++outIdx] = Base64DecMap[ch];
                continue;
            }

            qDebug() << "Exiting because of invalidly placed '='..";
            ok = false;
            return;
        }
        else if(ch == ' ')
        {
            /* One space is ok, and the previously applied whitespace facet(not implemented
             * at this time of writing) have ensured it's only one space, so we assume that. */
            --tail;
            --len;
            continue;
        }
        else
        {
            ok = false;
            return;
        }
    }

    if(outIdx % 4 != 0)
    {
        qDebug() << "Exiting because bytes not being a multiple of four: " << outIdx;
        ok = false;
        return;
    }

    out.resize(len);

    // 4-byte to 3-byte conversion
    len = (tail > (len / 4)) ? tail - (len / 4) : 0;
    int sidx = 0, didx = 0;
    if(len > 1)
    {
      while(didx < len - 2)
      {
          out[didx] =(((out[sidx] << 2) & 255) | ((out[sidx + 1] >> 4) & 003));
          out[didx + 1] =(((out[sidx + 1] << 4) & 255) | ((out[sidx + 2] >> 2) & 017));
          out[didx + 2] =(((out[sidx + 2] << 6) & 255) | (out[sidx + 3] & 077));
          sidx += 4;
          didx += 3;
      }
    }

    if(didx < len)
        out[didx] =(((out[sidx] << 2) & 255) | ((out[sidx + 1] >> 4) & 003));

    if(++didx < len)
        out[didx] =(((out[sidx + 1] << 4) & 255) | ((out[sidx + 2] >> 2) & 017));

    // Resize the output buffer
    if(len == 0 || len < out.size())
      out.resize(len);
}

AtomicValue::Ptr Base64Binary::fromLexical(const QString &str)
{
    const QString simple(str.simplified());
    if(simple.isEmpty())
        return AtomicValue::Ptr(new Base64Binary(QByteArray()));

    bool ok = false;
    QByteArray result;
    base64Decode(simple.toUtf8(), result, ok);

    if(ok)
        return AtomicValue::Ptr(new Base64Binary(result));
    else
        return ValidationError::createError();
}

Base64Binary::Ptr Base64Binary::fromValue(const QByteArray &data)
{
    return Base64Binary::Ptr(new Base64Binary(data));
}

QString Base64Binary::stringValue() const
{
    return QString::fromLatin1(m_value.toBase64().constData());
}

ItemType::Ptr Base64Binary::type() const
{
    return BuiltinTypes::xsBase64Binary;
}

QByteArray Base64Binary::asByteArray() const
{
    return m_value;
}

// vim: et:ts=4:sw=4:sts=4

QT_END_NAMESPACE
