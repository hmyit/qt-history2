/****************************************************************************
**
** Implementation of QMouseDriverFactory class.
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the sql module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qmousedriverinterface_p.h" // up here for GCC 2.7.* compatibility
#include "qmousedriverfactory_qws.h"

#include "qapplication.h"
#include "qmousepc_qws.h"
#include "qmousebus_qws.h"
#include "qmousevr41xx_qws.h"
#include "qmouseyopy_qws.h"
#include "qmouselinuxtp_qws.h"
#include <stdlib.h>

#if !defined(Q_OS_WIN32) || defined(QT_MAKEDLL)
#include <private/qpluginmanager_p.h>
#ifndef QT_NO_COMPONENT
class QMouseDriverFactoryPrivate : public QObject
{
public:
    QMouseDriverFactoryPrivate();
    ~QMouseDriverFactoryPrivate();

    static QPluginManager<QMouseDriverInterface> *manager;
};

static QMouseDriverFactoryPrivate *instance = 0;
QPluginManager<QMouseDriverInterface> *QMouseDriverFactoryPrivate::manager = 0;

QMouseDriverFactoryPrivate::QMouseDriverFactoryPrivate()
: QObject(qApp)
{
    manager = new QPluginManager<QMouseDriverInterface>(IID_QMouseDriver, QApplication::libraryPaths(), "/mousedrivers", false);
}

QMouseDriverFactoryPrivate::~QMouseDriverFactoryPrivate()
{
    delete manager;
    manager = 0;

    instance = 0;
}

#endif //QT_NO_COMPONENT
#endif //QT_MAKEDLL

/*!
    \class QMouseDriverFactory qmousedriverfactory.h
    \brief The QMouseDriverFactory class creates QWSMouseHandler
    objects for Qt/Embedded.

    The graphics driver factory creates a QWSMouseHandler object for a
    given key with QMouseDriverFactory::create(key).

    The drivers are either built-in or dynamically loaded from a
    driver plugin (see \l QMouseDriverPlugin).

    This class is only available in Qt/Embedded.

    QMouseDriverFactory::keys() returns a list of valid keys.
*/

/*!
    Creates a QWSMouseHandler object that matches \a key and uses
    device \a device. This is either a built-in driver, or a driver
    from a driver plugin.

    \sa keys()
*/
QWSMouseHandler *QMouseDriverFactory::create(const QString& key, const QString &device)
{
    QString driver = key.lower();
#ifdef Q_OS_QNX6
    if (driver == "qnx" || driver.isEmpty())
        return new QWSQnxMouseHandler(key, device);
#endif
#ifndef QT_NO_QWS_MOUSE_LINUXTP
    if (driver == "linuxtp" || driver.isEmpty())
        return new QWSLinuxTPMouseHandler(key, device);
#endif
#ifndef QT_NO_QWS_MOUSE_YOPY
    if (driver == "yopy" || driver.isEmpty())
        return new QWSYopyMouseHandler(key, device);
#endif
#ifndef QT_NO_QWS_MOUSE_VR41
    if (driver == "vr41xx" || driver.isEmpty())
        return new QWSVr41xxMouseHandler(key, device);
#endif
#ifndef QT_NO_QWS_MOUSE_PC
    if (driver == "auto" || driver == "intellimouse" ||
         driver == "microsoft" || driver == "mousesystems" ||
         driver == "mouseman" || driver.isEmpty()) {
        qDebug("Creating mouse: %s", key.latin1());
        return new QWSPcMouseHandler(key, device);
    }
#endif
#ifndef QT_NO_QWS_MOUSE_BUS
    if (driver == "bus")
        return new QWSBusMouseHandler(key, device);
#endif

#if !defined(Q_OS_WIN32) || defined(QT_MAKEDLL)
#ifndef QT_NO_COMPONENT
    if (!instance)
        instance = new QMouseDriverFactoryPrivate;

    QInterfacePtr<QMouseDriverInterface> iface;
    QMouseDriverFactoryPrivate::manager->queryInterface(driver, &iface);

    if (iface)
        return iface->create(driver, device);
#endif
#endif
    return 0;
}

/*!
    Returns the list of keys this factory can create drivers for.

    \sa create()
*/
QStringList QMouseDriverFactory::keys()
{
    QStringList list;

#ifdef Q_OS_QNX6
    if (!list.contains("Qnx"))
        list << "Qnx";
#endif
#ifndef QT_NO_QWS_MOUSE_LINUXTP
    if (!list.contains("LinuxTP"))
        list << "LinuxTP";
#endif
#ifndef QT_NO_QWS_MOUSE_YOPY
    if (!list.contains("Yopy"))
        list << "Yopy";
#endif
#ifndef QT_NO_QWS_MOUSE_VR41
    if (!list.contains("VR41xx"))
        list << "VR41xx";
#endif
#ifndef QT_NO_QWS_MOUSE_PC
    if (!list.contains("Auto"))
        list << "Auto";
    if (!list.contains("IntelliMouse"))
        list << "IntelliMouse";
    if (!list.contains("Microsoft"))
        list << "Microsoft";
    if (!list.contains("MouseSystems"))
        list << "MouseSystems";
    if (!list.contains("MouseMan"))
        list << "MouseMan";
#endif
#ifndef QT_NO_QWS_MOUSE_BUS
    if (!list.contains("Bus"))
        list << "Bus";
#endif

#if !defined(Q_OS_WIN32) || defined(QT_MAKEDLL)
#ifndef QT_NO_COMPONENT
    if (!instance)
        instance = new QMouseDriverFactoryPrivate;

    list += QMouseDriverFactoryPrivate::manager->featureList();
#endif //QT_NO_COMPONENT
#endif //QT_MAKEDLL

    return list;
}
