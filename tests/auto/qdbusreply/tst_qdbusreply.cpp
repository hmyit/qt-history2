#include <qcoreapplication.h>
#include <qdebug.h>

#include <QtTest/QtTest>

#include <QtDBus>

typedef QMap<int,QString> IntStringMap;
Q_DECLARE_METATYPE(IntStringMap)

struct MyStruct
{
    int i;
    QString s;

    MyStruct() : i(1), s("String") { }
    bool operator==(const MyStruct &other) const
    { return i == other.i && s == other.s; }
};
Q_DECLARE_METATYPE(MyStruct)

QDBusArgument &operator<<(QDBusArgument &arg, const MyStruct &ms)
{
    arg.beginStructure();
    arg << ms.i << ms.s;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, MyStruct &ms)
{
    arg.beginStructure();
    arg >> ms.i >> ms.s;
    arg.endStructure();
    return arg;
}

class TypesInterface;
class tst_QDBusReply: public QObject
{
    Q_OBJECT
    QDBusInterface *iface;
    TypesInterface *adaptor;
public:
    tst_QDBusReply();

private slots:
    void initTestCase()
    {
        qDBusRegisterMetaType<IntStringMap>();
        qDBusRegisterMetaType<MyStruct>();
    }

    void init();
    void simpleTypes();
    void complexTypes();
    void wrongTypes();
};

class TypesInterface: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.trolltech.Qt.Autotests.TypesInterface")
public:
    TypesInterface(QObject *parent)
        : QDBusAbstractAdaptor(parent)
    { }

public slots:
    bool retrieveBool()
    {
        return true;
    }

    uchar retrieveUChar()
    {
        return 'A';
    }

    short retrieveShort()
    {
        return -47;
    }

    ushort retrieveUShort()
    {
        return 42U;
    }

    int retrieveInt()
    {
        return -470000;
    }

    uint retrieveUInt()
    {
        return 42424242;
    }

    qlonglong retrieveLongLong()
    {
        return -(Q_INT64_C(1) << 32);
    }

    qulonglong retrieveULongLong()
    {
        return Q_INT64_C(1) << 32;
    }

    double retrieveDouble()
    {
        return 1.5;
    }

    QString retrieveString()
    {
        return "This string you should see";
    }

    QDBusObjectPath retrieveObjectPath()
    {
        return QDBusObjectPath("/");
    }

    QDBusSignature retrieveSignature()
    {
        return QDBusSignature("g");
    }

    QDBusVariant retrieveVariant()
    {
        return QDBusVariant(retrieveString());
    }

    QStringList retrieveStringList()
    {
        return QStringList() << "one" << "two";
    }

    QByteArray retrieveByteArray()
    {
        return "Hello, World";
    }

    QVariantList retrieveList()
    {
        return QVariantList() << retrieveInt() << retrieveString()
                              << retrieveByteArray();
    }

    QVariantMap retrieveMap()
    {
        QVariantMap map;
        map["one"] = 1;
        map["two"] = 2U;
        map["string"] = retrieveString();
        map["stringlist"] = retrieveStringList();
        return map;
    }

    IntStringMap retrieveIntStringMap()
    {
        IntStringMap map;
        map[1] = "1";
        map[2] = "2";
        map[-1231456] = "foo";
        return map;
    }

    MyStruct retrieveStruct()
    {
        return MyStruct();
    }
};

tst_QDBusReply::tst_QDBusReply()
{
    adaptor = new TypesInterface(this);
    QDBusConnection::sessionBus().registerObject("/", this);

    iface = new QDBusInterface(QDBusConnection::sessionBus().baseService(), "/",
                               "com.trolltech.Qt.Autotests.TypesInterface",
                               QDBusConnection::sessionBus(),
                               this);
}

void tst_QDBusReply::init()
{
    QVERIFY(iface);
    QVERIFY(iface->isValid());
}

void tst_QDBusReply::simpleTypes()
{
    QDBusReply<bool> rbool = iface->call(QDBus::BlockWithGui, "retrieveBool");
    QVERIFY(rbool.isValid());
    QCOMPARE(rbool.value(), adaptor->retrieveBool());

    QDBusReply<uchar> ruchar = iface->call(QDBus::BlockWithGui, "retrieveUChar");
    QVERIFY(ruchar.isValid());
    QCOMPARE(ruchar.value(), adaptor->retrieveUChar());

    QDBusReply<short> rshort = iface->call(QDBus::BlockWithGui, "retrieveShort");
    QVERIFY(rshort.isValid());
    QCOMPARE(rshort.value(), adaptor->retrieveShort());

    QDBusReply<ushort> rushort = iface->call(QDBus::BlockWithGui, "retrieveUShort");
    QVERIFY(rushort.isValid());
    QCOMPARE(rushort.value(), adaptor->retrieveUShort());

    QDBusReply<int> rint = iface->call(QDBus::BlockWithGui, "retrieveInt");
    QVERIFY(rint.isValid());
    QCOMPARE(rint.value(), adaptor->retrieveInt());

    QDBusReply<uint> ruint = iface->call(QDBus::BlockWithGui, "retrieveUInt");
    QVERIFY(ruint.isValid());
    QCOMPARE(ruint.value(), adaptor->retrieveUInt());

    QDBusReply<qlonglong> rqlonglong = iface->call(QDBus::BlockWithGui, "retrieveLongLong");
    QVERIFY(rqlonglong.isValid());
    QCOMPARE(rqlonglong.value(), adaptor->retrieveLongLong());

    QDBusReply<qulonglong> rqulonglong = iface->call(QDBus::BlockWithGui, "retrieveULongLong");
    QVERIFY(rqulonglong.isValid());
    QCOMPARE(rqulonglong.value(), adaptor->retrieveULongLong());

    QDBusReply<double> rdouble = iface->call(QDBus::BlockWithGui, "retrieveDouble");
    QVERIFY(rdouble.isValid());
    QCOMPARE(rdouble.value(), adaptor->retrieveDouble());

    QDBusReply<QString> rstring = iface->call(QDBus::BlockWithGui, "retrieveString");
    QVERIFY(rstring.isValid());
    QCOMPARE(rstring.value(), adaptor->retrieveString());

    QDBusReply<QDBusObjectPath> robjectpath = iface->call(QDBus::BlockWithGui, "retrieveObjectPath");
    QVERIFY(robjectpath.isValid());
    QCOMPARE(robjectpath.value().value, adaptor->retrieveObjectPath().value);

    QDBusReply<QDBusSignature> rsignature = iface->call(QDBus::BlockWithGui, "retrieveSignature");
    QVERIFY(rsignature.isValid());
    QCOMPARE(rsignature.value().value, adaptor->retrieveSignature().value);

    QDBusReply<QDBusVariant> rdbusvariant = iface->call(QDBus::BlockWithGui, "retrieveVariant");
    QVERIFY(rdbusvariant.isValid());
    QCOMPARE(rdbusvariant.value().value, adaptor->retrieveVariant().value);

    QDBusReply<QVariant> rvariant = iface->call(QDBus::BlockWithGui, "retrieveVariant");
    QVERIFY(rvariant.isValid());
    QCOMPARE(rvariant.value(), adaptor->retrieveVariant().value);

    QDBusReply<QByteArray> rbytearray = iface->call(QDBus::BlockWithGui, "retrieveByteArray");
    QVERIFY(rbytearray.isValid());
    QCOMPARE(rbytearray.value(), adaptor->retrieveByteArray());

    QDBusReply<QStringList> rstringlist = iface->call(QDBus::BlockWithGui, "retrieveStringList");
    QVERIFY(rstringlist.isValid());
    QCOMPARE(rstringlist.value(), adaptor->retrieveStringList());
}

void tst_QDBusReply::complexTypes()
{
    QDBusReply<QVariantList> rlist = iface->call(QDBus::BlockWithGui, "retrieveList");
    QVERIFY(rlist.isValid());
    QCOMPARE(rlist.value(), adaptor->retrieveList());

    QDBusReply<QVariantMap> rmap = iface->call(QDBus::BlockWithGui, "retrieveMap");
    QVERIFY(rmap.isValid());
    QCOMPARE(rmap.value(), adaptor->retrieveMap());

    QDBusReply<IntStringMap> rismap = iface->call(QDBus::BlockWithGui, "retrieveIntStringMap");
    QVERIFY(rismap.isValid());
    QCOMPARE(rismap.value(), adaptor->retrieveIntStringMap());

    QDBusReply<MyStruct> rstruct = iface->call(QDBus::BlockWithGui, "retrieveStruct");
    QVERIFY(rstruct.isValid());
    QCOMPARE(rstruct.value(), adaptor->retrieveStruct());
}

void tst_QDBusReply::wrongTypes()
{
    QDBusReply<bool> rbool = iface->call(QDBus::BlockWithGui, "retrieveInt");
    QVERIFY(!rbool.isValid());

    rbool = iface->call(QDBus::BlockWithGui, "retrieveShort");
    QVERIFY(!rbool.isValid());

    rbool = iface->call(QDBus::BlockWithGui, "retrieveStruct");
    QVERIFY(!rbool.isValid());

    QDBusReply<short> rshort = iface->call(QDBus::BlockWithGui, "retrieveInt");
    QVERIFY(!rshort.isValid());

    rshort = iface->call(QDBus::BlockWithGui, "retrieveBool");
    QVERIFY(!rshort.isValid());

    rshort = iface->call(QDBus::BlockWithGui, "retrieveStruct");
    QVERIFY(!rshort.isValid());

    QDBusReply<MyStruct> rstruct = iface->call(QDBus::BlockWithGui, "retrieveInt");
    QVERIFY(!rstruct.isValid());

    rstruct = iface->call(QDBus::BlockWithGui, "retrieveShort");
    QVERIFY(!rstruct.isValid());

    rstruct = iface->call(QDBus::BlockWithGui, "retrieveIntStringMap");
    QVERIFY(!rstruct.isValid());
}

QTEST_MAIN(tst_QDBusReply)

#include "tst_qdbusreply.moc"
