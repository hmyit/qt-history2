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

#include "qprocess.h"
#include "qprocess_p.h"

#include <qdatetime.h>
#include <qfileinfo.h>
#include <qtimer.h>

#define SLEEPMIN 10
#define SLEEPMAX 500
#define NOTIFYTIMEOUT 100

 
void QProcessPrivate::createPipe(Q_PIPE pipe[2])
{
    // Open the pipes.  Make non-inheritable copies of input write and output
    // read handles to avoid non-closable handles (this is done by the
    // DuplicateHandle() call).

      SECURITY_ATTRIBUTES secAtt = { sizeof( SECURITY_ATTRIBUTES ), NULL, TRUE };
    
    HANDLE tmpStdin, tmpStdout, tmpStderr;
        if ( !CreatePipe( &writePipe[0], &tmpStdin, &secAtt, 0 ) ) {
           
        }
        if ( !DuplicateHandle( GetCurrentProcess(), tmpStdin, GetCurrentProcess(), &writePipe[1], 0, FALSE, DUPLICATE_SAME_ACCESS ) ) {
        }
        if ( !CloseHandle( tmpStdin ) ) {
        }
    
        if ( !CreatePipe( &tmpStdout, &standardReadPipe[1], &secAtt, 0 ) ) {
        }
        if ( !DuplicateHandle( GetCurrentProcess(), tmpStdout, GetCurrentProcess(), &standardReadPipe[0], 0, FALSE, DUPLICATE_SAME_ACCESS ) ) {
        }
        if ( !CloseHandle( tmpStdout ) ) {
        }
        if ( !CreatePipe( &tmpStderr, &errorReadPipe[1], &secAtt, 0 ) ) {
        }
        if ( !DuplicateHandle( GetCurrentProcess(), tmpStderr, GetCurrentProcess(), &errorReadPipe[0], 0, FALSE, DUPLICATE_SAME_ACCESS ) ) {
        }
        if ( !CloseHandle( tmpStderr ) ) {
        }
}

void QProcessPrivate::destroyPipe(Q_PIPE pipe[2])
{
    if (pipe[0] != INVALID_Q_PIPE) {
        CloseHandle(pipe[0]);
        pipe[0] = INVALID_Q_PIPE; 
    }
    if (pipe[1] != INVALID_Q_PIPE) {
        CloseHandle(pipe[1]);
        pipe[1] = INVALID_Q_PIPE;
    }

}

void QProcessPrivate::startProcess()
{
     Q_Q(QProcess);

 
    createPipe(standardReadPipe);
  //  createPipe(errorReadPipe);
  //  createPipe(writePipe);

    // construct the arguments for CreateProcess()
    
    QString args;
    // add the prgram as the first arrg ... it works better
    args = program + " ";
    for (int i=0; i<arguments.size(); ++i) {
        ///### handle .bat files
   	    QString tmp = arguments.at(i);
	    // escape a single " because the arguments will be parsed
	    tmp.replace( "\"", "\\\"" );
	    if (tmp.isEmpty() || tmp.contains(' ') || tmp.contains('\t')) {
	        // The argument must not end with a \ since this would be interpreted
	        // as escaping the quote -- rather put the \ behind the quote: e.g.
	        // rather use "foo"\ than "foo\"
	        QString endQuote("\"");
	        int i = tmp.length();
	        while (i>=0 && tmp.at(i-1) == '\\') {
                --i;
		        endQuote += "\\";
	        }
            args += QString(" \"") + tmp.left(i) + endQuote;
	    } else {
	        args += ' ' + tmp;
	    }
    }

    bool success;
    
    if (pid) {
        delete pid;
        pid = 0;
    }
    pid = new PROCESS_INFORMATION;
    memset(pid, 0, sizeof(PROCESS_INFORMATION));
    
    processState = QProcess::Starting;
    emit q->stateChanged(processState);

#ifdef UNICODE
    if (!(QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based)) {
	    STARTUPINFOW startupInfo = {
	        sizeof( STARTUPINFO ), 0, 0, 0,
	        (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
	        0, 0, 0,
	        STARTF_USESTDHANDLES,
	        0, 0, 0,
	        writePipe[0], standardReadPipe[1], errorReadPipe[1]
	    };
        QByteArray envlist;
	    if (environment.isEmpty()) {
	        int pos = 0;
	        // add PATH if necessary (for DLL loading)
	        char *path = qgetenv("PATH");
	        if (environment.grep(QRegExp("^PATH=",FALSE)).empty() && path) {
		        QString tmp = QString("PATH=%1").arg(qgetenv("PATH"));
		        uint tmpSize = sizeof(TCHAR) * (tmp.length()+1);
		        envlist.resize(envlist.size() + tmpSize );
		        memcpy(envlist.data()+pos, tmp.ucs2(), tmpSize);
		        pos += tmpSize;
	        }
	        // add the user environment
	        for (QStringList::Iterator it = environment.begin(); it != environment.end(); it++ ) {
		        QString tmp = *it;
		        uint tmpSize = sizeof(TCHAR) * (tmp.length()+1);
		        envlist.resize(envlist.size() + tmpSize);
		        memcpy(envlist.data()+pos, tmp.ucs2(), tmpSize);
		        pos += tmpSize;
	        }
	        // add the 2 terminating 0 (actually 4, just to be on the safe side)
	        envlist.resize( envlist.size()+4 );
	        envlist[pos++] = 0;
	        envlist[pos++] = 0;
	        envlist[pos++] = 0;
	        envlist[pos++] = 0;
	    }

        qDebug("Creating process");
        qDebug("   program : [%s]", program.latin1());
        qDebug("   args : %s", args.latin1());
        qDebug("   pass enviroment : %s", environment.isEmpty() ? "no" : "yes");
        
        success = CreateProcessW(0, (WCHAR*)args.utf16(),
		                         0, 0, TRUE, CREATE_UNICODE_ENVIRONMENT | CREATE_NO_WINDOW, 
                                 environment.isEmpty() ? 0 : envlist.data(),
                                 workingDirectory.isEmpty() ? 0 : (WCHAR*)workingDirectory.ucs2(),
		                         &startupInfo, (PROCESS_INFORMATION*)pid);
    } else
#endif // UNICODE
    {
#ifndef Q_OS_TEMP
	    STARTUPINFOA startupInfo = { sizeof( STARTUPINFOA ), 0, 0, 0,
	        (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
	        0, 0, 0,
	        STARTF_USESTDHANDLES,
	        0, 0, 0,
	        writePipe[0], standardReadPipe[1], errorReadPipe[1]
	    };
        QByteArray envlist;
	    if (environment.isEmpty()) {
            int pos = 0;
	        // add PATH if necessary (for DLL loading)
	        char *path = qgetenv("PATH");
	        if (environment.grep( QRegExp("^PATH=",FALSE) ).empty() && path ) {
		        QByteArray tmp = QString("PATH=%1").arg(qgetenv("PATH")).local8Bit();
		        uint tmpSize = tmp.length() + 1;
		        envlist.resize(envlist.size() + tmpSize);
		        memcpy(envlist.data()+pos, tmp.data(), tmpSize);
		        pos += tmpSize;
	        }
	        // add the user environment
	        for (QStringList::Iterator it = environment.begin(); it != environment.end(); it++) {
		        QByteArray tmp = (*it).local8Bit();
		        uint tmpSize = tmp.length() + 1;
		        envlist.resize(envlist.size() + tmpSize);
		        memcpy(envlist.data()+pos, tmp.data(), tmpSize);
		        pos += tmpSize;
	        }
	        // add the terminating 0 (actually 2, just to be on the safe side)
	        envlist.resize(envlist.size()+2);
	        envlist[pos++] = 0;
	        envlist[pos++] = 0;
	    }
        qDebug("Creating process");
        qDebug("   program : %s", program.latin1());
        qDebug("   args : %s", args.latin1());

	    success = CreateProcessA(program.toLocal8Bit().data(), args.toLocal8Bit().data(),
		0, 0, TRUE, 0, environment.isEmpty() ? 0 : envlist.data(),
		workingDirectory.local8Bit(), &startupInfo, (PROCESS_INFORMATION*)pid);
#endif // Q_OS_TEMP
    }
    
#ifndef Q_OS_TEMP
    CloseHandle(writePipe[0]);
    standardReadPipe[1] = INVALID_Q_PIPE;
    CloseHandle(standardReadPipe[1]);
    standardReadPipe[1] = INVALID_Q_PIPE;
    CloseHandle(errorReadPipe[1]);
    errorReadPipe[1] = INVALID_Q_PIPE;
#endif
    
    qDebug("call startupNotification()");

    processState = QProcess::Running;
    startupNotification();

    // now that we have started, start the timer;
    if (notifier) {
        delete notifier;
        notifier = 0;
    }
    notifier = new QTimer(q);
    QObject::connect(notifier, SIGNAL(timeout()), q, SLOT(notified()));
    notifier->start(NOTIFYTIMEOUT);
    
    qDebug("started");
}

void QProcessPrivate::execChild()
{
}

bool QProcessPrivate::processStarted()
{
    return processState == QProcess::Running;
}

Q_LONGLONG QProcessPrivate::bytesAvailableFromStdout() const
{
    DWORD bytesAvail = 0;
    DWORD r;
    char dummy;    
    PeekNamedPipe(standardReadPipe[0], &dummy, 1, &r, &bytesAvail, 0);
    qDebug("bytesAvailableFromStdout() = %d", bytesAvail); 
    return bytesAvail;
}

Q_LONGLONG QProcessPrivate::bytesAvailableFromStderr() const
{
    DWORD bytesAvail = 0; 
    PeekNamedPipe(errorReadPipe[0], 0, 0, 0, &bytesAvail, 0);
    qDebug("bytesAvailableFromStderr() = %d", bytesAvail);
    return bytesAvail;
}

Q_LONGLONG QProcessPrivate::readFromStdout(char *data, Q_LONGLONG maxlen)
{
    DWORD read = qMin(maxlen, bytesAvailableFromStdout());
    DWORD bytesRead = 0;

    if (read > 0 && !ReadFile(standardReadPipe[0], data, read, &bytesRead, 0))
        bytesRead = -1;
    qDebug("readFromStdout() = %d", bytesRead);
    return bytesRead;
}

Q_LONGLONG QProcessPrivate::readFromStderr(char *data, Q_LONGLONG maxlen)
{
    DWORD read = qMin(maxlen, bytesAvailableFromStderr());
    DWORD bytesRead = 0;

    if (read > 0 && !ReadFile(errorReadPipe[0], data, read, &bytesRead, 0))
        bytesRead = -1;

    qDebug("readFromStdout() = %d", bytesRead);
    return bytesRead;
}

void QProcessPrivate::killProcess()
{
    if (pid)
	    TerminateProcess(((PROCESS_INFORMATION*)pid)->hProcess, 0xf291);
}

bool QProcessPrivate::waitForStarted(int)
{
    Q_Q(QProcess);

    if (processStarted())
        return true;

    processError = QProcess::Timedout;
    q->setErrorString(QT_TRANSLATE_NOOP(QProcess, "Process opeation timed out"));
    return false; 
}

bool QProcessPrivate::waitForReadyRead(int msecs)
{
    Q_Q(QProcess);

    HANDLE pipe = (processChannel == QProcess::StandardOutput)
             ? standardReadPipe[0] : errorReadPipe[0];


    QTime start;
    start.start();

    int nextSleep = SLEEPMIN; 
    for (;;) {
        DWORD bytesAvail = 0; 
        if (PeekNamedPipe(pipe, 0, 0, 0, &bytesAvail, 0) == 0 || bytesAvail != 0)
            return true;
        nextSleep = qMin(qMin(nextSleep, SLEEPMAX), msecs - start.elapsed());
        if (nextSleep >= 0)
            break;
        Sleep(nextSleep);
        nextSleep *= 2;
    }

    processError = QProcess::Timedout;
    q->setErrorString(QT_TRANSLATE_NOOP(QProcess, "Process opeation timed out"));
    return false;
}


bool isProcessStopped(QProcessPrivate * that, int msecs)
{
    qDebug("isProcessStopped");

    if (!that->pid)
        return true;

    if (WaitForSingleObject(((PROCESS_INFORMATION*)that->pid)->hProcess, msecs) == WAIT_TIMEOUT) {
        return false;
    }

    DWORD theExitCode;
    if (GetExitCodeProcess(((PROCESS_INFORMATION*)that->pid)->hProcess, &theExitCode)) {
        if (theExitCode != STILL_ACTIVE) { // this should ever be true?
            that->exitCode = theExitCode;
            that->crashed = that->exitCode != 0xf291;
        }
    }
    return true;
}


bool QProcessPrivate::waitForFinished(int msecs)
{
    Q_Q(QProcess);

    qDebug("QProcessPrivate::waitForFinished(%d)", msecs);
    if (!isProcessStopped(this, msecs)) {
        qDebug("timeout");
        processError = QProcess::Timedout;
        q->setErrorString(QT_TRANSLATE_NOOP(QProcess, "Process opeation timed out"));
        return false;
    }
    qDebug("finished");
    processDied();
    return true;
}

Q_LONGLONG QProcessPrivate::writeToStdin(const char *data, Q_LONGLONG maxlen)
{
    Q_LONGLONG totalWritten = 0;
    
    while (totalWritten < maxlen) {
        DWORD written = 0;
        DWORD write = qMin(8192, maxlen - totalWritten);
        data += totalWritten;
        if (!WriteFile(writePipe[1], data, write, &written, 0)) {
            totalWritten = -1;
            break;
        }
		totalWritten += written;
    }
    qDebug("writeToStdin() = %d", totalWritten); 
    return totalWritten;
}

bool QProcessPrivate::waitForWrite(int)
{
    Q_Q(QProcess);

    if (processStarted())
        return true;

    processError = QProcess::Timedout;
    q->setErrorString(QT_TRANSLATE_NOOP(QProcess, "Process opeation timed out"));
    return false;
}

void QProcessPrivate::notified()
{
    notifier->stop();

    if (pid && isProcessStopped(this, 0)) {
        qDebug("processs is stopped");
        processDied();
        return;
    }
    
    if (!writeBuffer.isEmpty())
        readyWrite();

    if (bytesAvailableFromStdout())
        readyReadStandardOutput();
    
    if (bytesAvailableFromStderr())
        readyReadStandardError();
    
    notifier->start(NOTIFYTIMEOUT);
}
