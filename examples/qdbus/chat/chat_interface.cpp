/*
 * This file was generated by dbusxml2cpp version 0.6
 * Command line was: dbusxml2cpp -i chat_interface.h -p :chat_interface.cpp chat/com.trolltech.chat.xml
 *
 * dbusxml2cpp is Copyright (C) 2006 Trolltech ASA. All rights reserved.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#include "chat_interface.h"
/*
 * Implementation of interface class ComTrolltechChatInterface
 */

ComTrolltechChatInterface::ComTrolltechChatInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)
{
}

ComTrolltechChatInterface::~ComTrolltechChatInterface()
{
}

