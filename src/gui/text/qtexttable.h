#ifndef QTEXTTABLE_H
#define QTEXTTABLE_H

#ifndef QT_H
#include <qglobal.h>
#include <qsharedpointer.h>
#include <qobject.h>
#include "qtextcursor.h"
#endif // QT_H

class QTextTablePrivate;
class QTextPieceTable;

class Q_GUI_EXPORT QTextTableCellProperties
{
public:
    QTextTableCellProperties();
    ~QTextTableCellProperties();

    inline int row() const { return r; }
    inline int col() const { return c; }

    inline int rowSpan() const { return rSpan; }
    inline int colSpan() const { return cSpan; }

    inline bool isValid() const { return r > 0 && c > 0; }

    inline QTextCursor start() const { return s; }
    inline QTextCursor end() const { return e; }

    inline bool operator==(const QTextTableCellProperties &other) const
    { return r == other.r && c == other.c; }
    inline bool operator!=(const QTextTableCellProperties &other) const
    { return !operator==(other); }

private:
    friend class QTextTable;
    QTextTableCellProperties(QTextTablePrivate *p, QTextPieceTable *pt, int row, int col);

    int r, c;
    QTextCursor s;
    QTextCursor e;
    int rSpan;
    int cSpan;
};

class Q_GUI_EXPORT QTextTable : public QObject
{
    Q_OBJECT
public:
    ~QTextTable();

    void resize(int rows, int cols);
    void insertRows(int pos, int num);
    void insertCols(int pos, int num);
    void removeRows(int pos, int num);
    void removeCols(int pos, int num);

    int rows() const;
    int cols() const;

    void setRowSpan(int row, int col, int rowspan);
    void setColSpan(int row, int col, int colspan);

    QTextTableCellProperties cellAt(int row, int col) const;
    QTextTableCellProperties cellAt(const QTextCursor &c) const;

    QTextCursor rowStart(const QTextCursor &c) const;
    QTextCursor rowEnd(const QTextCursor &c) const;
    QTextCursor start() const;
    QTextCursor end() const;

    void setFormat(const QTextTableFormat &format);
    QTextTableFormat format() const;

    inline bool operator == (const QTextTable &o) {
	return (d == o.d);
    }
    inline bool operator != (const QTextTable &o) {
	return (d != o.d);
    }

private:
    friend class QTextCursor;
    friend class QTextCursorPrivate;
    friend class QTextTableManager;

    QTextTable(QTextPieceTable *pt, QObject *parent);

    QTextTablePrivate *d;
    QTextPieceTable *pieceTable;
#if defined(Q_DISABLE_COPY)
    QTextTable(const QTextTable &o);
    QTextTable & operator =(const QTextTable &o);
#endif
};

#endif
