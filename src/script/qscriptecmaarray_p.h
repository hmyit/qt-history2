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

#ifndef QSCRIPTECMAARRAY_P_H
#define QSCRIPTECMAARRAY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qscriptarray_p.h"

#ifndef QT_NO_SCRIPT

#include "qscriptecmacore_p.h"

QT_BEGIN_NAMESPACE

namespace QScript { namespace Ecma {

class Array: public Core
{
public:
    Array(QScriptEnginePrivate *engine);
    virtual ~Array();

    virtual void execute(QScriptContextPrivate *context);

    class Instance: public QScriptObjectData {
    public:
        Instance() {}
        virtual ~Instance() {}

        static Instance *get(const QScriptValueImpl &object,
                             QScriptClassInfo *klass);

    public: // attributes
        QScript::Array value;
    };

    inline Instance *get(const QScriptValueImpl &object) const
    { return Instance::get(object, classInfo()); }

    void newArray(QScriptValueImpl *result,
                  const QScript::Array &value = QScript::Array());

protected:
    static QScriptValueImpl method_toString(QScriptContextPrivate *context,
                                            QScriptEnginePrivate *eng,
                                            QScriptClassInfo *classInfo);
    static QScriptValueImpl method_toLocaleString(QScriptContextPrivate *context,
                                                  QScriptEnginePrivate *eng,
                                                  QScriptClassInfo *classInfo);
    static QScriptValueImpl method_concat(QScriptContextPrivate *context,
                                          QScriptEnginePrivate *eng,
                                          QScriptClassInfo *classInfo);
    static QScriptValueImpl method_join(QScriptContextPrivate *context,
                                        QScriptEnginePrivate *eng,
                                        QScriptClassInfo *classInfo);
    static QScriptValueImpl method_pop(QScriptContextPrivate *context,
                                       QScriptEnginePrivate *eng,
                                       QScriptClassInfo *classInfo);
    static QScriptValueImpl method_push(QScriptContextPrivate *context,
                                        QScriptEnginePrivate *eng,
                                        QScriptClassInfo *classInfo);
    static QScriptValueImpl method_reverse(QScriptContextPrivate *context,
                                           QScriptEnginePrivate *eng,
                                           QScriptClassInfo *classInfo);
    static QScriptValueImpl method_shift(QScriptContextPrivate *context,
                                         QScriptEnginePrivate *eng,
                                         QScriptClassInfo *classInfo);
    static QScriptValueImpl method_slice(QScriptContextPrivate *context,
                                         QScriptEnginePrivate *eng,
                                         QScriptClassInfo *classInfo);
    static QScriptValueImpl method_sort(QScriptContextPrivate *context,
                                        QScriptEnginePrivate *eng,
                                        QScriptClassInfo *classInfo);
    static QScriptValueImpl method_splice(QScriptContextPrivate *context,
                                          QScriptEnginePrivate *eng,
                                          QScriptClassInfo *classInfo);
    static QScriptValueImpl method_unshift(QScriptContextPrivate *context,
                                           QScriptEnginePrivate *eng,
                                           QScriptClassInfo *classInfo);
};

} } // namespace QScript::Ecma

#endif // QT_NO_SCRIPT

QT_END_NAMESPACE

#endif
