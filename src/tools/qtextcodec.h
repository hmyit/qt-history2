/****************************************************************************
** $Id: //depot/qt/main/src/tools/qtextcodec.h#6 $
**
** Definition of QTextCodec class
**
** Created : 981015
**
** Copyright (C) 1998 Troll Tech AS.  All rights reserved.
**
** This file is part of Troll Tech's internal development tree for Qt.
**
** This header text will be replaced by an appropriate text by the
** mkdist script which generates external distributions.
**
** If you are using the Qt Professional Edition or the Qt Free Edition,
** please notify Troll Tech at <info@troll.no> if you see this text.
**
** To Troll Tech developers: This header was generated by the script
** fixcopyright-int. It has the same number of text lines as the free
** and professional editions to avoid line number inconsistency.
**
*****************************************************************************/

#ifndef QTEXTCODEC_H
#define QTEXTCODEC_H

#include "qstring.h"

class QTextCodec;
class QIODevice;

class QTextEncoder {
public:
    virtual ~QTextEncoder();
    virtual char* fromUnicode(const QString& uc, int& len_in_out) = 0;
};

class QTextDecoder {
public:
    virtual ~QTextDecoder();
    virtual QString toUnicode(const char* chars, int len) = 0;
};

class QTextCodec {
public:
    virtual ~QTextCodec();

    static QTextCodec* loadCharmap(QIODevice*);
    static QTextCodec* loadCharmapFile(QString filename);

    static QTextCodec* codecForMib(int mib);
    static QTextCodec* codecForName(const char* hint);
    static QTextCodec* codecForContent(const char* chars, int len);
    static QTextCodec* codecForIndex(int i);

    static const char* locale();
    
    virtual const char* name() const = 0;
    virtual int mib() const = 0;

    virtual QTextDecoder* makeDecoder() const;
    virtual QTextEncoder* makeEncoder() const;

    virtual QString toUnicode(const char* chars, int len) const;
    virtual char* fromUnicode(const QString& uc, int& len_in_out) const;

    char* fromUnicode(const QString& uc) const;
    QString toUnicode(const QByteArray&, int len) const;
    QString toUnicode(const QByteArray&) const;
    QString toUnicode(const char* chars) const;

    virtual int heuristicContentMatch(const char* chars, int len) const = 0;
    virtual int heuristicNameMatch(const char* hint) const;

protected:
    QTextCodec();
    static int simpleHeuristicNameMatch(const char* name, const char* hint);
};

#endif
