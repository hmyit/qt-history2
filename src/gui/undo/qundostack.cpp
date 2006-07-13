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

#include <QtCore/qdebug.h>
#include "qundostack.h"
#include "qundogroup.h"
#include "qundostack_p.h"

#ifndef QT_NO_UNDOCOMMAND

/*!
    \class QUndoCommand
    \brief The QUndoCommand class is the base class of all commands stored on a QUndoStack.
    \since 4.2
    \ingroup miscellaneous

    For an overview of Qt's Undo Framework, see the
    \l{Overview of Qt's Undo Framework}{overview document}.

    A QUndoCommand represents a single editing action on a document; for example,
    inserting or deleting a block of text in a text editor. QUndoCommand can apply
    a change to the document with redo() and undo the change with undo(). The
    implementations for these functions must be provided in a derived class.

    \code
    class AppendText : public QUndoCommand
    {
    public:
        AppendText(QString *doc, const QString &text)
            : m_document(doc), m_text(text) { setText("append text"); }
        virtual void undo()
            { m_document->chop(m_text.length()); }
        virtual void redo()
            { m_document->append(m_text); }
    private:
        QString *m_document;
        QString m_text;
    };
    \endcode

    A QUndoCommand has an associated text(). This is a short string
    describing what the command does. It is used to update the text
    properties of the stack's undo and redo actions; see
    QUndoStack::createUndoAction() and QUndoStack::createRedoAction().

    To support command compression, QUndoCommand has an id() and the virtual function
    mergeWith(). These functions are used by QUndoStack::push().

    To support command macros, a QUndoCommand object can have any number of child
    commands. Undoing or redoing the parent command will cause the child
    commands to be undone or redone. A command can be assigned
    to a parent explicitly in the constructor.

    \code
    QUndoCommand *insertRed = new QUndoCommand(); // an empty command
    insertRed->setText("insert red text");

    new InsertText(document, idx, text, insertRed); // becomes child of insertRed
    new SetColor(document, idx, text.length(), Qt::red, insertRed);

    stack.push(insertRed);
    \endcode

    Another way to create macros is to use the convenience functions
    QUndoStack::beginMacro() and QUndoStack::endMacro().

    \sa QUndoStack
*/

/*!
    Constructs a QUndoCommand object with parent \a parent and text \a text.

    If \a parent is not 0, this command is appended to parent's child list.
    The parent command then owns this command and will delete it in its
    destructor.

    \sa ~QUndoCommand()
*/

QUndoCommand::QUndoCommand(const QString &text, QUndoCommand *parent)
{
    d = new QUndoCommandPrivate;
    if (parent != 0)
        parent->d->child_list.append(this);
    d->text = text;
}

/*!
    Constructs a QUndoCommand object with parent \a parent.

    If \a parent is not 0, this command is appended to parent's child list.
    The parent command then owns this command and will delete it in its
    destructor.

    \sa ~QUndoCommand()
*/

QUndoCommand::QUndoCommand(QUndoCommand *parent)
{
    d = new QUndoCommandPrivate;
    if (parent != 0)
        parent->d->child_list.append(this);
}

/*!
    Destroys the QUndoCommand object and all child commands. If the command was in a QUndoGroup,
    removes it grom the group.

    \sa QUndoCommand()
*/

QUndoCommand::~QUndoCommand()
{
    qDeleteAll(d->child_list);
    delete d;
}

/*!
    Returns the ID of this command.

    A command ID is used in command compression. It must be an integer unique to
    this command's class, or -1 if the command doesn't support compression.

    If the command supports compression this function must be overriden in the
    derived class to return the correct ID. The base implementation returns -1.

    QUndoStack will only try to merge two commands if they have the same ID, and
    the ID is not -1.

    \sa mergeWith(), QUndoStack::push()
*/

int QUndoCommand::id() const
{
    return -1;
}

/*!
    Attempts to merge this command with the specified \a command. Returns true on
    success; otherwise returns false.

    If this function returns true, calling this command's redo() must have the same
    effect as redoing both this command and the specified \a command.
    Similarly, calling this command's undo() must have the same effect as undoing
    both the specified \a command and this command.

    QUndoStack will only try to merge two commands if they have the same id, and
    the id is not -1.

    The default implementation returns false.

    \code
    bool AppendText::mergeWith(const QUndoCommand *other)
    {
        if (other->id() != id()) // make sure other is also an AppendText command
            return false;
        m_text += static_cast<const AppendText*>(other)->m_text;
        return true;
    }
    \endcode

    \sa id() QUndoStack::push()
*/

bool QUndoCommand::mergeWith(const QUndoCommand *command)
{
    Q_UNUSED(command);
    return false;
}

/*!
    Applies a change to the document. This function must be implemented in
    the derived class.

    The default implementation calls redo() on all child commands.

    \sa undo()
*/

void QUndoCommand::redo()
{
    for (int i = 0; i < d->child_list.size(); ++i)
        d->child_list.at(i)->redo();
}

/*!
    Reverts a change to the document. After undo() is called, the state of
    the document should be the same as before redo() was called. This function must
    be implemented in the derived class.

    The default implementation calls undo() on all child commands in reverse order.

    \sa redo()
*/

void QUndoCommand::undo()
{
    for (int i = d->child_list.size() - 1; i >= 0; --i)
        d->child_list.at(i)->undo();
}

/*!
    Returns a short text string describing what this command does, f.ex. "insert text".
    It is used to update the text properties of the stack's undo and redo actions.

    \sa setText() QUndoStack::createUndoAction() QUndoStack::createRedoAction()
*/

QString QUndoCommand::text() const
{
    return d->text;
}

/*!
    Sets the command's text to be the \a text specified.

    The specified text should be a short user-readable string describing what this
    command does.

    \sa text() QUndoStack::createUndoAction() QUndoStack::createRedoAction()
*/

void QUndoCommand::setText(const QString &text)
{
    d->text = text;
}

#endif // QT_NO_UNDOCOMMAND

#ifndef QT_NO_UNDOSTACK

/*!
    \class QUndoStack
    \brief The QUndoStack class is a stack of QUndoCommand objects.
    \since 4.2
    \ingroup miscellaneous

    For an overview of Qt's Undo Framework, see the
    \l{Overview of Qt's Undo Framework}{overview document}.

    An undo stack maintains a stack of commands that have been applied to a
    document.

    New commands are pushed on the stack using push(). Commands can be
    undone and redone using undo() and redo(), or by triggering the
    actions returned by createUndoAction() and createRedoAction().

    QUndoStack keeps track of the \a current command. This is the command
    which will be executed by the next call to redo(). The index of this
    command is returned by index(). The state of the edited object can be
    rolled forward or back using setIndex(). If the top-most command on the
    stack has already been redone, index() is equal to count().

    QUndoStack provides support for undo and redo actions, command
    compression, command macros, and supports the concept of a
    \e{clean state}.

    \section1 Undo and Redo Actions

    QUndoStack provides convenient undo and redo QAction objects, which
    can be inserted into a menu or a toolbar. When commands are undone or
    redone, QUndoStack updates the text properties of these actions
    to reflect what change they will trigger. The actions are also disabled
    when no command is available for undo or redo. These actions
    are returned by QUndoStack::createUndoAction() and QUndoStack::createRedoAction().

    \section1 Command Compression and Macros

    Command compression is useful when several commands can be compressed
    into a single command that can be undone and redone in a single operation.
    For example, when a user types a character in a text editor, a new command
    is created. This command inserts the character into the document at the
    cursor position. However, it is more convenient for the user to be able
    to undo or redo typing of whole words, sentences, or paragraphs.
    Command compression allows these single-character commands to be merged
    into a single command which inserts or deletes sections of text.
    For more information, see QUndoCommand::mergeWith() and push().

    A command macro is a sequence of commands, all of which are undone and
    redone in one go. Command macros are created by giving a command a list
    of child commands.
    Undoing or redoing the parent command will cause the child commands to
    be undone or redone. Command macros may be created explicitly
    by specifying a parent in the QUndoCommand constructor, or by using the
    convenience functions beginMacro() and endMacro().

    Although command compression and macros appear to have the effect to the
    user, they often have different uses in an application. Commands that
    perform small changes to a document may be usefully compressed if there is
    no need to individually record them, and if only larger changes are relevant
    to the user.
    However, for commands that need to be recorded individually, or those that
    cannot be compressed, it is useful to use macros to provide a more convenient
    user experience while maintaining a record of each command.

    \section1 Clean State

    QUndoStack supports the concept of a clean state. When the
    document is saved to disk, the stack can be marked as clean using
    setClean(). Whenever the stack returns to this state through the use
    of undo/redo commands, it emits the signal cleanChanged(), which
    is also emitted when the stack leaves the clean state. This signal is
    usually used to enable and disable the save actions in the application,
    and to update the document's title to reflect that it contains unsaved
    changes.

    \sa QUndoCommand, QUndoView
*/

QUndoAction::QUndoAction(const QString &prefix, QObject *parent)
    : QAction(parent)
{
    m_prefix = prefix;
}

void QUndoAction::setPrefixedText(const QString &text)
{
    QString s = m_prefix;
    if (!m_prefix.isEmpty() && !text.isEmpty())
        s.append(QLatin1Char(' '));
    s.append(text);
    setText(s);
}

/*! \internal
    Sets the current index to \a idx, emitting appropriate signals. If \a clean is true,
    makes \a idx the clean index as well.
*/

void QUndoStackPrivate::setIndex(int idx, bool clean)
{
    Q_Q(QUndoStack);

    bool was_clean = index == clean_index;

    if (idx != index) {
        index = idx;
        emit q->indexChanged(index);
        emit q->canUndoChanged(q->canUndo());
        emit q->undoTextChanged(q->undoText());
        emit q->canRedoChanged(q->canRedo());
        emit q->redoTextChanged(q->redoText());
    }

    if (clean)
        clean_index = index;

    bool is_clean = index == clean_index;
    if (is_clean != was_clean)
        emit q->cleanChanged(is_clean);
}

/*!
    Constructs an empty undo stack with the parent \a parent. The
    stack will initally be in the clean state. If \a parent is a
    QUndoGroup object, the stack is automatically added to the group.

    \sa push()
*/

QUndoStack::QUndoStack(QObject *parent)
    : QObject(*(new QUndoStackPrivate), parent)
{
#ifndef QT_NO_UNDOGROUP
    if (QUndoGroup *group = qobject_cast<QUndoGroup*>(parent))
        group->addStack(this);
#endif
}

/*!
    Destroys the undo stack, deleting any commands that are on it. If the
    stack is in a QUndoGroup, the stack is automatically removed from the group.

    \sa QUndoStack()
*/

QUndoStack::~QUndoStack()
{
#ifndef QT_NO_UNDOGROUP
    Q_D(QUndoStack);
    if (d->group != 0)
        d->group->removeStack(this);
#endif
    clear();
}

/*!
    Clears the command stack by deleting all commands on it, and returns the stack
    to the clean state.

    Commands are not undone or redone; the state of the edited object remains
    unchanged.

    This function is usually used when the contents of the document are
    abandoned.

    \sa QUndoStack()
*/

void QUndoStack::clear()
{
    Q_D(QUndoStack);

    d->macro_stack.clear();
    qDeleteAll(d->command_list);
    d->command_list.clear();
    d->setIndex(0, true);
}

/*!
    Pushes \a cmd on the stack or merges it with the most recently executed command.
    In either case, executes \a cmd by calling its redo() function.

    If \a cmd's id is not -1, and if the id is the same as that of the
    most recently executed command, QUndoStack will attempt to merge the two
    commands by calling QUndoCommand::mergeWith() on the most recently executed
    command. If QUndoCommand::mergeWith() returns true, the \a cmd is deleted.

    In all other cases \a cmd is simply pushed on the stack.

    If the current command index does not point to the top of the stack - ie.
    if commands were undone before \a cmd was pushed - the current command and
    all commands above it are deleted. Hence \a cmd always ends up being the
    top-most command on the stack.

    Once a command is pushed, the stack takes ownership of it. There
    are no getters to return the command, since modifying it after it has
    been executed will almost always lead to corruption of the document's
    state.

    \sa QUndoCommand::id() QUndoCommand::mergeWith()
*/

void QUndoStack::push(QUndoCommand *cmd)
{
    Q_D(QUndoStack);
    cmd->redo();

    bool macro = !d->macro_stack.isEmpty();

    QUndoCommand *cur = 0;
    if (macro) {
        QUndoCommand *macro_cmd = d->macro_stack.last();
        if (!macro_cmd->d->child_list.isEmpty())
            cur = macro_cmd->d->child_list.last();
    } else {
        if (d->index > 0)
            cur = d->command_list.at(d->index - 1);
        while (d->index < d->command_list.size())
            delete d->command_list.takeLast();
        if (d->clean_index > d->index)
            d->clean_index = -1; // we've deleted the clean state
    }

    bool try_merge = cur != 0
                        && cur->id() != -1
                        && cur->id() == cmd->id()
                        && (macro || d->index != d->clean_index);

    if (try_merge && cur->mergeWith(cmd)) {
        delete cmd;
        if (!macro) {
            emit indexChanged(d->index);
            emit canUndoChanged(canUndo());
            emit undoTextChanged(undoText());
            emit canRedoChanged(canRedo());
            emit redoTextChanged(redoText());
        }
    } else {
        if (macro) {
            d->macro_stack.last()->d->child_list.append(cmd);
        } else {
            d->command_list.append(cmd);
            d->setIndex(d->index + 1, false);
        }
    }
}

/*!
    Marks the stack as clean and emits cleanChanged() if the stack was
    not already clean.

    Whenever the stack returns to this state through the use of undo/redo
    commands, it emits the signal cleanChanged(). This signal is also
    emitted when the stack leaves the clean state.

    \sa isClean(), cleanIndex()
*/

void QUndoStack::setClean()
{
    Q_D(QUndoStack);
    if (!d->macro_stack.isEmpty()) {
        qWarning("QUndoStack::setClean(): cannot set clean in the middle of a macro");
        return;
    }

    d->setIndex(d->index, true);
}

/*!
    If the stack is in the clean state, returns true; otherwise returns false.

    \sa setClean() cleanIndex()
*/

bool QUndoStack::isClean() const
{
    Q_D(const QUndoStack);
    if (!d->macro_stack.isEmpty())
        return false;
    return d->clean_index == d->index;
}

/*!
    Returns the clean index. This is the index at which setClean() was called.

    A stack may not have a clean index. This happens if a document is saved,
    some commands are undone, then a new command is pushed. Since
    push() deletes all the undone commands before pushing the new command, the stack
    can't return to the clean state again. In this case, this function returns -1.

    \sa isClean() setClean()
*/

int QUndoStack::cleanIndex() const
{
    Q_D(const QUndoStack);
    return d->clean_index;
}

/*!
    Undoes the command below the current command by calling QUndoCommand::undo().
    Decrements the current command index.

    If the stack is empty, or if the bottom command on the stack has already been
    undone, this function does nothing.

    \sa redo() index()
*/

void QUndoStack::undo()
{
    Q_D(QUndoStack);
    if (d->index == 0)
        return;

    if (!d->macro_stack.isEmpty()) {
        qWarning("QUndoStack::undo(): cannot undo in the middle of a macro");
        return;
    }

    int idx = d->index - 1;
    d->command_list.at(idx)->undo();
    d->setIndex(idx, false);
}

/*!
    Redoes the current command by calling QUndoCommand::redo(). Increments the current
    command index.

    If the stack is empty, or if the top command on the stack has already been
    redone, this function does nothing.

    \sa undo() index()
*/

void QUndoStack::redo()
{
    Q_D(QUndoStack);
    if (d->index == d->command_list.size())
        return;

    if (!d->macro_stack.isEmpty()) {
        qWarning("QUndoStack::redo(): cannot redo in the middle of a macro");
        return;
    }

    d->command_list.at(d->index)->redo();
    d->setIndex(d->index + 1, false);
}

/*!
    Returns the number of commands on the stack. Macro commands are counted as
    one command.

    \sa index() setIndex()
*/

int QUndoStack::count() const
{
    Q_D(const QUndoStack);
    return d->command_list.size();
}

/*!
    Returns the index of the current command. This is the command that will be
    executed on the next call to redo(). It is not always the top-most command
    on the stack, since a number of commands may have been undone.

    \sa undo() redo() count()
*/

int QUndoStack::index() const
{
    Q_D(const QUndoStack);
    return d->index;
}

/*!
    Repeatedly calls undo() or redo() until the the current command index reaches
    \a idx. This function can be used to roll the state of the document forwards
    of backwards. indexChanged() is emitted only once.

    \sa index() count() undo() redo()
*/

void QUndoStack::setIndex(int idx)
{
    Q_D(QUndoStack);
    if (!d->macro_stack.isEmpty()) {
        qWarning("QUndoStack::setIndex(): cannot set index in the middle of a macro");
        return;
    }

    if (idx < 0)
        idx = 0;
    else if (idx > d->command_list.size())
        idx = d->command_list.size();

    int i = d->index;
    while (i < idx)
        d->command_list.at(i++)->redo();
    while (i > idx)
        d->command_list.at(--i)->undo();

    d->setIndex(idx, false);
}

/*!
    Returns true if there is a command available for undo; otherwise returns false.

    This function returns false if the stack is empty, or if the bottom command
    on the stack has already been undone.

    Synonymous with index() == 0.

    \sa index() canRedo()
*/

bool QUndoStack::canUndo() const
{
    Q_D(const QUndoStack);
    if (!d->macro_stack.isEmpty())
        return false;
    return d->index > 0;
}

/*!
    Returns true if there is a command available for redo; otherwise returns false.

    This function returns false if the stack is empty or if the top command
    on the stack has already been redone.

    Synonymous with index() == count().

    \sa index() canUndo()
*/

bool QUndoStack::canRedo() const
{
    Q_D(const QUndoStack);
    if (!d->macro_stack.isEmpty())
        return false;
    return d->index < d->command_list.size();
}

/*!
    Returns the text of the command which will be undone in the next call to undo().

    \sa QUndoCommand::text() redoText()
*/

QString QUndoStack::undoText() const
{
    Q_D(const QUndoStack);
    if (!d->macro_stack.isEmpty())
        return QString();
    if (d->index > 0)
        return d->command_list.at(d->index - 1)->text();
    return QString();
}

/*!
    Returns the text of the command which will be redone in the next call to redo().

    \sa QUndoCommand::text() undoText()
*/

QString QUndoStack::redoText() const
{
    Q_D(const QUndoStack);
    if (!d->macro_stack.isEmpty())
        return QString();
    if (d->index < d->command_list.size())
        return d->command_list.at(d->index)->text();
    return QString();
}

/*!
    Creates an undo QAction object with the given \a parent.

    Triggering this action will cause a call to undo(). The text of this action
    is the text of the command which will be undone in the next call to undo(),
    prefixed by the specified \a prefix. If there is no command available for undo,
    this action will be disabled.

    If \a prefix is empty, the default prefix "Undo" is used.

    \sa createRedoAction(), canUndo(), QUndoCommand::text()
*/

QAction *QUndoStack::createUndoAction(QObject *parent, const QString &prefix) const
{
    QString pref = prefix.isEmpty() ? tr("Undo") : prefix;
    QUndoAction *result = new QUndoAction(pref, parent);
    result->setEnabled(canUndo());
    result->setPrefixedText(undoText());
    connect(this, SIGNAL(canUndoChanged(bool)),
            result, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(undoTextChanged(QString)),
            result, SLOT(setPrefixedText(QString)));
    connect(result, SIGNAL(triggered()), this, SLOT(undo()));
    return result;
}

/*!
    Creates an redo QAction object with the given \a parent.

    Triggering this action will cause a call to redo(). The text of this action
    is the text of the command which will be redone in the next call to redo(),
    prefixed by the specified \a prefix. If there is no command available for redo,
    this action will be disabled.

    If \a prefix is empty, the default prefix "Redo" is used.

    \sa createUndoAction(), canRedo(), QUndoCommand::text()
*/

QAction *QUndoStack::createRedoAction(QObject *parent, const QString &prefix) const
{
    QString pref = prefix.isEmpty() ? tr("Redo") : prefix;
    QUndoAction *result = new QUndoAction(pref, parent);
    result->setEnabled(canRedo());
    result->setPrefixedText(redoText());
    connect(this, SIGNAL(canRedoChanged(bool)),
            result, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(redoTextChanged(QString)),
            result, SLOT(setPrefixedText(QString)));
    connect(result, SIGNAL(triggered()), this, SLOT(redo()));
    return result;
}

/*!
    Begins composition of a macro command with text \a text.

    An empty command with text \a text is pushed on the stack. Any subsequent
    commands pushed on the stack will be appended to the empty command's children,
    until endMacro() is called.

    Calls to beginMacro() and endMacro() mey be nested, but every call to beginMacro()
    must have a matching call to endMacro().

    While a macro is composed, the stack is disabled. This means that:
    \list
    \i indexChanged() and cleanChanged() are not emitted,
    \i canUndo() and canRedo() return false,
    \i calling undo() or redo() has no effect,
    \i the undo/redo actions are disabled.
    \endlist
    The stack becomes enabled and appropriate signals are emitted when endMacro() is called
    for the outermost macro.

    \code
    stack.beginMacro("insert red text");
    stack.push(new InsertText(document, idx, text));
    stack.push(new SetColor(document, idx, text.length(), Qt::red));
    stack.endMacro(); // indexChanged() is emitted
    \endcode

    This code is equivalent to:

    \code
    QUndoCommand *insertRed = new QUndoCommand(); // an empty command
    insertRed->setText("insert red text");

    new InsertText(document, idx, text, insertRed); // becomes child of insertRed
    new SetColor(document, idx, text.length(), Qt::red, insertRed);

    stack.push(insertRed);
    \endcode

    \sa endMacro()
*/

void QUndoStack::beginMacro(const QString &text)
{
    Q_D(QUndoStack);
    QUndoCommand *cmd = new QUndoCommand();
    cmd->setText(text);

    if (d->macro_stack.isEmpty()) {
        while (d->index < d->command_list.size())
            delete d->command_list.takeLast();
        if (d->clean_index > d->index)
            d->clean_index = -1; // we've deleted the clean state
        d->command_list.append(cmd);
    } else {
        d->macro_stack.last()->d->child_list.append(cmd);
    }
    d->macro_stack.append(cmd);

    if (d->macro_stack.count() == 1) {
        emit canUndoChanged(false);
        emit undoTextChanged(QString());
        emit canRedoChanged(false);
        emit redoTextChanged(QString());
    }
}

/*!
    Ends composition of a macro command.

    If this is the outermost macro in a set nested macros, this function emits
    indexChanged() once for the entire macro command.

    \sa beginMacro()
*/

void QUndoStack::endMacro()
{
    Q_D(QUndoStack);
    if (d->macro_stack.isEmpty()) {
        qWarning("QUndoStack::endMacro(): no matching beginMacro()");
        return;
    }

    d->macro_stack.removeLast();

    if (d->macro_stack.isEmpty())
        d->setIndex(d->index + 1, false);
}

/*!
    Returns the text of the command at index \a idx.

    \sa beginMacro()
*/

QString QUndoStack::text(int idx) const
{
    Q_D(const QUndoStack);

    if (idx < 0 || idx >= d->command_list.size())
        return QString();
    return d->command_list.at(idx)->text();
}

/*!
    \property QUndoStack::active
    \brief the active status of this stack.

    An application often has multiple undo stacks, one for each opened document. The active
    stack is the one associated with the currently active document. If the stack belongs
    to a QUndoGroup, calls to QUndoGroup::undo() or QUndoGroup::redo() will be forwarded
    to this stack when it is active. If the QUndoGroup is watched by a QUndoView, the view
    will display the contents of this stack when it is active. If the stack does not belong to
    a QUndoGroup, making it active has no effect.

    It is the programmer's responsibility to specify which stack is active by
    calling setActive(), usually when the associated document window receives focus.

    \sa QUndoGroup
*/

void QUndoStack::setActive(bool active)
{
#ifdef QT_NO_UNDOGROUP
    Q_UNUSED(active);
#else
    Q_D(QUndoStack);

    if (d->group != 0) {
        if (active)
            d->group->setActiveStack(this);
        else if (d->group->activeStack() == this)
            d->group->setActiveStack(0);
    }
#endif
}

bool QUndoStack::isActive() const
{
#ifdef QT_NO_UNDOGROUP
    return true;
#else
    Q_D(const QUndoStack);
    return d->group == 0 || d->group->activeStack() == this;
#endif
}

/*!
    \fn void QUndoStack::indexChanged(int idx)

    This signal is emitted whenever a command modifies the state of the document.
    This happens when a command is undone or redone. When a macro
    command is undone or redone, or setIndex() is called, this signal
    is emitted only once.

    \a idx specifies the index of the current command, ie. the command which will be
    executed on the next call to redo().

    \sa index() setIndex()
*/

/*!
    \fn void QUndoStack::cleanChanged(bool clean)

    This signal is emitted whenever the stack enters or leaves the clean state.
    If \a clean is true, the stack is in a clean state; otherwise this signal
    indicates that it has left the clean state.

    \sa isClean() setClean()
*/

/*!
    \fn void QUndoStack::undoTextChanged(const QString &undoText)

    This signal is emitted whenever the value of undoText() changes. It is
    used to update the text property of the undo action returned by createUndoAction().
    \a undoText specifies the new text.
*/

/*!
    \fn void QUndoStack::canUndoChanged(bool canUndo)

    This signal is emitted whenever the value of canUndo() changes. It is
    used to enable or disable the undo action returned by createUndoAction().
    \a canUndo specifies the new value.
*/

/*!
    \fn void QUndoStack::redoTextChanged(const QString &redoText)

    This signal is emitted whenever the value of redoText() changes. It is
    used to update the text property of the redo action returned by createRedoAction().
    \a redoText specifies the new text.
*/

/*!
    \fn void QUndoStack::canRedoChanged(bool canUndo)

    This signal is emitted whenever the value of canRedo() changes. It is
    used to enable or disable the redo action returned by createRedoAction().
    \a canUndo specifies the new value.
*/

#endif // QT_NO_UNDOSTACK
