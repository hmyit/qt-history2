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

#ifndef QPROCESS_P_H
#define QPROCESS_P_H

#include "qprocess.h"

#include <private/qinternal_p.h>
#include <private/qiodevice_p.h>
#include <qstringlist.h>

#ifdef Q_OS_WIN
typedef HANDLE Q_PIPE;
#define INVALID_Q_PIPE INVALID_HANDLE_VALUE
#else
typedef int Q_PIPE;
#define INVALID_Q_PIPE -1
#endif


class QSocketNotifier;
class QTimer;

class QProcessPrivate : public QIODevicePrivate
{
public:
    Q_DECLARE_PUBLIC(QProcess)

    QProcessPrivate();
    virtual ~QProcessPrivate();

    // private slots
    void readyReadStandardOutput();
    void readyReadStandardError();
    void readyWrite();
    void startupNotification();
    void processDied();
    void notified();

    QProcess::ProcessChannel processChannel;
    QProcess::ProcessError processError;
    QProcess::ProcessState processState;
    QString workingDirectory;
    Q_PID pid;

    QString program;
    QStringList arguments;
    QStringList environment;

    QRingBuffer outputReadBuffer;
    QRingBuffer errorReadBuffer;
    QRingBuffer writeBuffer;

    Q_PIPE standardReadPipe[2];
    Q_PIPE errorReadPipe[2];
    Q_PIPE writePipe[2];
    Q_PIPE childStartedPipe[2];
    void createPipe(Q_PIPE pipe[2]);
    void destroyPipe(Q_PIPE pipe[2]);

    QSocketNotifier *standardReadSocketNotifier;
    QSocketNotifier *errorReadSocketNotifier;
    QSocketNotifier *writeSocketNotifier;
    QSocketNotifier *startupSocketNotifier;

    // the wonderfull windows notifier
    QTimer * notifier;

    void startProcess();
    void execChild();
    bool processStarted();
    void killProcess();

    int exitCode;
    bool crashed;

    bool waitForStarted(int msecs = 30000);
    bool waitForReadyRead(int msecs = 30000);
    bool waitForFinished(int msecs = 30000);
    bool waitForWrite(int msecs = 30000);

    Q_LONGLONG bytesAvailableFromStdout() const;
    Q_LONGLONG bytesAvailableFromStderr() const;
    Q_LONGLONG readFromStdout(char *data, Q_LONGLONG maxlen);
    Q_LONGLONG readFromStderr(char *data, Q_LONGLONG maxlen);
    Q_LONGLONG writeToStdin(const char *data, Q_LONGLONG maxlen);

    void cleanup();
};

#endif
