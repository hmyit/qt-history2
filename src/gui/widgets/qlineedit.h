/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QLINEEDIT_H
#define QLINEEDIT_H

class QValidator;
class QMenu;

#include "qframe.h"
#include "qstring.h"

#ifndef QT_NO_LINEEDIT

class QLineEditPrivate;

class Q_GUI_EXPORT QLineEdit : public QWidget
{
    Q_OBJECT

    Q_ENUMS(EchoMode)
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(int maxLength READ maxLength WRITE setMaxLength)
    Q_PROPERTY(bool frame READ hasFrame WRITE setFrame)
    Q_PROPERTY(EchoMode echoMode READ echoMode WRITE setEchoMode)
    Q_PROPERTY(QString displayText READ displayText)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
    Q_PROPERTY(bool modified READ isModified WRITE setModified DESIGNABLE false)
    Q_PROPERTY(bool hasSelectedText READ hasSelectedText)
    Q_PROPERTY(QString selectedText READ selectedText)
    Q_PROPERTY(bool dragEnabled READ dragEnabled WRITE setDragEnabled)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY(bool undoAvailable READ isUndoAvailable)
    Q_PROPERTY(bool redoAvailable READ isRedoAvailable)
    Q_PROPERTY(QString inputMask READ inputMask WRITE setInputMask)
    Q_PROPERTY(bool acceptableInput READ hasAcceptableInput)
    Q_PROPERTY(bool contextMenuEnabled READ contextMenuEnabled WRITE setContextMenuEnabled)

public:
    QLineEdit(QWidget* parent=0);
    QLineEdit(const QString &, QWidget* parent=0);
#ifdef QT_COMPAT
    QT_COMPAT_CONSTRUCTOR QLineEdit(QWidget* parent, const char* name);
    QT_COMPAT_CONSTRUCTOR QLineEdit(const QString &, QWidget* parent, const char* name);
    QT_COMPAT_CONSTRUCTOR QLineEdit(const QString &, const QString &, QWidget* parent=0, const char* name=0);
#endif
    ~QLineEdit();

    QString text() const;

    QString displayText() const;

    int maxLength() const;
    void setMaxLength(int);

    void setFrame(bool);
    bool hasFrame() const;

    enum EchoMode { Normal, NoEcho, Password };
    EchoMode echoMode() const;
    void setEchoMode(EchoMode);

    bool isReadOnly() const;
    void setReadOnly(bool);

    void setValidator(const QValidator *);
    const QValidator * validator() const;

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    int cursorPosition() const;
    void setCursorPosition(int);
    int cursorPositionAt(const QPoint &pos);

    void setAlignment(Qt::Alignment flag);
    Qt::Alignment alignment() const;

    void cursorForward(bool mark, int steps = 1);
    void cursorBackward(bool mark, int steps = 1);
    void cursorWordForward(bool mark);
    void cursorWordBackward(bool mark);
    void backspace();
    void del();
    void home(bool mark);
    void end(bool mark);

    bool isModified() const;
    void setModified(bool);

    void setSelection(int, int);
    bool hasSelectedText() const;
    QString selectedText() const;
    int selectionStart() const;

    bool isUndoAvailable() const;
    bool isRedoAvailable() const;

    void setDragEnabled(bool b);
    bool dragEnabled() const;

    QString inputMask() const;
    void setInputMask(const QString &inputMask);
    bool hasAcceptableInput() const;

    void setContextMenuEnabled(bool enabled);
    bool contextMenuEnabled() const;

public slots:
    void setText(const QString &);
    void clear();
    void selectAll();
    void undo();
    void redo();
#ifndef QT_NO_CLIPBOARD
    void cut();
    void copy() const;
    void paste();
#endif

public:
    void deselect();
    void insert(const QString &);

signals:
    void textChanged(const QString &);
    void cursorPositionChanged(int, int);
    void returnPressed();
    void lostFocus();
    void selectionChanged();

protected:
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *);
    void focusInEvent(QFocusEvent *);
    void focusOutEvent(QFocusEvent *);
    void paintEvent(QPaintEvent *);
#ifndef QT_NO_DRAGANDDROP
    void dragEnterEvent(QDragEnterEvent *);
    void dragMoveEvent(QDragMoveEvent *e);
    void dragLeaveEvent(QDragLeaveEvent *e);
    void dropEvent(QDropEvent *);
#endif
    void changeEvent(QEvent *);
    void contextMenuEvent(QContextMenuEvent *);
    virtual QMenu *createPopupMenu();
#ifdef QT_COMPAT
    inline QT_COMPAT void repaintArea(int, int) { update(); }
#endif

    void inputMethodEvent(QInputMethodEvent *);
public:
    QVariant inputMethodQuery(Qt::InputMethodQuery) const;
    bool event(QEvent *);
protected:

public:
#ifdef QT_COMPAT
    inline QT_COMPAT void clearModified() { setModified(false); }
    inline QT_COMPAT void cursorLeft(bool mark, int steps = 1) { cursorForward(mark, -steps); }
    inline QT_COMPAT void cursorRight(bool mark, int steps = 1) { cursorForward(mark, steps); }
    QT_COMPAT bool validateAndSet(const QString &, int, int, int);
    inline QT_COMPAT bool frame() const { return hasFrame(); }
    inline QT_COMPAT void clearValidator() { setValidator(0); }
    inline QT_COMPAT bool hasMarkedText() const { return hasSelectedText(); }
    inline QT_COMPAT QString markedText() const { return selectedText(); }
    QT_COMPAT bool edited() const;
    QT_COMPAT void setEdited(bool);
    QT_COMPAT int characterAt(int, QChar*) const;
    QT_COMPAT bool getSelection(int *, int *);
#endif

private:
    Q_DISABLE_COPY(QLineEdit)
    Q_DECLARE_PRIVATE(QLineEdit)
    Q_PRIVATE_SLOT(d, void clipboardChanged())
};

#endif // QT_NO_LINEEDIT

#endif // QLINEEDIT_H
