#ifndef EXTENSION_H
#define EXTENSION_H

#include <QString>
#include <QObject>

#define Q_TYPEID(IFace) QLatin1String(IFace##_iid)

struct Extensible
{
    virtual ~Extensible() {}

    virtual QObject *extension(const QString &iid) const = 0;
};
Q_DECLARE_INTERFACE(Extensible, "http://trolltech.com/Qt/Extensible")

struct ExtensionFactory
{
    virtual ~ExtensionFactory() {}

    virtual QObject *extension(QObject *object, const QString &iid) const = 0;
};
Q_DECLARE_INTERFACE(ExtensionFactory, "http://trolltech.com/Qt/ExtensionFactory")

struct ExtensionManager
{
    virtual ~ExtensionManager() {}

    virtual void registerExtensions(ExtensionFactory *factory, const QString &iid) = 0;
    virtual void unregisterExtensions(ExtensionFactory *factory, const QString &iid) = 0;

    virtual QObject *extension(QObject *object, const QString &iid) const = 0;
};
Q_DECLARE_INTERFACE(ExtensionManager, "http://trolltech.com/Qt/ExtensionManager")

template <class T>
inline T qt_extension(ExtensionManager* manager, QObject *object)
{ return 0; }

#define Q_DECLARE_EXTENSION_INTERFACE(IFace, IId) \
Q_DECLARE_INTERFACE(IFace, IId) \
template <> inline IFace *qt_extension<IFace *>(ExtensionManager *manager, QObject *object) \
{ QObject *extension = manager->extension(object, IFace##_iid); return (IFace *)(extension ? extension->qt_metacast(IFace##_iid) : 0); }

#endif // EXTENSION_H
