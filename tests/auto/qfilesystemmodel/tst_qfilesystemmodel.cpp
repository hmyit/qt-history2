/****************************************************************************
**
** Copyright (C) 2005-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include "../../../src/gui/dialogs/qfilesystemmodel_p.h"
#include <QFileIconProvider>

//TESTED_CLASS=QFileSystemModel
//TESTED_FILES=gui/dialogs/qfilesystemmodel.h gui/dialogs/qfilesystemmodel.cpp

#define WAITTIME 1000

// Will try to wait for the condition while allowing event processing
// for a maximum of 5 seconds.
#define TRY_COMPARE(expr, expected) \
    do { \
        const int step = 50; \
        for (int i = 0; i < 5000 && ((expr) != (expected)); i+=step) { \
            QTest::qWait(step); \
        } \
        QCOMPARE(expr, expected); \
    } while(0)

// Will try to wait for the condition while allowing event processing
// for a maximum of 5 seconds.
#define TRY_VERIFY(expr) \
    do { \
        const int step = 50; \
        for (int i = 0; i < 5000 && !(expr); i+=step) { \
            QTest::qWait(step); \
        } \
        QVERIFY(expr); \
    } while(0)

// Will try to wait for the condition while allowing event processing
// for a maximum of 5 seconds.
#define TRY_WAIT(expr) \
    do { \
        const int step = 50; \
        for (int i = 0; i < 5000 && !(expr); i+=step) { \
            QTest::qWait(step); \
        } \
    } while(0)

class tst_QFileSystemModel : public QObject {
  Q_OBJECT

public:
    tst_QFileSystemModel();
    virtual ~tst_QFileSystemModel();

public Q_SLOTS:
    void init();
    void cleanup();

private slots:
    void indexPath();

    void rootPath();
    void naturalCompare_data();
    void naturalCompare();
    void readOnly();
    void iconProvider();

    void rowCount();

    void rowsInserted_data();
    void rowsInserted();

    void rowsRemoved_data();
    void rowsRemoved();

    void dataChanged_data();
    void dataChanged();

    void filters_data();
    void filters();

    void nameFilters();

    void setData_data();
    void setData();

    void sort();

    void mkdir();

    void caseSensitivity();

protected:
    bool createFiles(const QString &test_path, const QStringList &initial_files, int existingFileCount = 0, const QStringList &intial_dirs = QStringList(), const QString &baseDir = QDir::temp().absolutePath());

private:
    QFileSystemModel *model;
};

tst_QFileSystemModel::tst_QFileSystemModel() : model(0)
{
    qRegisterMetaType<QModelIndex>("QModelIndex");
}

tst_QFileSystemModel::~tst_QFileSystemModel()
{
    QString tmp = QDir::temp().path() + '/' + QString("flatdirtest");
    QDir dir(tmp);
    if (!dir.rmdir(tmp))
        qWarning("failed to remove tmp dir");
}

void tst_QFileSystemModel::init()
{
    cleanup();
    QCOMPARE(model, (QFileSystemModel*)0);
    model = new QFileSystemModel;
}

void tst_QFileSystemModel::cleanup()
{
    delete model;
    model = 0;
    QString tmp = QDir::temp().path() + '/' + QString("flatdirtest");
    QDir dir(tmp);
    if (dir.exists(tmp)) {
        QStringList list = dir.entryList(QDir::AllEntries | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot);
        for (int i = 0; i < list.count(); ++i) {
            QFileInfo fi(dir.path() + '/' + list.at(i));
            if (fi.exists() && fi.isFile()) {
		        QFile p(fi.absoluteFilePath());
                p.setPermissions(QFile::ReadUser | QFile::ReadOwner | QFile::ExeOwner | QFile::ExeUser | QFile::WriteUser | QFile::WriteOwner | QFile::WriteOther);
		        QFile dead(dir.path() + '/' + list.at(i));
		        dead.remove();
	        }
	        if (fi.exists() && fi.isDir())
                QVERIFY(dir.rmdir(list.at(i)));
        }
        list = dir.entryList(QDir::AllEntries | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot);
        QVERIFY(list.count() == 0);
    }
}

void tst_QFileSystemModel::indexPath()
{
#ifndef Q_OS_WIN
    int depth = QDir::currentPath().count('/');
    model->setRootPath(QDir::currentPath());
    QTest::qWait(WAITTIME);
    QString backPath;
    for (int i = 0; i <= depth * 2 + 1; ++i) {
        backPath += "../";
        QModelIndex idx = model->index(backPath);
        QVERIFY(i != depth - 1 ? idx.isValid() : !idx.isValid());
    }
    QTest::qWait(WAITTIME * 3);
    qApp->processEvents();
#endif
}

void tst_QFileSystemModel::rootPath()
{
    QCOMPARE(model->rootPath(), QString(QDir().path()));

    QSignalSpy rootChanged(model, SIGNAL(rootPathChanged(const QString &)));
    QModelIndex root = model->setRootPath(model->rootPath());
    root = model->setRootPath("this directory shouldn't exist");
    QCOMPARE(rootChanged.count(), 0);

    QString oldRootPath = model->rootPath();
    root = model->setRootPath(QDir::homePath());

    TRY_VERIFY(model->rowCount(root) > 0);
    QCOMPARE(model->rootPath(), QString(QDir::homePath()));
    QCOMPARE(rootChanged.count(), oldRootPath == model->rootPath() ? 0 : 1);
    QCOMPARE(model->rootDirectory().absolutePath(), QDir::homePath());
}

void tst_QFileSystemModel::naturalCompare_data()
{
    QTest::addColumn<QString>("s1");
    QTest::addColumn<QString>("s2");
    QTest::addColumn<int>("caseSensitive");
    QTest::addColumn<int>("result");
    QTest::addColumn<int>("swap");
    for (int j = 0; j < 4; ++j) { // <- set a prefix and a postfix string (not numbers)
        QString prefix = (j == 0 || j == 1) ? "b" : "";
        QString postfix = (j == 1 || j == 2) ? "y" : "";

        for (int k = 0; k < 3; ++k) { // <- make 0 not a special case
            QString num = QString("%1").arg(k);
            QString nump = QString("%1").arg(k + 1);
            for (int i = 10; i < 12; ++i) { // <- swap s1 and s2 and reverse the result
                QTest::newRow("basic") << prefix + "0" + postfix << prefix + "0" + postfix << int(Qt::CaseInsensitive) << 0;

                // s1 should always be less then s2
                QTest::newRow("just text")    << prefix + "fred" + postfix     << prefix + "jane" + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("just numbers") << prefix + num + postfix        << prefix + "9" + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("zero")         << prefix + num + postfix        << prefix + "0" + nump + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("space b")      << prefix + num + postfix        << prefix + " " + nump + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("space a")      << prefix + num + postfix        << prefix + nump + " " + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("tab b")        << prefix + num + postfix        << prefix + "    " + nump + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("tab a")        << prefix + num + postfix        << prefix + nump + "   " + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("10 vs 2")      << prefix + num + postfix        << prefix + "10" + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("diff len")     << prefix + num + postfix        << prefix + nump + postfix + "x" << int(Qt::CaseInsensitive) << i;
                QTest::newRow("01 before 1")  << prefix + "0" + num + postfix  << prefix + nump + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("mul nums 2nd") << prefix + "1-" + num + postfix << prefix + "1-" + nump + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("mul nums 2nd") << prefix + "10-" + num + postfix<< prefix + "10-10" + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("mul nums 2nd") << prefix + "10-0"+ num + postfix<< prefix + "10-10" + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("mul nums 2nd") << prefix + "10-" + num + postfix<< prefix + "10-010" + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("mul nums big") << prefix + "10-" + num + postfix<< prefix + "20-0" + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("mul nums big") << prefix + "2-" + num + postfix << prefix + "10-0" + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("mul alphabet") << prefix + num + "-a" + postfix << prefix + num + "-c" + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("mul alphabet2")<< prefix + num + "-a9" + postfix<< prefix + num + "-c0" + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("mul nums w\\0")<< prefix + num + "-"+ num + postfix<< prefix + num+"-0"+nump + postfix << int(Qt::CaseInsensitive) << i;
                QTest::newRow("num first")    << prefix + num + postfix  << prefix + "a" + postfix << int(Qt::CaseInsensitive) << i;
            }
        }
    }
}

void tst_QFileSystemModel::naturalCompare()
{
    QFETCH(QString, s1);
    QFETCH(QString, s2);
    QFETCH(int, caseSensitive);
    QFETCH(int, result);

    if (result == 10)
        QCOMPARE(QFileSystemModelPrivate::naturalCompare(s1, s2, Qt::CaseSensitivity(caseSensitive)), -1);
    else
        if (result == 11)
            QCOMPARE(QFileSystemModelPrivate::naturalCompare(s2, s1, Qt::CaseSensitivity(caseSensitive)), 1);
    else
        QCOMPARE(QFileSystemModelPrivate::naturalCompare(s2, s1, Qt::CaseSensitivity(caseSensitive)), result);
}

void tst_QFileSystemModel::readOnly()
{
    QCOMPARE(model->isReadOnly(), true);
    QModelIndex root = model->setRootPath(QDir::homePath());

    TRY_VERIFY(model->rowCount(root) > 0);
    QVERIFY(!(model->flags(model->index(0, 0, root)) & Qt::ItemIsEditable));
    model->setReadOnly(false);
    QCOMPARE(model->isReadOnly(), false);
    QVERIFY(model->flags(model->index(0, 0, root)) & Qt::ItemIsEditable);
}

void tst_QFileSystemModel::iconProvider()
{
    QVERIFY(model->iconProvider());
    QFileIconProvider *p = new QFileIconProvider();
    model->setIconProvider(p);
    QCOMPARE(model->iconProvider(), p);
    model->setIconProvider(0);
    delete p;
}

bool tst_QFileSystemModel::createFiles(const QString &test_path, const QStringList &initial_files, int existingFileCount, const QStringList &initial_dirs, const QString &dir)
{
    QDir baseDir(dir);
    if (!baseDir.exists(test_path)) {
        if (!baseDir.mkdir(test_path) && false) {
            qDebug() << "failed to create dir" << test_path;
            return false;
        }
    }
    //qDebug() << (model->rowCount(model->index(test_path))) << existingFileCount << initial_files;
    TRY_WAIT((model->rowCount(model->index(test_path)) == existingFileCount));
    for (int i = 0; i < initial_dirs.count(); ++i) {
        QDir dir(test_path);
        if (!dir.exists()) {
            qWarning() << "error" << test_path << "doesn't exists";
            return false;
        }
        if(!dir.mkdir(initial_dirs.at(i))) {
            qWarning() << "error" << "failed to make" << initial_dirs.at(i);
            return false;
        }
        //qDebug() << test_path + '/' + initial_dirs.at(i) << (QFile::exists(test_path + '/' + initial_dirs.at(i)));
    }
    for (int i = 0; i < initial_files.count(); ++i) {
        QFile file(test_path + '/' + initial_files.at(i));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
            qDebug() << "failed to open file" << initial_files.at(i);
            return false;
        }
        if (!file.resize(1024 + file.size())) {
            qDebug() << "failed to resize file" << initial_files.at(i);
            return false;
        }
        if (!file.flush()) {
            qDebug() << "failed to flush file" << initial_files.at(i);
            return false;
        }
        file.close();
#ifdef Q_OS_WIN
        if (initial_files.at(i)[0] == '.')
            QProcess::execute(QString("attrib +h %1").arg(file.fileName()));
#endif
        //qDebug() << test_path + '/' + initial_files.at(i) << (QFile::exists(test_path + '/' + initial_files.at(i)));
    }

    return true;
}

void tst_QFileSystemModel::rowCount()
{
    QString tmp = QDir::temp().path() + '/' + QString("flatdirtest");
    QVERIFY(createFiles(tmp, QStringList()));

    QSignalSpy spy2(model, SIGNAL(rowsInserted(const QModelIndex &, int, int)));
    QSignalSpy spy3(model, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)));

    QStringList files = QStringList() <<  "b" << "d" << "f" << "h" << "j" << ".a" << ".c" << ".e" << ".g";
    QString l = "b,d,f,h,j,.a,.c,.e,.g";
    QVERIFY(createFiles(tmp, files));

    QModelIndex root = model->setRootPath(tmp);
    TRY_COMPARE(model->rowCount(root), 5);
    QVERIFY(spy2.count() > 0);
    QVERIFY(spy3.count() > 0);
}

void tst_QFileSystemModel::rowsInserted_data()
{
    QTest::addColumn<int>("count");
    QTest::addColumn<int>("assending");
    for (int i = 0; i < 4; ++i) {
        QTest::newRow(QString("Qt::AscendingOrder %1").arg(i).toLocal8Bit().constData())  << i << (int)Qt::AscendingOrder;
        QTest::newRow(QString("Qt::DescendingOrder %1").arg(i).toLocal8Bit().constData()) << i << (int)Qt::DescendingOrder;
    }
}

void tst_QFileSystemModel::rowsInserted()
{
    QString tmp = QDir::temp().path() + '/' + QString("flatdirtest");
    rowCount();
    QModelIndex root = model->index(model->rootPath());

    QFETCH(int, assending);
    QFETCH(int, count);
    model->sort(0, (Qt::SortOrder)assending);

    QSignalSpy spy0(model, SIGNAL(rowsInserted(const QModelIndex &, int, int)));
    QSignalSpy spy1(model, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)));
    int oldCount = model->rowCount(root);
    for (int i = 0; i < count; ++i)
        QVERIFY(createFiles(tmp, QStringList(QString("c%1").arg(i)), 5 + i));
    TRY_COMPARE(model->rowCount(root), oldCount + count);
    QCOMPARE(spy0.count(), count);
    if (assending == (Qt::SortOrder)Qt::AscendingOrder) {
        QString letter = model->index(model->rowCount(root) - 1, 0, root).data().toString();
        QCOMPARE(letter, QString("j"));
    } else {
        QCOMPARE(model->index(model->rowCount(root) - 1, 0, root).data().toString(), QString("b"));
    }
    if (spy0.count() > 0)
    if (count == 0) QCOMPARE(spy0.count(), 0); else QVERIFY(spy0.count() >= 1);
    if (count == 0) QCOMPARE(spy1.count(), 0); else QVERIFY(spy1.count() >= 1);

    QVERIFY(createFiles(tmp, QStringList(".hidden_file"), 5 + count));

    if (count != 0) TRY_VERIFY(spy0.count() >= 1); else TRY_VERIFY(spy0.count() == 0);
    if (count != 0) TRY_VERIFY(spy1.count() >= 1); else TRY_VERIFY(spy1.count() == 0);
}

void tst_QFileSystemModel::rowsRemoved_data()
{
    rowsInserted_data();
}

void tst_QFileSystemModel::rowsRemoved()
{
    QString tmp = QDir::temp().path() + '/' + QString("flatdirtest");
    rowCount();
    QModelIndex root = model->index(model->rootPath());

    QFETCH(int, count);
    QFETCH(int, assending);
    model->sort(0, (Qt::SortOrder)assending);
    QTest::qWait(WAITTIME);

    QSignalSpy spy0(model, SIGNAL(rowsRemoved(const QModelIndex &, int, int)));
    QSignalSpy spy1(model, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)));
    int oldCount = model->rowCount(root);
    for (int i = count - 1; i >= 0; --i) {
        //qDebug() << "removing" <<  model->index(i, 0, root).data().toString();
        QVERIFY(QFile::remove(tmp + '/' + model->index(i, 0, root).data().toString()));
    }
    for (int i = 0 ; i < 10; ++i) {
        QTest::qWait(WAITTIME);
        qApp->processEvents();
        if (count != 0) {
            if (i == 10 || spy0.count() != 0) {
                QVERIFY(spy0.count() >= 1);
                QVERIFY(spy1.count() >= 1);
            }
        } else {
            if (i == 10 || spy0.count() == 0) {
                QVERIFY(spy0.count() == 0);
                QVERIFY(spy1.count() == 0);
            }
        }
        QStringList lst;
        for (int i = 0; i < model->rowCount(root); ++i)
            lst.append(model->index(i, 0, root).data().toString());
        if (model->rowCount(root) == oldCount - count)
            break;
        qDebug() << "still have:" << lst << QFile::exists(tmp + '/' + QString(".a"));
        QDir tmpLister(tmp);
        qDebug() << tmpLister.entryList();
    }
    QCOMPARE(model->rowCount(root), oldCount - count);

    QVERIFY(QFile::exists(tmp + '/' + QString(".a")));
    QVERIFY(QFile::remove(tmp + '/' + QString(".a")));
    QVERIFY(QFile::remove(tmp + '/' + QString(".c")));
    QTest::qWait(WAITTIME);

    if (count != 0) QVERIFY(spy0.count() >= 1); else QVERIFY(spy0.count() == 0);
    if (count != 0) QVERIFY(spy1.count() >= 1); else QVERIFY(spy1.count() == 0);
}

void tst_QFileSystemModel::dataChanged_data()
{
    rowsInserted_data();
}

void tst_QFileSystemModel::dataChanged()
{
    // This can't be tested right now sense we don't watch files, only directories
    return;

    /*
    QString tmp = QDir::temp().path() + '/' + QString("flatdirtest");
    rowCount();
    QModelIndex root = model->index(model->rootPath());

    QFETCH(int, count);
    QFETCH(int, assending);
    model->sort(0, (Qt::SortOrder)assending);

    QSignalSpy spy(model, SIGNAL(dataChanged (const QModelIndex &, const QModelIndex &)));
    QStringList files;
    for (int i = 0; i < count; ++i)
        files.append(model->index(i, 0, root).data().toString());
    createFiles(tmp, files);

    QTest::qWait(WAITTIME);

    if (count != 0) QVERIFY(spy.count() >= 1); else QVERIFY(spy.count() == 0);
    */
}

void tst_QFileSystemModel::filters_data()
{
    QTest::addColumn<QStringList>("files");
    QTest::addColumn<QStringList>("dirs");
    QTest::addColumn<int>("dirFilters");
    QTest::addColumn<QStringList>("nameFilters");
    QTest::addColumn<int>("rowCount");
    QTest::newRow("no dirs") << (QStringList() << "a" << "b" << "c") << QStringList() << (int)(QDir::Dirs) << QStringList() << 2;
    QTest::newRow("one dir - dotdot") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") << (int)(QDir::Dirs | QDir::NoDotAndDotDot) << QStringList() << 1;
    QTest::newRow("one dir") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") << (int)(QDir::Dirs) << QStringList() << 3;
    QTest::newRow("no dir + hidden") << (QStringList() << "a" << "b" << "c") << QStringList() << (int)(QDir::Dirs | QDir::Hidden) << QStringList() << 2;
    QTest::newRow("dir+hid+files") << (QStringList() << "a" << "b" << "c") << QStringList() <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden) << QStringList() << 5;
    QTest::newRow("dir+file+hid-dot .A") << (QStringList() << "a" << "b" << "c") << (QStringList() << ".A") <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot) << QStringList() << 4;
    QTest::newRow("dir+files+hid+dot A") << (QStringList() << "a" << "b" << "c") << (QStringList() << "AFolder") <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot) << (QStringList() << "A*") << 2;
    QTest::newRow("dir+files+hid+dot+cas1") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot | QDir::CaseSensitive) << (QStringList() << "Z") << 1;
    QTest::newRow("dir+files+hid+dot+cas2") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot | QDir::CaseSensitive) << (QStringList() << "a") << 1;
    QTest::newRow("dir+files+hid+dot+cas+alldir") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot | QDir::CaseSensitive | QDir::AllDirs) << (QStringList() << "Z") << 1;

    QTest::newRow("case sensitive") << (QStringList() << "Antiguagdb" << "Antiguamtd"
        << "Antiguamtp" << "afghanistangdb" << "afghanistanmtd")
        << QStringList() << (int)(QDir::Files) << QStringList() << 5;
}

void tst_QFileSystemModel::filters()
{
    QString tmp = QDir::temp().path() + '/' + QString("flatdirtest");
    QVERIFY(createFiles(tmp, QStringList()));
    QModelIndex root = model->setRootPath(tmp);

    QFETCH(QStringList, files);
    QFETCH(QStringList, dirs);
    QFETCH(int, dirFilters);
    QFETCH(QStringList, nameFilters);
    QFETCH(int, rowCount);

    if (nameFilters.count() > 0)
        model->setNameFilters(nameFilters);
    model->setNameFilterDisables(false);
    model->setFilter((QDir::Filters)dirFilters);

    QVERIFY(createFiles(tmp, files, 0, dirs));
    TRY_COMPARE(model->rowCount(root), rowCount);

    // Make sure that we do what QDir does
    QDir xFactor(tmp);
    QDir::Filters  filters = (QDir::Filters)dirFilters;
    if (nameFilters.count() > 0)
        QCOMPARE(xFactor.entryList(nameFilters, filters).count(), rowCount);
    else
        QVERIFY(xFactor.entryList(filters).count() == rowCount);

    if (files.count() >= 3 && rowCount >= 3 && rowCount != 5) {
        QString fileName1 = (tmp + '/' + files.at(0));
        QString fileName2 = (tmp + '/' + files.at(1));
        QString fileName3 = (tmp + '/' + files.at(2));
        QFile::Permissions originalPermissions = QFile::permissions(fileName1);
        QVERIFY(QFile::setPermissions(fileName1, QFile::WriteOwner));
        QVERIFY(QFile::setPermissions(fileName2, QFile::ReadOwner));
#ifndef Q_OS_WIN
        QVERIFY(QFile::setPermissions(fileName3, QFile::ExeOwner));
#endif
        model->setFilter((QDir::Files | QDir::Readable));
        TRY_COMPARE(model->rowCount(root), 1);

        model->setFilter((QDir::Files | QDir::Writable));
        TRY_COMPARE(model->rowCount(root), 1);

#ifndef Q_OS_WIN
        model->setFilter((QDir::Files | QDir::Executable));
        TRY_COMPARE(model->rowCount(root), 1);
#endif
        // reset permissions
        QVERIFY(QFile::setPermissions(fileName1, originalPermissions));
        QVERIFY(QFile::setPermissions(fileName2, originalPermissions));
#ifndef Q_OS_WIN
        QVERIFY(QFile::setPermissions(fileName3, originalPermissions));
#endif
    }
}

void tst_QFileSystemModel::nameFilters()
{
    QStringList list;
    list << "a" << "b" << "c";
    model->setNameFilters(list);
    model->setNameFilterDisables(false);
    QCOMPARE(model->nameFilters(), list);

    QString tmp = QDir::temp().path() + '/' + QString("flatdirtest");
    QVERIFY(createFiles(tmp, list));
    QModelIndex root = model->setRootPath(tmp);
    QTest::qWait(WAITTIME);
    QCOMPARE(model->rowCount(root), 3);

    QStringList filters;
    filters << "a" << "b";
    model->setNameFilters(filters);
    QTest::qWait(WAITTIME);

    QCOMPARE(model->rowCount(root), 2);
}
void tst_QFileSystemModel::setData_data()
{
    QTest::addColumn<QStringList>("files");
    QTest::addColumn<QString>("oldFileName");
    QTest::addColumn<QString>("newFileName");
    QTest::addColumn<bool>("success");
    /*QTest::newRow("outside current dir") << (QStringList() << "a" << "b" << "c")
              << QDir::temp().path() + '/' + QString("flatdirtest") + '/' + "a"
              << QDir::temp().absolutePath() + '/' + "a"
              << false;
    */
 QTest::newRow("in current dir") << (QStringList() << "a" << "b" << "c")
              << QDir::temp().path() + '/' + QString("flatdirtest") + '/' + "a"
              <<  "d"
              << true;
}

void tst_QFileSystemModel::setData()
{
    QString tmp = QDir::temp().path() + '/' + QString("flatdirtest");
    QFETCH(QStringList, files);
    QFETCH(QString, oldFileName);
    QFETCH(QString, newFileName);
    QFETCH(bool, success);

    QVERIFY(createFiles(tmp, files));
    QModelIndex root = model->setRootPath(tmp);
    QTest::qWait(WAITTIME);
    QCOMPARE(model->rowCount(root), 3);

    QModelIndex idx = model->index(oldFileName);
    QCOMPARE(idx.isValid(), true);
    QCOMPARE(model->setData(idx, newFileName), false);

    model->setReadOnly(false);
    QCOMPARE(model->setData(idx, newFileName), success);
    if (success)
        QCOMPARE(QFile::rename(tmp + '/' + newFileName, oldFileName), true);

    QTest::qWait(WAITTIME);
    QCOMPARE(model->rowCount(root), 3);
}

void tst_QFileSystemModel::sort()
{
    QModelIndex root = model->setRootPath(QDir::home().absolutePath());
    TRY_VERIFY(model->rowCount(root) > 0);

    QPersistentModelIndex idx = model->index(0, 1, root);
    model->sort(0, Qt::AscendingOrder);
    model->sort(0, Qt::DescendingOrder);
    QVERIFY(idx.column() != 0);
}


void tst_QFileSystemModel::mkdir()
{
    QString tmp = QDir::tempPath();
    QString newFolderPath = QDir::toNativeSeparators(tmp + '/' + "NewFoldermkdirtest4");
    QModelIndex tmpDir = model->index(tmp);
    QVERIFY(tmpDir.isValid());
    QDir bestatic(newFolderPath);
    if (bestatic.exists()) {
        if (!bestatic.rmdir(newFolderPath))
            qWarning() << "unable to remove" << newFolderPath;
        QTest::qWait(WAITTIME);
    }
    model->mkdir(tmpDir, "NewFoldermkdirtest3");
    model->mkdir(tmpDir, "NewFoldermkdirtest5");
    QModelIndex idx = model->mkdir(tmpDir, "NewFoldermkdirtest4");
    QVERIFY(idx.isValid());
    int oldRow = idx.row();
    QTest::qWait(WAITTIME);
    idx = model->index(newFolderPath);
    QDir cleanup(tmp);
    QVERIFY(cleanup.rmdir(QLatin1String("NewFoldermkdirtest3")));
    QVERIFY(cleanup.rmdir(QLatin1String("NewFoldermkdirtest5")));
    bestatic.rmdir(newFolderPath);
    QVERIFY(0 != idx.row());
    QCOMPARE(oldRow, idx.row());
}

void tst_QFileSystemModel::caseSensitivity()
{
    QString tmp = QDir::temp().path() + '/' + QString("flatdirtest");
    QStringList files;
    files << "a" << "c" << "C";
    QVERIFY(createFiles(tmp, files));
    QModelIndex root = model->index(tmp);
    QCOMPARE(model->rowCount(root), 0);
    for (int i = 0; i < files.count(); ++i) {
        QVERIFY(model->index(tmp + '/' + files.at(i)).isValid());
    }
}

QTEST_MAIN(tst_QFileSystemModel)
#include "tst_qfilesystemmodel.moc"

