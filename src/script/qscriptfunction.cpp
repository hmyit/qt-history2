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

#include "qscriptfunction_p.h"

#ifndef QT_NO_SCRIPT

#include "qscriptengine_p.h"
#include "qscriptvalueimpl_p.h"
#include "qscriptcontext_p.h"
#include "qscriptmember_p.h"
#include "qscriptobject_p.h"

QT_BEGIN_NAMESPACE

QScriptFunction::~QScriptFunction()
{
}

QString QScriptFunction::toString(QScriptContextPrivate *) const
{
    QString result;
    result += QLatin1String("function () { [native] }");
    return result;
}

QString QScriptFunction::fileName() const
{
    return QString();
}

QString QScriptFunction::functionName() const
{
    return QString();
}

int QScriptFunction::startLineNumber() const
{
    return -1;
}

int QScriptFunction::endLineNumber() const
{
    return -1;
}

void QScriptFunction::mark(QScriptEnginePrivate *engine, int generation)
{
    for (int i = 0; i < formals.count(); ++i)
        engine->markString(formals.at(i), generation);
}

// public API function
void QScript::CFunction::execute(QScriptContextPrivate *context)
{
    QScriptEngine *eng = context->engine();
    QScriptEnginePrivate *eng_p = QScriptEnginePrivate::get(eng);

    eng_p->newUndefined(&context->m_result);

#ifndef Q_SCRIPT_NO_EVENT_NOTIFY
    eng_p->notifyFunctionEntry(context);
#endif

    QScriptValueImpl result = QScriptValuePrivate::valueOf((*m_funPtr)(eng->currentContext(), eng));
    if (result.isValid())
        context->m_result = result;

#ifndef Q_SCRIPT_NO_EVENT_NOTIFY
    eng_p->notifyFunctionExit(context);
#endif
}

QString QScript::CFunction::functionName() const
{
    return QString();
}

// internal API function
void QScript::C2Function::execute(QScriptContextPrivate *context)
{
    QScriptEngine *eng = context->engine();
    QScriptEnginePrivate *eng_p = QScriptEnginePrivate::get(eng);

    bool blocked = eng_p->blockGC(true);

#ifndef Q_SCRIPT_NO_EVENT_NOTIFY
    eng_p->notifyFunctionEntry(context);
#endif

    context->m_result = (*m_funPtr)(context, eng_p, m_classInfo);
    Q_ASSERT(context->m_result.isValid());

#ifndef Q_SCRIPT_NO_EVENT_NOTIFY
    eng_p->notifyFunctionExit(context);
#endif

    eng_p->blockGC(blocked);
}

QString QScript::C2Function::functionName() const
{
    if (!m_name.isEmpty())
        return m_name;
    return QString();
}

QT_END_NAMESPACE

#endif // QT_NO_SCRIPT
