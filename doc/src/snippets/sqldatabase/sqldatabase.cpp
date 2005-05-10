#include <QtGui>
#include <QtSql>

#include <iostream>

using namespace std;

void QSqlDatabase_snippets()
{
    {
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("acidalia");
    db.setDatabaseName("customdb");
    db.setUserName("mojito");
    db.setPassword("J0a1m8");
    bool ok = db.open();
    Q_UNUSED(ok);
    }

    {
    QSqlDatabase db = QSqlDatabase::database();
    }
}

void QSqlField_snippets()
{
#if 0
    {
    QSqlField field("age", QVariant::Int);
    field.setValue(QPixmap());  // WRONG
    }
#endif

    {
    QSqlField field("age", QVariant::Int);
    field.setValue(QString("123"));  // casts QString to int
    }

    {
    QSqlQuery query;
    QSqlRecord record = query.record();
    QSqlField field = record.field("country");
    }
}

void doSomething(const QString &)
{
}

void QSqlQuery_snippets()
{
    {
    // typical loop
    QSqlQuery query("SELECT country FROM artist");
    while (query.next()) {
        QString country = query.value(0).toString();
        doSomething(country);
    }
    }

    {
    // field index lookup
    QSqlQuery query("SELECT * FROM artist");
    int fieldNo = query.record().indexOf("country");
    while (query.next()) {
        QString country = query.value(fieldNo).toString();
        doSomething(country);
    }
    }

    {
    // named with named
    QSqlQuery query;
    query.prepare("INSERT INTO person (id, forename, surname) "
                  "VALUES (:id, :forename, :surname)");
    query.bindValue(":id", 1001);
    query.bindValue(":forename", "Bart");
    query.bindValue(":surname", "Simpson");
    query.exec();
    }

    {
    // positional with named
    QSqlQuery query;
    query.prepare("INSERT INTO person (id, forename, surname) "
                  "VALUES (:id, :forename, :surname)");
    query.bindValue(0, 1001);
    query.bindValue(1, "Bart");
    query.bindValue(2, "Simpson");
    query.exec();
    }

    {
    // positional 1
    QSqlQuery query;
    query.prepare("INSERT INTO person (id, forename, surname) "
                  "VALUES (?, ?, ?)");
    query.bindValue(0, 1001);
    query.bindValue(1, "Bart");
    query.bindValue(2, "Simpson");
    query.exec();
    }

    {
    // positional 2
    QSqlQuery query;
    query.prepare("INSERT INTO person (id, forename, surname) "
                  "VALUES (?, ?, ?)");
    query.addBindValue(1001);
    query.addBindValue("Bart");
    query.addBindValue("Simpson");
    query.exec();
    }

    {
    // stored
    QSqlQuery query;
    query.prepare("CALL AsciiToInt(?, ?)");
    query.bindValue(0, "A");
    query.bindValue(1, 0, QSql::Out);
    query.exec();
    int i = query.boundValue(1).toInt(); // i is 65
    Q_UNUSED(i);
    }

    QSqlQuery query;

    {
    // examine with named binding
    QMapIterator<QString, QVariant> i(query.boundValues());
    while (i.hasNext()) {
        i.next();
        cout << i.key().toAscii().data() << ": "
             << i.value().toString().toAscii().data() << endl;
    }
    }

    {
    // examine with positional binding
    QList<QVariant> list = query.boundValues().values();
    for (int i = 0; i < list.size(); ++i)
        cout << i << ": " << list.at(i).toString().toAscii().data() << endl;
    }
}

void QSqlQueryModel_snippets()
{
    {
    QSqlQueryModel *model = new QSqlQueryModel;
    model->setQuery("SELECT name, salary FROM employee");
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("Name"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Salary"));

    QTableView *view = new QTableView;
    view->setModel(model);
    view->show();
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }

    QSqlQueryModel model;
    model.setQuery("SELECT * FROM employee");
    int salary = model.record(4).value("salary").toInt();
    Q_UNUSED(salary);

    {
    int salary = model.data(model.index(4, 2)).toInt();
    Q_UNUSED(salary);
    }

    for (int row = 0; row < model.rowCount(); ++row) {
        for (int col = 0; col < model.columnCount(); ++col) {
            qDebug() << model.data(model.index(row, col));
        }
    }
}

class MyModel : public QSqlQueryModel
{
public:
    QVariant data(const QModelIndex &item, int role) const;

    int m_specialColumnNo;
};

QVariant MyModel::data(const QModelIndex &item, int role) const
{
    if (item.column() == m_specialColumnNo) {
        // handle column separately
    }
    return QSqlQueryModel::data(item, role);
}

void QSqlTableModel_snippets()
{
    QSqlTableModel *model = new QSqlTableModel;
    model->setTable("employee");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->select();
    model->removeColumn(0); // don't show the ID
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("Name"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Salary"));

    QTableView *view = new QTableView;
    view->setModel(model);
    view->show();

    {
    QSqlTableModel model;
    model.setTable("employee");
    QString name = model.record(4).value("name").toString();
    }
}

void sql_intro_snippets()
{
    {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("bigblue");
    db.setDatabaseName("flightdb");
    db.setUserName("acarlson");
    db.setPassword("1uTbSbAs");
    bool ok = db.open();
    Q_UNUSED(ok);
    }

    {
    QSqlDatabase firstDB = QSqlDatabase::addDatabase("QMYSQL", "first");
    QSqlDatabase secondDB = QSqlDatabase::addDatabase("QMYSQL", "second");
    }

    {
    QSqlDatabase defaultDB = QSqlDatabase::database();
    QSqlDatabase firstDB = QSqlDatabase::database("first");
    QSqlDatabase secondDB = QSqlDatabase::database("second");
    }

    {
    // SELECT1
    QSqlQuery query;
    query.exec("SELECT name, salary FROM employee WHERE salary > 50000");

    while (query.next()) {
        QString name = query.value(0).toString();
        int salary = query.value(1).toInt();
        qDebug() << name << salary;
    }
    }

    {
    // FEATURE
    QSqlQuery query;
    int numRows;
    query.exec("SELECT name, salary FROM employee WHERE salary > 50000");

    QSqlDatabase defaultDB = QSqlDatabase::database();
    if (defaultDB.driver()->hasFeature(QSqlDriver::QuerySize)) {
        numRows = query.size();
    } else {
        // this can be very slow
        query.last();
        numRows = query.at() + 1;
    }
    }

    {
    // INSERT1
    QSqlQuery query;
    query.exec("INSERT INTO employee (id, name, salary) "
               "VALUES (1001, 'Thad Beaumont', 65000)");
    }

    {
    // NAMED BINDING
    QSqlQuery query;
    query.prepare("INSERT INTO employee (id, name, salary) "
                  "VALUES (:id, :name, :salary)");
    query.bindValue(":id", 1001);
    query.bindValue(":name", "Thad Beaumont");
    query.bindValue(":salary", 65000);
    query.exec();
    }

    {
    // POSITIONAL BINDING
    QSqlQuery query;
    query.prepare("INSERT INTO employee (id, name, salary) "
                  "VALUES (?, ?, ?)");
    query.addBindValue(1001);
    query.addBindValue("Thad Beaumont");
    query.addBindValue(65000);
    query.exec();
    }

    {
    // UPDATE1
    QSqlQuery query;
    query.exec("UPDATE employee SET salary = 70000 WHERE id = 1003");
    }

    {
    // DELETE1
    QSqlQuery query;
    query.exec("DELETE FROM employee WHERE id = 1007");
    }

    {
    // TRANSACTION
    QSqlDatabase::database().transaction();
    QSqlQuery query;
    query.exec("SELECT id FROM employee WHERE name = 'Torild Halvorsen'");
    if (query.next()) {
        int employeeId = query.value(0).toInt();
        query.exec("INSERT INTO project (id, name, ownerid) "
                   "VALUES (201, 'Manhattan Project', "
                   + QString::number(employeeId) + ")");
    }
    QSqlDatabase::database().commit();
    }

    {
    // SQLQUERYMODEL1
    QSqlQueryModel model;
    model.setQuery("SELECT * FROM employee");

    for (int i = 0; i < model.rowCount(); ++i) {
        int id = model.record(i).value("id").toInt();
        QString name = model.record(i).value("name").toString();
        qDebug() << id << name;
    }
    }

    {
    // SQLTABLEMODEL1
    QSqlTableModel model;
    model.setTable("employee");
    model.setFilter("salary > 50000");
    model.setSort(2, Qt::DescendingOrder);
    model.select();

    for (int i = 0; i < model.rowCount(); ++i) {
        QString name = model.record(i).value("name").toString();
        int salary = model.record(i).value("salary").toInt();
        qDebug() << name << salary;
    }
    }

    {
    // SQLTABLEMODEL2
    QSqlTableModel model;
    model.setTable("employee");

    for (int i = 0; i < model.rowCount(); ++i) {
        QSqlRecord record = model.record(i);
        int salary = record.value("salary").toInt();
        salary += salary / 10;
        record.setValue("salary", salary);
        model.setRecord(i, record);
    }
    model.submitAll();

    // SQLTABLEMODEL3
    int row = 1;
    int column = 2;
    model.setData(model.index(row, column), 75000);
    model.submitAll();

    // SQLTABLEMODEL4
    model.insertRows(row, 1);
    model.setData(model.index(row, 0), 1013);
    model.setData(model.index(row, 1), "Peter Gordon");
    model.setData(model.index(row, 2), 68500);
    model.submitAll();

    model.removeRows(row, 5);
    model.submitAll();
    }
}

class XyzResult : public QSqlResult
{
public:
    XyzResult(const QSqlDriver *driver)
        : QSqlResult(driver) {}
    ~XyzResult() {}

protected:
    QVariant data(int /* index */) { return QVariant(); }
    bool isNull(int /* index */) { return false; }
    bool reset(const QString & /* query */) { return false; }
    bool fetch(int /* index */) { return false; }
    bool fetchFirst() { return false; }
    bool fetchLast() { return false; }
    int size() { return 0; }
    int numRowsAffected() { return 0; }
    QSqlRecord record() { return QSqlRecord(); }
};

class XyzDriver : public QSqlDriver
{
public:
    XyzDriver() {}
    ~XyzDriver() {}

    bool hasFeature(DriverFeature /* feature */) const { return false; }
    bool open(const QString & /* db */, const QString & /* user */,
              const QString & /* password */, const QString & /* host */,
              int /* port */, const QString & /* options */)
        { return false; }
    void close() {}
    QSqlResult *createResult() const { return new XyzResult(this); }
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QSqlDatabase_snippets();
    QSqlField_snippets();
    QSqlQuery_snippets();
    QSqlQueryModel_snippets();
    QSqlTableModel_snippets();

    XyzDriver driver;
    XyzResult result(&driver);
}
