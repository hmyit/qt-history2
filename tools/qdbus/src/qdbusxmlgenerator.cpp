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

#include <QtCore/qcoreapplication.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qstringlist.h>

#include "qdbusinterface_p.h"   // for ANNOTATION_NO_WAIT
#include "qdbusabstractadaptor_p.h" // for QCLASSINFO_DBUS_*
#include "qdbusconnection_p.h"  // for the flags
#include "qdbusmetatype_p.h"
#include "qdbusmetatype.h"
#include "qdbusutil_p.h"

extern QDBUS_EXPORT QString qDBusGenerateMetaObjectXml(QString interface, const QMetaObject *mo,
                                                       const QMetaObject *base, int flags);

// implement the D-Bus org.freedesktop.DBus.Introspectable interface
// we do that by analysing the metaObject of all the adaptor interfaces

static QString generateInterfaceXml(const QMetaObject *mo, int flags, int methodOffset, int propOffset)
{
    QString retval;

    // start with properties:
    if (flags & (QDBusConnection::ExportProperties |
                 QDBusConnection::ExportNonScriptableProperties)) {
        for (int i = propOffset; i < mo->propertyCount(); ++i) {
            static const char *accessvalues[] = {0, "read", "write", "readwrite"};

            QMetaProperty mp = mo->property(i);

            if (!((mp.isScriptable() && (flags & QDBusConnection::ExportProperties)) ||
                  (!mp.isScriptable() && (flags & QDBusConnection::ExportNonScriptableProperties))))
                continue;

            int access = 0;
            if (mp.isReadable())
                access |= 1;
            if (mp.isWritable())
                access |= 2;

            int typeId = qDBusNameToTypeId(mp.typeName());
            if (!typeId)
                continue;
            const char *signature = QDBusMetaType::typeToSignature( typeId );
            if (!signature)
                continue;

            retval += QString(QLatin1String("    <property name=\"%1\" type=\"%2\" access=\"%3\""))
                      .arg(QLatin1String( mp.name() ))
                      .arg(QLatin1String( signature ))
                      .arg(QLatin1String( accessvalues[access] ));

            if (QDBusMetaType::signatureToType(signature) == QVariant::Invalid) {
                const char *typeName = QVariant::typeToName( QVariant::Type(typeId) );
                retval += QString::fromLatin1("      <annotation name=\"com.trolltech.QtDBus.QtTypeName\" value=\"%3\"/>\n    </property>\n")
                          .arg(QLatin1String(typeName));
            } else {
                retval += QLatin1String("/>\n");
            }
        }
    }

    // now add methods:
    for (int i = methodOffset; i < mo->methodCount(); ++i) {
        QMetaMethod mm = mo->method(i);
        QByteArray signature = mm.signature();
        int paren = signature.indexOf('(');

        bool isSignal;
        if (mm.methodType() == QMetaMethod::Signal)
            // adding a signal
            isSignal = true;
        else if (mm.methodType() == QMetaMethod::Slot && mm.access() == QMetaMethod::Public)
            isSignal = false;
        else
            continue;           // neither signal nor public slot

        if (isSignal && !(flags & (QDBusConnection::ExportSignals |
                                   QDBusConnection::ExportNonScriptableSignals)))
            continue;           // we're not exporting any signals
        if (!isSignal && !(flags & (QDBusConnection::ExportSlots |
                                    QDBusConnection::ExportNonScriptableSlots)))
            continue;           // we're not exporting any slots

        QString xml = QString(QLatin1String("    <%1 name=\"%2\">\n"))
                      .arg(isSignal ? QLatin1String("signal") : QLatin1String("method"))
                      .arg(QLatin1String(signature.left(paren)));

        // check the return type first
        int typeId = qDBusNameToTypeId(mm.typeName());
        if (typeId) {
            const char *typeName = QDBusMetaType::typeToSignature( typeId );
            if (typeName) {
                xml += QString(QLatin1String("      <arg type=\"%1\" direction=\"out\"/>\n"))
                       .arg(QLatin1String(typeName));

                // do we need to describe this argument?
                if (QDBusMetaType::signatureToType(typeName) == QVariant::Invalid)
                    xml += QString::fromLatin1("      <annotation name=\"com.trolltech.QtDBus.QtTypeName.Out0\" value=\"%1\"/>\n")
                           .arg(QLatin1String(mm.typeName()));
            } else
                continue;
        }
        else if (*mm.typeName())
            continue;           // wasn't a valid type

        QList<QByteArray> names = mm.parameterNames();
        QList<int> types;
        int inputCount = qDBusParametersForMethod(mm, types);
        if (inputCount == -1)
            continue;           // invalid form
        if (isSignal && inputCount + 1 != types.count())
            continue;           // signal with output arguments?
        if (isSignal && types.at(inputCount) == QDBusMetaTypeId::message)
            continue;           // signal with QDBusMessage argument?
        if (isSignal && mm.attributes() & QMetaMethod::Cloned)
            continue;           // cloned signal?

        int j;
        bool isScriptable = mm.attributes() & QMetaMethod::Scriptable;
        for (j = 1; j < types.count(); ++j) {
            // input parameter for a slot or output for a signal
            if (types.at(j) == QDBusMetaTypeId::message) {
                isScriptable = true;
                continue;
            }

            QString name;
            if (!names.at(j - 1).isEmpty())
                name = QString(QLatin1String("name=\"%1\" ")).arg(QLatin1String(names.at(j - 1)));

            bool isOutput = isSignal || j > inputCount;

            const char *signature = QDBusMetaType::typeToSignature( types.at(j) );
            xml += QString(QLatin1String("      <arg %1type=\"%2\" direction=\"%3\"/>\n"))
                   .arg(name)
                   .arg(QLatin1String(signature))
                   .arg(isOutput ? QLatin1String("out") : QLatin1String("in"));

            // do we need to describe this argument?
            if (QDBusMetaType::signatureToType(signature) == QVariant::Invalid) {
                const char *typeName = QVariant::typeToName( QVariant::Type(types.at(j)) );
                xml += QString::fromLatin1("      <annotation name=\"com.trolltech.QtDBus.QtTypeName.%1%2\" value=\"%3\"/>\n")
                       .arg(isOutput ? QLatin1String("Out") : QLatin1String("In"))
                       .arg(isOutput ? j - 1 : j - inputCount)
                       .arg(QLatin1String(typeName));
            }
        }

        int wantedMask;
        if (isScriptable)
            wantedMask = isSignal ? QDBusConnection::ExportSignals : QDBusConnection::ExportSlots;
        else
            wantedMask = isSignal ? QDBusConnection::ExportNonScriptableSignals :
                         QDBusConnection::ExportNonScriptableSlots;
        if ((flags & wantedMask) != wantedMask)
            continue;

        if (qDBusCheckAsyncTag(mm.tag()))
            // add the no-reply annotation
            xml += QLatin1String("      <annotation name=\"" ANNOTATION_NO_WAIT "\""
                                 " value=\"true\"/>\n");

        retval += xml;
        retval += QString(QLatin1String("    </%1>\n"))
                  .arg(isSignal ? QLatin1String("signal") : QLatin1String("method"));
    }

    return retval;
}

QString qDBusGenerateMetaObjectXml(QString interface, const QMetaObject *mo, const QMetaObject *base,
                                   int flags)
{
    if (interface.isEmpty()) {
        // generate the interface name from the meta object
        int idx = mo->indexOfClassInfo(QCLASSINFO_DBUS_INTERFACE);
        if (idx >= mo->classInfoOffset()) {
            interface = QLatin1String(mo->classInfo(idx).value());
        } else {
            interface = QLatin1String(mo->className());
            interface.replace(QLatin1String("::"), QLatin1String("."));

            if (interface.startsWith( QLatin1String("QDBus") )) {
                interface.prepend( QLatin1String("com.trolltech.QtDBus.") );
            } else if (interface.startsWith( QLatin1Char('Q') ) &&
                       interface.length() >= 2 && interface.at(1).isUpper()) {
                // assume it's Qt
                interface.prepend( QLatin1String("com.trolltech.Qt.") );
            } else if (!QCoreApplication::instance() ||
                       QCoreApplication::instance()->applicationName().isEmpty()) {
                interface.prepend( QLatin1String("local.") );
            } else {
                interface.prepend(QLatin1Char('.')).prepend( QCoreApplication::instance()->applicationName() );
                QStringList domainName =
                    QCoreApplication::instance()->organizationDomain().split(QLatin1Char('.'),
                                                                             QString::SkipEmptyParts);
                if (domainName.isEmpty())
                    interface.prepend(QLatin1String("local."));
                else
                    for (int i = 0; i < domainName.count(); ++i)
                        interface.prepend(QLatin1Char('.')).prepend(domainName.at(i));
            }
        }
    }

    QString xml;
    int idx = mo->indexOfClassInfo(QCLASSINFO_DBUS_INTROSPECTION);
    if (idx >= mo->classInfoOffset())
        return QString::fromUtf8(mo->classInfo(idx).value());
    else
        xml = generateInterfaceXml(mo, flags, base->methodCount(), base->propertyCount());

    if (xml.isEmpty())
        return QString();       // don't add an empty interface
    return QString(QLatin1String("  <interface name=\"%1\">\n%2  </interface>\n"))
        .arg(interface, xml);
}
