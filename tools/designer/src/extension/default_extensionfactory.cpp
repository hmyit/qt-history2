#include "default_extensionfactory.h"
#include "qextensionmanager.h"
#include <qpointer.h>
#include <qdebug.h>

DefaultExtensionFactory::DefaultExtensionFactory(QExtensionManager *parent)
    : QObject(parent)
{
}

QObject *DefaultExtensionFactory::extension(QObject *object, const QString &iid) const
{
    if (!object)
        return 0;

    QPair<QString, QObject*> key = qMakePair(iid, object);
    if (!m_extensions.contains(key)) {
        if (QObject *ext = createExtension(object, iid, const_cast<DefaultExtensionFactory*>(this))) {
            m_extensions.insert(key, ext);
        }
    }

    if (!m_extended.contains(object)) {
        connect(object, SIGNAL(destroyed(QObject*)), this, SLOT(objectDestroyed(QObject*)));
        m_extended.insert(object, true);
    }

    return m_extensions.value(key);
}

void DefaultExtensionFactory::objectDestroyed(QObject *object)
{
    QMapMutableIterator< QPair<QString,QObject*>, QObject*> it(m_extensions);
    while (it.hasNext()) {
        it.next();

        QPointer<QObject> o = it.key().second;
        if (!o || o == object) {
            it.remove();
        }
    }

    m_extended.remove(object);
}

QObject *DefaultExtensionFactory::createExtension(QObject *object, const QString &iid, QObject *parent) const
{
    Q_UNUSED(object);
    Q_UNUSED(iid);
    Q_UNUSED(parent);

    return 0;
}

QExtensionManager *DefaultExtensionFactory::extensionManager() const
{
    return static_cast<QExtensionManager *>(parent());
}
