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

#ifndef QTEXTCODEC_H
#define QTEXTCODEC_H

#include "qstring.h"

#ifndef QT_NO_TEXTCODEC

class QTextCodec;
class QIODevice;

class QTextDecoder;
class QTextEncoder;
template <typename T> class QList;

class Q_CORE_EXPORT QTextCodec
{
public:
    static QTextCodec* codecForName(const QByteArray &name);
    static QTextCodec* codecForName(const char *name) { return codecForName(QByteArray(name)); }
    static QTextCodec* codecForMib(int mib);

    static QList<QByteArray> availableCodecs();
    static QList<int> availableMibs();

    static QTextCodec* codecForLocale();
    static void setCodecForLocale(QTextCodec *c);

    static QTextCodec* codecForTr();
    static void setCodecForTr(QTextCodec *c);

    static QTextCodec* codecForCStrings();
    static void setCodecForCStrings(QTextCodec *c);

    QTextDecoder* makeDecoder() const;
    QTextEncoder* makeEncoder() const;

    bool canEncode(QChar) const;
    bool canEncode(const QString&) const;

    QString toUnicode(const QByteArray&) const;
    QString toUnicode(const char* chars) const;
    QByteArray fromUnicode(const QString& uc) const;
    enum ConversionFlag {
        DefaultConversion,
        ConvertInvalidToNull = 0x80000000,
        KeepHeader = 0x1,
        WriteHeader = 0x2
    };
    Q_DECLARE_FLAGS(ConversionFlags, ConversionFlag)

    struct ConverterState {
        ConverterState(ConversionFlags f = DefaultConversion)
            : flags(f), remainingChars(0), invalidChars(0), d(0) { state_data[0] = state_data[1] = state_data[2] = 0; }
        ~ConverterState() { if (d) qFree(d); }
        ConversionFlags flags;
        int remainingChars;
        int invalidChars;
        uint state_data[3];
        void *d;
    };

    QString toUnicode(const char *in, int length, ConverterState *state = 0) const
        { return convertToUnicode(in, length, state); }
    QByteArray fromUnicode(const QChar *in, int length, ConverterState *state = 0) const
        { return convertFromUnicode(in, length, state); }

    virtual QByteArray name() const = 0;
    virtual QList<QByteArray> aliases() const;
    virtual int mibEnum() const = 0;

protected:
    virtual QString convertToUnicode(const char *in, int length, ConverterState *state) const = 0;
    virtual QByteArray convertFromUnicode(const QChar *in, int length, ConverterState *state) const = 0;

    QTextCodec();
    virtual ~QTextCodec();

public:
#ifdef QT_COMPAT
    static QT_COMPAT QTextCodec* codecForContent(const char*, int) { return 0; }
    static QT_COMPAT const char* locale();
    static QT_COMPAT QTextCodec* codecForName(const char* hint, int) { return codecForName(QByteArray(hint)); }
    QT_COMPAT QByteArray fromUnicode(const QString& uc, int& lenInOut) const;
    QT_COMPAT QString toUnicode(const QByteArray&, int len) const;
    QT_COMPAT QByteArray mimeName() const { return name(); }
#endif

private:
    friend class QTextCodecCleanup;
    static QTextCodec *cftr;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QTextCodec::ConversionFlags)

inline QTextCodec* QTextCodec::codecForTr() { return cftr; }
inline void QTextCodec::setCodecForTr(QTextCodec *c) { cftr = c; }
inline QTextCodec* QTextCodec::codecForCStrings() { return QString::codecForCStrings; }
inline void QTextCodec::setCodecForCStrings(QTextCodec *c) { QString::codecForCStrings = c; }

class Q_CORE_EXPORT QTextEncoder {
public:
    QTextEncoder(const QTextCodec *codec) : c(codec) {}
    ~QTextEncoder();
    QByteArray fromUnicode(const QString& str);
    QByteArray fromUnicode(const QChar *uc, int len);
#ifdef QT_COMPAT
    QByteArray fromUnicode(const QString& uc, int& lenInOut);
#endif
private:
    const QTextCodec *c;
    QTextCodec::ConverterState state;
};

class Q_CORE_EXPORT QTextDecoder {
public:
    QTextDecoder(const QTextCodec *codec) : c(codec) {}
    ~QTextDecoder();
    QString toUnicode(const char* chars, int len);
    QString toUnicode(const QByteArray &ba);
private:
    const QTextCodec *c;
    QTextCodec::ConverterState state;
};

#endif // QT_NO_TEXTCODEC
#endif // QTEXTCODEC_H
