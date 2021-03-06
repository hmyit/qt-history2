/****************************************************************************
 * **
 * ** Copyright (C) 2007-$THISYEAR$ $TROLLTECH$. All rights reserved.
 * **
 * ** This file is part of the $MODULE$ of the Qt Toolkit.
 * **
 * ** $TROLLTECH_DUAL_LICENSE$
 * **
 * ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * **
 * ****************************************************************************/

#ifndef QABSTRACTMESSAGEHANDLER_H
#define QABSTRACTMESSAGEHANDLER_H

#include <QtXml/QSourceLocation>
#include <QtCore/QSharedData>
#include <QtCore/QtGlobal>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Xml)

class QAbstractMessageHandlerPrivate;
class Q_XML_EXPORT QAbstractMessageHandler : public QSharedData
{
public:
    typedef QExplicitlySharedDataPointer<QAbstractMessageHandler> Ptr;

    QAbstractMessageHandler();
    virtual ~QAbstractMessageHandler();

    void message(QtMsgType type,
                 const QString &description,
                 const QUrl &identifier = QUrl(),
                 const QSourceLocation &sourceLocation = QSourceLocation());

protected:
    virtual void handleMessage(QtMsgType type,
                               const QString &description,
                               const QUrl &identifier,
                               const QSourceLocation &sourceLocation) = 0;

    QAbstractMessageHandlerPrivate *d;
private:
    Q_DISABLE_COPY(QAbstractMessageHandler)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
// vim: et:ts=4:sw=4:sts=4
