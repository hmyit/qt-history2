/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "abstractactioneditor.h"

/*!
    \class QDesignerActionEditorInterface

    \brief The QDesignerActionEditorInterface class allows you to
    change the focus of Qt Designer's action editor.

    \inmodule QtDesigner

    The QDesignerActionEditorInterface class is not intended to be
    instantiated directly. You can retrieve an interface to \QD's
    action editor using the
    QDesignerFormEditorInterface::actionEditor() function.

    You can control which actions that are available in the action
    editor's window using the manageAction() and unmanageAction()
    functions. An action that is managed by \QD is available in the
    action editor while an unmanaged action is ignored.

    QDesignerActionEditorInterface also provides the core() function
    that you can use to retrieve a pointer to \QD's current
    QDesignerFormEditorInterface object, and the setFormWindow()
    function that enables you to change the currently selected form
    window.

    \sa QDesignerFormEditorInterface, QDesignerFormWindowInterface
*/

/*!
    Constructs an action editor interface with the given \a parent and
    the specified window \a flags.
*/
QDesignerActionEditorInterface::QDesignerActionEditorInterface(QWidget *parent, Qt::WindowFlags flags)
    : QWidget(parent, flags)
{
}

/*!
    Destroys the action editor interface.
*/
QDesignerActionEditorInterface::~QDesignerActionEditorInterface()
{
}

/*!
    Returns a pointer to \QD's current QDesignerFormEditorInterface
    object.
*/
QDesignerFormEditorInterface *QDesignerActionEditorInterface::core() const
{
    return 0;
}

/*!
    \fn void QDesignerActionEditorInterface::setFormWindow(QDesignerFormWindowInterface *formWindow)

    Sets the currently selected form window to \a formWindow.

*/

/*!
    \fn void QDesignerActionEditorInterface::manageAction(QAction *action)

    Instructs \QD to manage the specified \a action. An action that is
    managed by \QD is available in the action editor.

    \sa unmanageAction()
*/

/*!
    \fn void QDesignerActionEditorInterface::unmanageAction(QAction *action)

    Instructs \QD to ignore the specified \a action. An unmanaged
    action is not available in the action editor.

    \sa manageAction()

*/
