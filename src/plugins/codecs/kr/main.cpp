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

#include <qtextcodecplugin.h>
#include <qtextcodec.h>
#include <qstringlist.h>

#include "qeuckrcodec.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_TEXTCODECPLUGIN

class KRTextCodecs : public QTextCodecPlugin
{
public:
    KRTextCodecs() {}

    QList<QByteArray> names() const;
    QList<QByteArray> aliases() const;
    QList<int> mibEnums() const;

    QTextCodec *createForMib(int);
    QTextCodec *createForName(const QByteArray &);
};

QList<QByteArray> KRTextCodecs::names() const
{
    QList<QByteArray> list;
    list += QEucKrCodec::_name();
#ifdef Q_WS_X11
    list += QFontKsc5601Codec::_name();
#endif
    list += QCP949Codec::_name();
    return list;
}

QList<QByteArray> KRTextCodecs::aliases() const
{
    QList<QByteArray> list;
    list += QEucKrCodec::_aliases();
#ifdef Q_WS_X11
    list += QFontKsc5601Codec::_aliases();
#endif
    list += QCP949Codec::_aliases();
    return list;
}

QList<int> KRTextCodecs::mibEnums() const
{
    QList<int> list;
    list += QEucKrCodec::_mibEnum();
#ifdef Q_WS_X11
    list += QFontKsc5601Codec::_mibEnum();
#endif
    list += QCP949Codec::_mibEnum();
    return list;
}

QTextCodec *KRTextCodecs::createForMib(int mib)
{
    if (mib == QEucKrCodec::_mibEnum())
        return new QEucKrCodec;
#ifdef Q_WS_X11
    if (mib == QFontKsc5601Codec::_mibEnum())
        return new QFontKsc5601Codec;
#endif
    if (mib == QCP949Codec::_mibEnum())
        return new QCP949Codec;
    return 0;
}


QTextCodec *KRTextCodecs::createForName(const QByteArray &name)
{
    if (name == QEucKrCodec::_name() || QEucKrCodec::_aliases().contains(name))
        return new QEucKrCodec;
#ifdef Q_WS_X11
    if (name == QFontKsc5601Codec::_name() || QFontKsc5601Codec::_aliases().contains(name))
        return new QFontKsc5601Codec;
#endif
    if (name == QCP949Codec::_name() || QCP949Codec::_aliases().contains(name))
        return new QCP949Codec;
    return 0;
}


Q_EXPORT_STATIC_PLUGIN(KRTextCodecs);
Q_EXPORT_PLUGIN2(qkrcodecs, KRTextCodecs);

#endif // QT_NO_TEXTCODECPLUGIN

QT_END_NAMESPACE
