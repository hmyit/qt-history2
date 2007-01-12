/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtTest/QtTest>

#include <QtScript/qscriptvalue.h>
#include <QtScript/qscriptengine.h>

//TESTED_CLASS=
//TESTED_FILES=qscriptvalue.h qscriptvalue.cpp

class tst_QScriptValue : public QObject
{
    Q_OBJECT

public:
    tst_QScriptValue();
    virtual ~tst_QScriptValue();

private slots:
    void ctor();
    void invalidate();
    void engine();
    void toString();
    void toNumber();
    void toBoolean();
    void toInteger();
    void toInt32();
    void toUInt32();
    void toUInt16();
    void toVariant();
    void toQObject();
    void toObject();
    void toPrimitive();
    void instanceOf();
    void getSetProperty();
    void getSetPrototype();
    void call();
    // isXXX functions are tested in qscriptengine tests.
};

tst_QScriptValue::tst_QScriptValue()
{
}

tst_QScriptValue::~tst_QScriptValue()
{
}

void tst_QScriptValue::ctor()
{
    QScriptValue v;
    QCOMPARE(v.isValid(), false);
    QCOMPARE(v.engine(), (QScriptEngine *)0);
}

void tst_QScriptValue::invalidate()
{
    QScriptEngine eng;
    QScriptValue object = eng.newObject();
    object.invalidate();
    QCOMPARE(object.isObject(), false);
    QCOMPARE(object.isValid(), false);
}

void tst_QScriptValue::engine()
{
    QScriptEngine eng;
    QScriptValue object = eng.newObject();
    QCOMPARE(object.engine(), &eng);
}

static QScriptValue myFunction(QScriptContext *, QScriptEngine *eng)
{
    return eng->undefinedScriptValue();
}

void tst_QScriptValue::toString()
{
    QScriptEngine eng;

    QScriptValue undefined = eng.undefinedScriptValue();
    QCOMPARE(undefined.toString(), QString("undefined"));

    QScriptValue null = eng.nullScriptValue();
    QCOMPARE(null.toString(), QString("null"));

    QScriptValue falskt = eng.scriptValue(false);
    QCOMPARE(falskt.toString(), QString("false"));

    QScriptValue sant = eng.scriptValue(true);
    QCOMPARE(sant.toString(), QString("true"));

    QScriptValue number = eng.scriptValue(123);
    QCOMPARE(number.toString(), QString("123"));

    QScriptValue str = eng.scriptValue(QString("ciao"));
    QCOMPARE(str.toString(), QString("ciao"));

    QScriptValue object = eng.newObject();
    QCOMPARE(object.toString(), QString("[object Object]"));

    QScriptValue fun = eng.scriptValue(myFunction);
    QCOMPARE(fun.toString(), QString("function () { [native] }"));

    QScriptValue inv = QScriptValue();
    QCOMPARE(inv.toString(), QString());
}

void tst_QScriptValue::toNumber()
{
    QScriptEngine eng;

    QScriptValue undefined = eng.undefinedScriptValue();
    QCOMPARE(qIsNan(undefined.toNumber()), true);

    QScriptValue null = eng.nullScriptValue();
    QCOMPARE(null.toNumber(), 0.0);

    QScriptValue falskt = eng.scriptValue(false);
    QCOMPARE(falskt.toNumber(), 0.0);

    QScriptValue sant = eng.scriptValue(true);
    QCOMPARE(sant.toNumber(), 1.0);

    QScriptValue number = eng.scriptValue(123.0);
    QCOMPARE(number.toNumber(), 123.0);

    QScriptValue str = eng.scriptValue(QString("ciao"));
    QCOMPARE(qIsNan(str.toNumber()), true);

    QScriptValue str2 = eng.scriptValue(QString("123"));
    QCOMPARE(str2.toNumber(), 123.0);

    QScriptValue object = eng.newObject();
    QCOMPARE(qIsNan(object.toNumber()), true);

    QScriptValue fun = eng.scriptValue(myFunction);
    QCOMPARE(qIsNan(fun.toNumber()), true);

    QScriptValue inv = QScriptValue();
    QCOMPARE(inv.toNumber(), 0.0);
}

void tst_QScriptValue::toBoolean()
{
    QScriptEngine eng;

    QScriptValue undefined = eng.undefinedScriptValue();
    QCOMPARE(undefined.toBoolean(), false);

    QScriptValue null = eng.nullScriptValue();
    QCOMPARE(null.toBoolean(), false);

    QScriptValue falskt = eng.scriptValue(false);
    QCOMPARE(falskt.toBoolean(), false);

    QScriptValue sant = eng.scriptValue(true);
    QCOMPARE(sant.toBoolean(), true);

    QScriptValue number = eng.scriptValue(0.0);
    QCOMPARE(number.toBoolean(), false);

    QScriptValue number2 = eng.scriptValue(qSNan());
    QCOMPARE(number2.toBoolean(), false);

    QScriptValue number3 = eng.scriptValue(123.0);
    QCOMPARE(number3.toBoolean(), true);

    QScriptValue number4 = eng.scriptValue(-456.0);
    QCOMPARE(number4.toBoolean(), true);

    QScriptValue str = eng.scriptValue(QString(""));
    QCOMPARE(str.toBoolean(), false);

    QScriptValue str2 = eng.scriptValue(QString("123"));
    QCOMPARE(str2.toBoolean(), true);

    QScriptValue object = eng.newObject();
    QCOMPARE(object.toBoolean(), true);

    QScriptValue fun = eng.scriptValue(myFunction);
    QCOMPARE(fun.toBoolean(), true);

    QScriptValue inv = QScriptValue();
    QCOMPARE(inv.toBoolean(), false);
}

void tst_QScriptValue::toInteger()
{
    QScriptEngine eng;

    QScriptValue number = eng.scriptValue(123.0);
    QCOMPARE(number.toInteger(), 123.0);

    QScriptValue number2 = eng.scriptValue(qSNan());
    QCOMPARE(number2.toInteger(), 0.0);

    QScriptValue number3 = eng.scriptValue(qInf());
    QCOMPARE(qIsInf(number3.toInteger()), true);

    QScriptValue number4 = eng.scriptValue(0.5);
    QCOMPARE(number4.toInteger(), 0.0);

    QScriptValue number5 = eng.scriptValue(123.5);
    QCOMPARE(number5.toInteger(), 123.0);

    QScriptValue number6 = eng.scriptValue(-456.5);
    QCOMPARE(number6.toInteger(), -456.0);

    QScriptValue inv;
    QCOMPARE(inv.toInteger(), 0.0);
}

void tst_QScriptValue::toInt32()
{
    QScriptEngine eng;

    QScriptValue number = eng.scriptValue(123.0);
    QCOMPARE(number.toInt32(), 123);

    QScriptValue number2 = eng.scriptValue(qSNan());
    QCOMPARE(number2.toInt32(), 0);

    QScriptValue number3 = eng.scriptValue(qInf());
    QCOMPARE(number3.toInt32(), 0);

    QScriptValue number4 = eng.scriptValue(0.5);
    QCOMPARE(number4.toInt32(), 0);

    QScriptValue number5 = eng.scriptValue(123.5);
    QCOMPARE(number5.toInt32(), 123);

    QScriptValue number6 = eng.scriptValue(-456.5);
    QCOMPARE(number6.toInt32(), -456);

    QScriptValue number7 = eng.scriptValue(Q_INT64_C(0x100000000));
    QCOMPARE(number7.toInt32(), 0);

    QScriptValue number8 = eng.scriptValue(Q_INT64_C(0x100000001));
    QCOMPARE(number8.toInt32(), 1);

    QScriptValue inv;
    QCOMPARE(inv.toInt32(), 0);
}

void tst_QScriptValue::toUInt32()
{
    QScriptEngine eng;

    QScriptValue number = eng.scriptValue(123.0);
    QCOMPARE(number.toUInt32(), quint32(123));

    QScriptValue number2 = eng.scriptValue(qSNan());
    QCOMPARE(number2.toUInt32(), quint32(0));

    QScriptValue number3 = eng.scriptValue(qInf());
    QCOMPARE(number3.toUInt32(), quint32(0));

    QScriptValue number4 = eng.scriptValue(0.5);
    QEXPECT_FAIL("", "toUInt32() should floor(), not round()", Continue);
    QCOMPARE(number4.toUInt32(), quint32(0));

    QScriptValue number5 = eng.scriptValue(123.5);
    QEXPECT_FAIL("", "toUInt32() should floor(), not round()", Continue);
    QCOMPARE(number5.toUInt32(), quint32(123));

    QScriptValue number6 = eng.scriptValue(-456.5);
    QCOMPARE(number6.toUInt32(), quint32(-456));

    QScriptValue number7 = eng.scriptValue(Q_INT64_C(0x100000000));
    QCOMPARE(number7.toUInt32(), quint32(0));

    QScriptValue number8 = eng.scriptValue(Q_INT64_C(0x100000001));
    QCOMPARE(number8.toUInt32(), quint32(1));

    QScriptValue inv;
    QCOMPARE(inv.toUInt32(), quint32(0));
}

void tst_QScriptValue::toUInt16()
{
    QScriptEngine eng;

    QScriptValue number = eng.scriptValue(123.0);
    QCOMPARE(number.toUInt16(), quint16(123));

    QScriptValue number2 = eng.scriptValue(qSNan());
    QCOMPARE(number2.toUInt16(), quint16(0));

    QScriptValue number3 = eng.scriptValue(qInf());
    QCOMPARE(number3.toUInt16(), quint16(0));

    QScriptValue number4 = eng.scriptValue(0.5);
    QEXPECT_FAIL("", "toUInt16() should floor(), not round()", Continue);
    QCOMPARE(number4.toUInt16(), quint16(0));

    QScriptValue number5 = eng.scriptValue(123.5);
    QEXPECT_FAIL("", "toUInt32() should floor(), not round()", Continue);
    QCOMPARE(number5.toUInt16(), quint16(123));

    QScriptValue number6 = eng.scriptValue(-456.5);
    QCOMPARE(number6.toUInt16(), quint16(-456));

    QScriptValue number7 = eng.scriptValue(0x10000);
    QCOMPARE(number7.toUInt16(), quint16(0));

    QScriptValue number8 = eng.scriptValue(0x10001);
    QCOMPARE(number8.toUInt16(), quint16(1));

    QScriptValue inv;
    QCOMPARE(inv.toUInt16(), quint16(0));
}

void tst_QScriptValue::toVariant()
{
    QScriptEngine eng;

    QScriptValue undefined = eng.undefinedScriptValue();
    QCOMPARE(undefined.toVariant(), QVariant());

    QScriptValue null = eng.nullScriptValue();
    QCOMPARE(null.toVariant(), QVariant(0));

    QScriptValue number = eng.scriptValue(123.0);
    QCOMPARE(number.toVariant(), QVariant(123.0));

    QScriptValue falskt = eng.scriptValue(false);
    QCOMPARE(falskt.toVariant(), QVariant(false));

    QScriptValue sant = eng.scriptValue(true);
    QCOMPARE(sant.toVariant(), QVariant(true));

    QScriptValue str = eng.scriptValue(QString("ciao"));
    QCOMPARE(str.toVariant(), QVariant(QString("ciao")));

    QVariant var(QChar(0x007A));
    QScriptValue opaque = eng.scriptValueFromVariant(var);
    QCOMPARE(opaque.toVariant(), var);

    QScriptValue object = eng.newObject();
    QCOMPARE(object.toVariant(), QVariant(QString("[object Object]")));

    QScriptValue qobject = eng.scriptValueFromQObject(0);
    QCOMPARE(qobject.toVariant(), QVariant((QObject*)0));

    QScriptValue inv;
    QCOMPARE(inv.toVariant(), QVariant());
}

void tst_QScriptValue::toQObject()
{
    QScriptEngine eng;

    QScriptValue undefined = eng.undefinedScriptValue();
    QCOMPARE(undefined.toQObject(), (QObject *)0);

    QScriptValue null = eng.nullScriptValue();
    QCOMPARE(null.toQObject(), (QObject *)0);

    QScriptValue falskt = eng.scriptValue(false);
    QCOMPARE(falskt.toQObject(), (QObject *)0);

    QScriptValue sant = eng.scriptValue(true);
    QCOMPARE(sant.toQObject(), (QObject *)0);

    QScriptValue number = eng.scriptValue(123.0);
    QCOMPARE(number.toQObject(), (QObject *)0);

    QScriptValue str = eng.scriptValue(QString("ciao"));
    QCOMPARE(str.toQObject(), (QObject *)0);

    QScriptValue object = eng.newObject();
    QCOMPARE(object.toQObject(), (QObject *)0);

    QScriptValue qobject = eng.scriptValueFromQObject(this);
    QCOMPARE(qobject.toQObject(), this);

    QScriptValue qobject2 = eng.scriptValueFromQObject(0);
    QCOMPARE(qobject2.toQObject(), (QObject *)0);

    QScriptValue inv;
    QCOMPARE(inv.toQObject(), (QObject *)0);
}

void tst_QScriptValue::toObject()
{
    QScriptEngine eng;

    QScriptValue undefined = eng.undefinedScriptValue();
    QCOMPARE(undefined.toObject().isValid(), false);

    QScriptValue null = eng.nullScriptValue();
    QCOMPARE(null.toObject().isValid(), false);

    QScriptValue falskt = eng.scriptValue(false);
    {
        QScriptValue tmp = falskt.toObject();
        QCOMPARE(tmp.isObject(), true);
        QCOMPARE(tmp.toNumber(), falskt.toNumber());
    }

    QScriptValue sant = eng.scriptValue(true);
    {
        QScriptValue tmp = sant.toObject();
        QCOMPARE(tmp.isObject(), true);
        QCOMPARE(tmp.toNumber(), sant.toNumber());
    }

    QScriptValue number = eng.scriptValue(123.0);
    {
        QScriptValue tmp = number.toObject();
        QCOMPARE(tmp.isObject(), true);
        QCOMPARE(tmp.toNumber(), number.toNumber());
    }

    QScriptValue str = eng.scriptValue(QString("ciao"));
    {
        QScriptValue tmp = str.toObject();
        QCOMPARE(tmp.isObject(), true);
        QCOMPARE(tmp.toString(), str.toString());
    }

    QScriptValue object = eng.newObject();
    {
        QScriptValue tmp = object.toObject();
        QCOMPARE(tmp.isObject(), true);
    }

    QScriptValue qobject = eng.scriptValueFromQObject(this);
    QCOMPARE(qobject.toObject().isValid(), true);

    QScriptValue inv;
    QCOMPARE(inv.toObject().isValid(), false);
}

void tst_QScriptValue::toPrimitive()
{
    QScriptEngine eng;

    QScriptValue undefined = eng.undefinedScriptValue();
    {
        QScriptValue tmp;
        tmp = undefined.toPrimitive(QScriptValue::NoTypeHint);
        QCOMPARE(tmp.isUndefined(), true);
        tmp = undefined.toPrimitive(QScriptValue::NumberTypeHint);
        QCOMPARE(tmp.isUndefined(), true);
        tmp = undefined.toPrimitive(QScriptValue::StringTypeHint);
        QCOMPARE(tmp.isUndefined(), true);
    }

    QScriptValue null = eng.nullScriptValue();
    {
        QScriptValue tmp;
        tmp = null.toPrimitive(QScriptValue::NoTypeHint);
        QCOMPARE(tmp.isNull(), true);
        tmp = null.toPrimitive(QScriptValue::NumberTypeHint);
        QCOMPARE(tmp.isNull(), true);
        tmp = null.toPrimitive(QScriptValue::StringTypeHint);
        QCOMPARE(tmp.isNull(), true);
    }

    QScriptValue number = eng.scriptValue(123.0);
    {
        QScriptValue tmp;
        tmp = number.toPrimitive(QScriptValue::NoTypeHint);
        QCOMPARE(tmp.isNumber(), true);
        QCOMPARE(tmp.toNumber(), number.toNumber());
        tmp = number.toPrimitive(QScriptValue::NumberTypeHint);
        QCOMPARE(tmp.isNumber(), true);
        QCOMPARE(tmp.toNumber(), number.toNumber());
        tmp = number.toPrimitive(QScriptValue::StringTypeHint);
        QCOMPARE(tmp.isNumber(), true);
        QCOMPARE(tmp.toNumber(), number.toNumber());
    }

    QScriptValue falskt = eng.scriptValue(false);
    {
        QScriptValue tmp;
        tmp = falskt.toPrimitive(QScriptValue::NoTypeHint);
        QCOMPARE(tmp.isBoolean(), true);
        QCOMPARE(tmp.toBoolean(), false);
        tmp = falskt.toPrimitive(QScriptValue::NumberTypeHint);
        QCOMPARE(tmp.isBoolean(), true);
        QCOMPARE(tmp.toBoolean(), false);
        tmp = falskt.toPrimitive(QScriptValue::StringTypeHint);
        QCOMPARE(tmp.isBoolean(), true);
        QCOMPARE(tmp.toBoolean(), false);
    }

    QScriptValue sant = eng.scriptValue(true);
    {
        QScriptValue tmp;
        tmp = sant.toPrimitive(QScriptValue::NoTypeHint);
        QCOMPARE(tmp.isBoolean(), true);
        QCOMPARE(tmp.toBoolean(), true);
        tmp = sant.toPrimitive(QScriptValue::NumberTypeHint);
        QCOMPARE(tmp.isBoolean(), true);
        QCOMPARE(tmp.toBoolean(), true);
        tmp = sant.toPrimitive(QScriptValue::StringTypeHint);
        QCOMPARE(tmp.isBoolean(), true);
        QCOMPARE(tmp.toBoolean(), true);
    }

    QScriptValue str = eng.scriptValue(QString("ciao"));
    {
        QScriptValue tmp;
        tmp = str.toPrimitive(QScriptValue::NoTypeHint);
        QCOMPARE(tmp.isString(), true);
        QCOMPARE(tmp.toString(), str.toString());
        tmp = str.toPrimitive(QScriptValue::NumberTypeHint);
        QCOMPARE(tmp.isString(), true);
        QCOMPARE(tmp.toString(), str.toString());
        tmp = str.toPrimitive(QScriptValue::StringTypeHint);
        QCOMPARE(tmp.isString(), true);
        QCOMPARE(tmp.toString(), str.toString());
    }

    QScriptValue numberObject = eng.evaluate("new Number(123)");
    QCOMPARE(numberObject.isObject(), true);
    {
        QScriptValue tmp;
        tmp = numberObject.toPrimitive(QScriptValue::NoTypeHint);
        QCOMPARE(tmp.isNumber(), true);
        QCOMPARE(tmp.toNumber(), number.toNumber());
        tmp = numberObject.toPrimitive(QScriptValue::NumberTypeHint);
        QCOMPARE(tmp.isNumber(), true);
        QCOMPARE(tmp.toNumber(), number.toNumber());
        tmp = numberObject.toPrimitive(QScriptValue::StringTypeHint);
        QCOMPARE(tmp.isString(), true);
        QCOMPARE(tmp.toString(), QString("123"));
    }

    QScriptValue booleanObject = eng.evaluate("new Boolean(false)");
    QCOMPARE(booleanObject.isObject(), true);
    {
        QScriptValue tmp;
        tmp = booleanObject.toPrimitive(QScriptValue::NoTypeHint);
        QCOMPARE(tmp.isBoolean(), true);
        QCOMPARE(tmp.toBoolean(), false);
        tmp = booleanObject.toPrimitive(QScriptValue::NumberTypeHint);
        QCOMPARE(tmp.isBoolean(), true);
        QCOMPARE(tmp.toBoolean(), false);
        tmp = booleanObject.toPrimitive(QScriptValue::StringTypeHint);
        QCOMPARE(tmp.isString(), true);
        QCOMPARE(tmp.toString(), QString("false"));
    }


    QScriptValue stringObject = eng.evaluate("new String(\"ciao\")");
    QCOMPARE(stringObject.isObject(), true);
    {
        QScriptValue tmp;
        tmp = stringObject.toPrimitive(QScriptValue::NoTypeHint);
        QCOMPARE(tmp.isString(), true);
        QCOMPARE(tmp.toString(), str.toString());
        tmp = stringObject.toPrimitive(QScriptValue::NumberTypeHint);
        QCOMPARE(tmp.isString(), true);
        QCOMPARE(tmp.toString(), str.toString());
        tmp = stringObject.toPrimitive(QScriptValue::StringTypeHint);
        QCOMPARE(tmp.isString(), true);
        QCOMPARE(tmp.toString(), str.toString());
    }

    QScriptValue inv;
    QCOMPARE(inv.toPrimitive().isValid(), false);
}

void tst_QScriptValue::instanceOf()
{
    QEXPECT_FAIL("", "implement me", Continue);
    QCOMPARE(0, 1);
}

void tst_QScriptValue::getSetProperty()
{
    QScriptEngine eng;

    QScriptValue object = eng.newObject();

    QScriptValue str = eng.scriptValue("bar");
    object.setProperty("foo", str);
    QCOMPARE(object.property("foo").toString(), str.toString());

    QScriptNameId fooId = eng.nameId("foo");
    QCOMPARE(object.property(fooId).toString(), str.toString());

    QScriptValue num = eng.scriptValue(123.0);
    object.setProperty("baz", num);
    QCOMPARE(object.property("baz").toNumber(), num.toNumber());

    QScriptNameId bazId = eng.nameId("baz");
    QCOMPARE(object.property(bazId).toNumber(), num.toNumber());

    QScriptValue inv;
    inv.setProperty(fooId, num);
    QCOMPARE(inv.property(fooId).isValid(), false);

    QScriptValue array = eng.newArray();
    array.setProperty(0, num);
    QCOMPARE(array.property(0).toNumber(), num.toNumber());
    QCOMPARE(array.property("length").toUInt32(), quint32(1));
    array.setProperty(1, str);
    QCOMPARE(array.property(1).toString(), str.toString());
    QCOMPARE(array.property("length").toUInt32(), quint32(2));
    array.setProperty("length", eng.scriptValue(1));
    QCOMPARE(array.property("length").toUInt32(), quint32(1));
    QCOMPARE(array.property(1).isValid(), false);
}

void tst_QScriptValue::getSetPrototype()
{
    QScriptEngine eng;

    QScriptValue object = eng.newObject();
    object.setProperty("foo", eng.scriptValue("bar"));

    QScriptValue object2 = eng.newObject();
    object2.setPrototype(object);

    QCOMPARE(object2.prototype().isObject(), true);
    QCOMPARE(object2.prototype().property("foo").toString(), object.property("foo").toString());

    QScriptValue inv;
    inv.setPrototype(object);
    QCOMPARE(inv.prototype().isValid(), false);
}

void tst_QScriptValue::call()
{
    QScriptEngine eng;

    QScriptValue Object = eng.evaluate("Object");
    QCOMPARE(Object.isFunction(), true);

    {
        QScriptValue result = Object.call(Object);
        QCOMPARE(result.isObject(), true);
    }

    // test that correct "this" object is used
    {
        QScriptValue fun = eng.evaluate("function() { return this; }");
        QCOMPARE(fun.isFunction(), true);

        {
            QScriptValue numberObject = eng.scriptValue(123.0).toObject();
            QScriptValue result = fun.call(numberObject);
            QCOMPARE(result.isObject(), true);
            QCOMPARE(result.toNumber(), 123.0);
        }
    }

    // test that correct arguments are passed
    {
        QScriptValue fun = eng.evaluate("function() { return arguments[0]; }");
        QCOMPARE(fun.isFunction(), true);

        {
            QScriptValue result = fun.call(eng.undefinedScriptValue());
            QCOMPARE(result.isUndefined(), true);
        }

        {
            QScriptValueList args;
            args << eng.scriptValue(123.0);
            QScriptValue result = fun.call(eng.undefinedScriptValue(), args);
            QCOMPARE(result.isNumber(), true);
            QCOMPARE(result.toNumber(), 123.0);
        }
    }

    {
        QScriptValue fun = eng.evaluate("function() { return arguments[1]; }");
        QCOMPARE(fun.isFunction(), true);

        {
            QScriptValueList args;
            args << eng.scriptValue(123.0) << eng.scriptValue(456.0);
            QScriptValue result = fun.call(eng.undefinedScriptValue(), args);
            QCOMPARE(result.isNumber(), true);
            QCOMPARE(result.toNumber(), 456.0);
        }
    }

    {
        QScriptValue fun = eng.evaluate("function() { throw new Error('foo'); }");
        QCOMPARE(fun.isFunction(), true);

        {
            QScriptValue result = fun.call();
            QCOMPARE(result.isError(), true);
            QCOMPARE(eng.uncaughtException(), true);
        }
    }

    QScriptValue inv;
    QCOMPARE(inv.call().isValid(), false);
}

QTEST_MAIN(tst_QScriptValue)
#include "tst_qscriptvalue.moc"
