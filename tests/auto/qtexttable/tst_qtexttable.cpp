/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtTest/QtTest>


#include <qtextdocument.h>
#include <qtextdocumentfragment.h>
#include <qtexttable.h>
#include <qdebug.h>
#include <qtextcursor.h>

//TESTED_FILES=

class QTextDocument;

class tst_QTextTable : public QObject
{
    Q_OBJECT

public:
    tst_QTextTable();


public slots:
    void init();
    void cleanup();
private slots:
    void cursorPositioning();
    void variousTableModifications();
    void tableShrinking();
    void spans();
    void variousModifications2();
    void tableManager_undo();
    void tableManager_removeCell();
    void rowAt();
    void rowAtWithSpans();
    void multiBlockCells();
    void insertRows();
    void deleteInTable();
    void mergeCells();
    void splitCells();
    void blocksForTableShouldHaveEmptyFormat();
    void removeTableByRemoveRows();
    void removeTableByRemoveColumns();
    void setCellFormat();

private:
    QTextTable *create2x2Table();
    QTextTable *create4x4Table();

    QTextTable *createTable(int rows, int cols);

    QTextDocument *doc;
    QTextCursor cursor;
};

tst_QTextTable::tst_QTextTable()
{}

void tst_QTextTable::init()
{
    doc = new QTextDocument;
    cursor = QTextCursor(doc);
}

void tst_QTextTable::cleanup()
{
    cursor = QTextCursor();
    delete doc;
    doc = 0;
}

void tst_QTextTable::cursorPositioning()
{
    // ensure the cursor is placed at the beginning of the first cell upon
    // table creation
    QTextTable *table = cursor.insertTable(2, 2);

    QVERIFY(cursor == table->cellAt(0, 0).firstCursorPosition());
    QVERIFY(table->cellAt(0, 0).firstPosition() == table->firstPosition());
}

void tst_QTextTable::variousTableModifications()
{
    QTextTableFormat tableFmt;

    QTextTable *tab = cursor.insertTable(2, 2, tableFmt);
    QVERIFY(doc->toPlainText().length() == 5);
    QVERIFY(tab == cursor.currentTable());
    QVERIFY(tab->columns() == 2);
    QVERIFY(tab->rows() == 2);

    QVERIFY(cursor.position() == 1);
    QTextCharFormat fmt = cursor.charFormat();
    QVERIFY(fmt.objectIndex() == -1);
    QTextTableCell cell = tab->cellAt(cursor);
    QVERIFY(cell.isValid());
    QVERIFY(cell.row() == 0);
    QVERIFY(cell.column() == 0);

    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.position() == 2);
    fmt = cursor.charFormat();
    QVERIFY(fmt.objectIndex() == -1);
    cell = tab->cellAt(cursor);
    QVERIFY(cell.isValid());
    QVERIFY(cell.row() == 0);
    QVERIFY(cell.column() == 1);

    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.position() == 3);
    fmt = cursor.charFormat();
    QVERIFY(fmt.objectIndex() == -1);
    cell = tab->cellAt(cursor);
    QVERIFY(cell.isValid());
    QVERIFY(cell.row() == 1);
    QVERIFY(cell.column() == 0);

    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.position() == 4);
    fmt = cursor.charFormat();
    QVERIFY(fmt.objectIndex() == -1);
    cell = tab->cellAt(cursor);
    QVERIFY(cell.isValid());
    QVERIFY(cell.row() == 1);
    QVERIFY(cell.column() == 1);

    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.position() == 5);
    fmt = cursor.charFormat();
    QVERIFY(fmt.objectIndex() == -1);
    cell = tab->cellAt(cursor);
    QVERIFY(!cell.isValid());

    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.position() == 5);

    // check we can't delete the cells with the cursor
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.position() == 1);
    cursor.deleteChar();
    QVERIFY(doc->toPlainText().length() == 5);
    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.position() == 2);
    cursor.deleteChar();
    QVERIFY(doc->toPlainText().length() == 5);
    cursor.deletePreviousChar();
    QVERIFY(cursor.position() == 2);
    QVERIFY(doc->toPlainText().length() == 5);

    QTextTable *table = cursor.currentTable();
    QVERIFY(table->rows() == 2);
    QVERIFY(table->columns() == 2);

    table->insertRows(2, 1);
    QVERIFY(table->rows() == 3);
    QVERIFY(table->columns() == 2);
    QVERIFY(doc->toPlainText().length() == 7);
    table->insertColumns(2, 2);
    QVERIFY(table->rows() == 3);
    QVERIFY(table->columns() == 4);
    QVERIFY(doc->toPlainText().length() == 13);

    table->resize(4, 5);
    QVERIFY(table->rows() == 4);
    QVERIFY(table->columns() == 5);
    QVERIFY(doc->toPlainText().length() == 21);
}

void tst_QTextTable::tableShrinking()
{
    QTextTableFormat tableFmt;

    cursor.insertTable(3, 4, tableFmt);
    QVERIFY(doc->toPlainText().length() == 13);

    QTextTable *table = cursor.currentTable();
    QVERIFY(table->rows() == 3);
    QVERIFY(table->columns() == 4);

    table->removeRows(1, 1);
    QVERIFY(table->rows() == 2);
    QVERIFY(table->columns() == 4);
    QVERIFY(doc->toPlainText().length() == 9);
    table->removeColumns(1, 2);
    QVERIFY(table->rows() == 2);
    QVERIFY(table->columns() == 2);
    QVERIFY(doc->toPlainText().length() == 5);

    table->resize(1, 1);
    QVERIFY(table->rows() == 1);
    QVERIFY(table->columns() == 1);
    QVERIFY(doc->toPlainText().length() == 2);
}

void tst_QTextTable::spans()
{
    QTextTableFormat tableFmt;

    cursor.insertTable(2, 2, tableFmt);

    QTextTable *table = cursor.currentTable();
    QVERIFY(table->cellAt(0, 0) != table->cellAt(0, 1));
    table->mergeCells(0, 0, 1, 2);
    QVERIFY(table->rows() == 2);
    QVERIFY(table->columns() == 2);
    QVERIFY(table->cellAt(0, 0) == table->cellAt(0, 1));
    table->mergeCells(0, 0, 2, 2);
    QVERIFY(table->rows() == 2);
    QVERIFY(table->columns() == 2);
}

void tst_QTextTable::variousModifications2()
{
    QTextTableFormat tableFmt;

    cursor.insertTable(2, 5, tableFmt);
    QVERIFY(doc->toPlainText().length() == 11);
    QTextTable *table = cursor.currentTable();
    QVERIFY(cursor.position() == 1);
    QVERIFY(table->rows() == 2);
    QVERIFY(table->columns() == 5);

    table->insertColumns(0, 1);
    QVERIFY(table->rows() == 2);
    QVERIFY(table->columns() == 6);
    table->insertColumns(6, 1);
    QVERIFY(table->rows() == 2);
    QVERIFY(table->columns() == 7);

    table->insertRows(0, 1);
    QVERIFY(table->rows() == 3);
    QVERIFY(table->columns() == 7);
    table->insertRows(3, 1);
    QVERIFY(table->rows() == 4);
    QVERIFY(table->columns() == 7);

    table->removeRows(0, 1);
    QVERIFY(table->rows() == 3);
    QVERIFY(table->columns() == 7);
    table->removeRows(2, 1);
    QVERIFY(table->rows() == 2);
    QVERIFY(table->columns() == 7);

    table->removeColumns(0, 1);
    QVERIFY(table->rows() == 2);
    QVERIFY(table->columns() == 6);
    table->removeColumns(5, 1);
    QVERIFY(table->rows() == 2);
    QVERIFY(table->columns() == 5);
}

void tst_QTextTable::tableManager_undo()
{
    QTextTableFormat fmt;
    fmt.setBorder(10);
    QTextTable *table = cursor.insertTable(2, 2, fmt);
    QVERIFY(table);

    QVERIFY(table->format().border() == 10);

    fmt.setBorder(20);
    table->setFormat(fmt);

    QVERIFY(table->format().border() == 20);

    doc->undo();

    QVERIFY(table->format().border() == 10);
}

void tst_QTextTable::tableManager_removeCell()
{
    // essentially a test for TableManager::removeCell, in particular to remove empty items from the rowlist.
    // If it fails it'll triger assertions inside TableManager. Yeah, not pretty, should VERIFY here ;(
    cursor.insertTable(2, 2, QTextTableFormat());
    doc->undo();
    // ###
    QVERIFY(true);
}

void tst_QTextTable::rowAt()
{
    // test TablePrivate::rowAt
    QTextTable *table = cursor.insertTable(4, 2);

    QCOMPARE(table->rows(), 4);
    QCOMPARE(table->columns(), 2);

    QTextCursor cell00Cursor = table->cellAt(0, 0).firstCursorPosition();
    QTextCursor cell10Cursor = table->cellAt(1, 0).firstCursorPosition();
    QTextCursor cell20Cursor = table->cellAt(2, 0).firstCursorPosition();
    QTextCursor cell21Cursor = table->cellAt(2, 1).firstCursorPosition();
    QTextCursor cell30Cursor = table->cellAt(3, 0).firstCursorPosition();
    QVERIFY(table->cellAt(cell00Cursor).firstCursorPosition() == cell00Cursor);
    QVERIFY(table->cellAt(cell10Cursor).firstCursorPosition() == cell10Cursor);
    QVERIFY(table->cellAt(cell20Cursor).firstCursorPosition() == cell20Cursor);
    QVERIFY(table->cellAt(cell30Cursor).firstCursorPosition() == cell30Cursor);

    table->mergeCells(1, 0, 2, 1);

    QCOMPARE(table->rows(), 4);
    QCOMPARE(table->columns(), 2);

    QVERIFY(cell00Cursor == table->cellAt(0, 0).firstCursorPosition());
    QVERIFY(cell10Cursor == table->cellAt(1, 0).firstCursorPosition());
    QVERIFY(cell10Cursor == table->cellAt(2, 0).firstCursorPosition());
    QVERIFY(cell21Cursor == table->cellAt(2, 1).firstCursorPosition());
    QVERIFY(cell30Cursor == table->cellAt(3, 0).firstCursorPosition());

    table->mergeCells(1, 0, 2, 2);

    QCOMPARE(table->rows(), 4);
    QCOMPARE(table->columns(), 2);

    QVERIFY(cell00Cursor == table->cellAt(0, 0).firstCursorPosition());
    QVERIFY(cell00Cursor == table->cellAt(0, 0).firstCursorPosition());
    QVERIFY(cell10Cursor == table->cellAt(1, 0).firstCursorPosition());
    QVERIFY(cell10Cursor == table->cellAt(1, 1).firstCursorPosition());
    QVERIFY(cell10Cursor == table->cellAt(2, 0).firstCursorPosition());
    QVERIFY(cell10Cursor == table->cellAt(2, 1).firstCursorPosition());
    QVERIFY(cell30Cursor == table->cellAt(3, 0).firstCursorPosition());
}

void tst_QTextTable::rowAtWithSpans()
{
    QTextTable *table = cursor.insertTable(2, 2);

    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 2);

    table->mergeCells(0, 0, 2, 1);
    QVERIFY(table->cellAt(0, 0).rowSpan() == 2);

    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 2);

    table->mergeCells(0, 0, 2, 2);
    QVERIFY(table->cellAt(0, 0).columnSpan() == 2);

    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 2);
}

void tst_QTextTable::multiBlockCells()
{
    // little testcase for multi-block cells
    QTextTable *table = cursor.insertTable(2, 2);

    QVERIFY(cursor == table->cellAt(0, 0).firstCursorPosition());

    cursor.insertText("Hello");
    cursor.insertBlock(QTextBlockFormat());
    cursor.insertText("World");

    cursor.movePosition(QTextCursor::Left);
    QVERIFY(table->cellAt(0, 0) == table->cellAt(cursor));
}

void tst_QTextTable::insertRows()
{
    // little testcase for multi-block cells
    QTextTable *table = cursor.insertTable(2, 2);

    QVERIFY(cursor == table->cellAt(0, 0).firstCursorPosition());

    table->insertRows(0, 1);
    QVERIFY(table->rows() == 3);

    table->insertRows(1, 1);
    QVERIFY(table->rows() == 4);

    table->insertRows(-1, 1);
    QVERIFY(table->rows() == 5);

    table->insertRows(5, 2);
    QVERIFY(table->rows() == 7);

}

void tst_QTextTable::deleteInTable()
{
    QTextTable *table = cursor.insertTable(2, 2);
    table->cellAt(0, 0).firstCursorPosition().insertText("Blah");
    table->cellAt(0, 1).firstCursorPosition().insertText("Foo");
    table->cellAt(1, 0).firstCursorPosition().insertText("Bar");
    table->cellAt(1, 1).firstCursorPosition().insertText("Hah");

    cursor = table->cellAt(1, 1).firstCursorPosition();
    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);

    QCOMPARE(table->cellAt(cursor.position()).row(), 1);
    QCOMPARE(table->cellAt(cursor.position()).column(), 0);

    cursor.removeSelectedText();

    QCOMPARE(table->columns(), 2);
    QCOMPARE(table->rows(), 2);

    // verify table is still all in shape. Only the text inside should get deleted
    for (int row = 0; row < table->rows(); ++row)
        for (int col = 0; col < table->columns(); ++col) {
            const QTextTableCell cell = table->cellAt(row, col);
            QVERIFY(cell.isValid());
            QCOMPARE(cell.rowSpan(), 1);
            QCOMPARE(cell.columnSpan(), 1);
        }
}

QTextTable *tst_QTextTable::create2x2Table()
{
    cleanup();
    init();
    QTextTable *table = cursor.insertTable(2, 2);
    table->cellAt(0, 0).firstCursorPosition().insertText("Blah");
    table->cellAt(0, 1).firstCursorPosition().insertText("Foo");
    table->cellAt(1, 0).firstCursorPosition().insertText("Bar");
    table->cellAt(1, 1).firstCursorPosition().insertText("Hah");
    return table;
}

QTextTable *tst_QTextTable::create4x4Table()
{
    cleanup();
    init();
    QTextTable *table = cursor.insertTable(4, 4);
    table->cellAt(0, 0).firstCursorPosition().insertText("Blah");
    table->cellAt(0, 1).firstCursorPosition().insertText("Foo");
    table->cellAt(1, 0).firstCursorPosition().insertText("Bar");
    table->cellAt(1, 1).firstCursorPosition().insertText("Hah");
    return table;
}

QTextTable *tst_QTextTable::createTable(int rows, int cols)
{
    cleanup();
    init();
    QTextTable *table = cursor.insertTable(rows, cols);
    return table;
}

void tst_QTextTable::mergeCells()
{
    QTextTable *table = create4x4Table();

    table->mergeCells(1, 1, 1, 2);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));

    table->mergeCells(1, 1, 2, 2);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 2));

    table = create4x4Table();

    table->mergeCells(1, 1, 2, 1);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));

    table->mergeCells(1, 1, 2, 2);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 2));

    table = create4x4Table();

    table->mergeCells(1, 1, 2, 2);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 2));

    // should do nothing
    table->mergeCells(1, 1, 1, 1);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 2));

    table = create2x2Table();

    table->mergeCells(0, 1, 2, 1);
    table->mergeCells(0, 0, 2, 2);
    QVERIFY(table->cellAt(0, 0) == table->cellAt(0, 1));
    QVERIFY(table->cellAt(0, 0) == table->cellAt(1, 0));
    QVERIFY(table->cellAt(0, 0) == table->cellAt(1, 1));

    QTextBlock block = table->cellAt(0, 0).firstCursorPosition().block();

    QVERIFY(block.text() == "Blah Foo");
    QVERIFY(block.next().text() == "Hah");
    QVERIFY(block.next().next().text() == "Bar");

    table = create4x4Table();

    QTextCursor cursor = table->cellAt(3, 3).firstCursorPosition();
    QTextTable *t2 = cursor.insertTable(2, 2);
    t2->cellAt(0, 0).firstCursorPosition().insertText("Test");

    table->mergeCells(2, 2, 2, 2);
    cursor = table->cellAt(2, 2).firstCursorPosition();

    QTextFrame *frame = cursor.currentFrame();

    QTextFrame::iterator it = frame->begin();

    // find the embedded table
    while (it != frame->end() && !it.currentFrame())
        ++it;

    table = qobject_cast<QTextTable *>(it.currentFrame());

    QVERIFY(table);

    if (table) {
        cursor = table->cellAt(0, 0).firstCursorPosition();

        QVERIFY(cursor.block().text() == "Test");
    }

    table = create2x2Table();

    table->mergeCells(0, 1, 2, 1);

    QVERIFY(table->cellAt(0, 0) != table->cellAt(0, 1));
    QVERIFY(table->cellAt(0, 1) == table->cellAt(1, 1));

    // should do nothing
    table->mergeCells(0, 0, 1, 2);

    QVERIFY(table->cellAt(0, 0) != table->cellAt(0, 1));
    QVERIFY(table->cellAt(0, 1) == table->cellAt(1, 1));
}

void tst_QTextTable::splitCells()
{
    QTextTable *table = create4x4Table();
    table->mergeCells(1, 1, 2, 2);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 2));

    table->splitCell(1, 1, 1, 2);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 2));

    table->splitCell(1, 1, 1, 1);
    QVERIFY(table->cellAt(1, 1) != table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 2));


    table = create4x4Table();
    table->mergeCells(1, 1, 2, 2);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 2));

    table->splitCell(1, 1, 2, 1);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 2));

    table->splitCell(1, 1, 1, 1);
    QVERIFY(table->cellAt(1, 1) != table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 2));


    table = create4x4Table();
    table->mergeCells(1, 1, 2, 2);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 2));

    table->splitCell(1, 1, 1, 1);
    QVERIFY(table->cellAt(1, 1) != table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 2));

    table = createTable(2, 5);
    table->mergeCells(0, 0, 2, 1);
    table->mergeCells(0, 1, 2, 1);
    QVERIFY(table->cellAt(0, 0) == table->cellAt(1, 0));
    QVERIFY(table->cellAt(0, 1) == table->cellAt(1, 1));
    table->splitCell(0, 0, 1, 1);
    QVERIFY(table->cellAt(0, 0) != table->cellAt(1, 0));
    QVERIFY(table->cellAt(0, 1) == table->cellAt(1, 1));

    table = createTable(2, 5);
    table->mergeCells(0, 4, 2, 1);
    QVERIFY(table->cellAt(0, 4) == table->cellAt(1, 4));

    table->splitCell(0, 4, 1, 1);
    QVERIFY(table->cellAt(0, 4) != table->cellAt(1, 4));
}

void tst_QTextTable::blocksForTableShouldHaveEmptyFormat()
{
    QTextBlockFormat fmt;
    fmt.setProperty(QTextFormat::UserProperty, true);
    cursor.insertBlock(fmt);
    QVERIFY(cursor.blockFormat().hasProperty(QTextFormat::UserProperty));

    QTextTable *table = cursor.insertTable(1, 1);
    QVERIFY(!table->cellAt(0, 0).firstCursorPosition().blockFormat().hasProperty(QTextFormat::UserProperty));

    int userPropCount = 0;
    for (QTextBlock block = doc->begin();
         block.isValid(); block = block.next()) {
        if (block.blockFormat().hasProperty(QTextFormat::UserProperty))
            userPropCount++;
    }
    QCOMPARE(userPropCount, 1);
}

void tst_QTextTable::removeTableByRemoveRows()
{
    QPointer<QTextTable> table1 = QTextCursor(cursor).insertTable(4, 4);
    QPointer<QTextTable> table2 = QTextCursor(cursor).insertTable(4, 4);
    QPointer<QTextTable> table3 = QTextCursor(cursor).insertTable(4, 4);

    QVERIFY(table1);
    QVERIFY(table2);
    QVERIFY(table3);

    table2->removeRows(1, 1);

    QVERIFY(table1);
    QVERIFY(table2);
    QVERIFY(table3);

    table2->removeRows(0, table2->rows());

    QVERIFY(table1);
    QVERIFY(!table2);
    QVERIFY(table3);
}

void tst_QTextTable::removeTableByRemoveColumns()
{
    QPointer<QTextTable> table1 = QTextCursor(cursor).insertTable(4, 4);
    QPointer<QTextTable> table2 = QTextCursor(cursor).insertTable(4, 4);
    QPointer<QTextTable> table3 = QTextCursor(cursor).insertTable(4, 4);

    QVERIFY(table1);
    QVERIFY(table2);
    QVERIFY(table3);

    table2->removeColumns(1, 1);

    QVERIFY(table1);
    QVERIFY(table2);
    QVERIFY(table3);

    table2->removeColumns(0, table2->columns());

    QVERIFY(table1);
    QVERIFY(!table2);
    QVERIFY(table3);
}

void tst_QTextTable::setCellFormat()
{
    QTextTable *table = cursor.insertTable(2, 2);
    table->cellAt(0, 0).firstCursorPosition().insertText("First");
    table->cellAt(0, 1).firstCursorPosition().insertText("Second");
    table->cellAt(1, 0).firstCursorPosition().insertText("Third");
    table->cellAt(1, 1).firstCursorPosition().insertText("Fourth");
    QTextTableCell cell = table->cellAt(0, 0);
    QTextCharFormat fmt;
    fmt.setObjectIndex(23);
    fmt.setBackground(Qt::blue);
    cell.setFormat(fmt);
    QVERIFY(cell.format().background().color() == QColor(Qt::blue));
}

QTEST_MAIN(tst_QTextTable)
#include "tst_qtexttable.moc"
