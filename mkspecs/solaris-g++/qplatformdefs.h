#ifndef QPLATFORMDEFS_H
#define QPLATFORMDEFS_H

// Get Qt defines/settings

#include "qglobal.h"

// Set any POSIX/XOPEN defines at the top of this file to turn on specific APIs
#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS
#endif

#include <unistd.h>


// We are hot - unistd.h should have turned on the specific APIs we requested


#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <dlfcn.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/filio.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>



#if defined(_XOPEN_SOURCE) && (_XOPEN_SOURCE-0 >= 500) && (_XOPEN_VERSION-0 >= 500)
// Solaris 7 and better with specific feature test macros
#define QT_SOCKLEN_T            socklen_t
#elif defined(_XOPEN_SOURCE_EXTENDED) && defined(_XOPEN_UNIX)
// Solaris 2.6 and better with specific feature test macros
#define QT_SOCKLEN_T            size_t
#else
// always this case in practice
#define QT_SOCKLEN_T            int
#endif

// Solaris redefines connect -> __xnet_connect with _XOPEN_SOURCE_EXTENDED
static inline int qt_socket_connect(int s, struct sockaddr *addr, QT_SOCKLEN_T addrlen)
{ return  ::connect(s, addr, addrlen); }
#if defined (connect)
# undef connect
#endif

// Solaris redefines bind -> __xnet_bind with _XOPEN_SOURCE_EXTENDED
static inline int qt_socket_bind(int s, struct sockaddr *addr, QT_SOCKLEN_T addrlen)
{ return ::bind(s, addr, addrlen); }
#if defined(bind)
# undef bind
#endif

// POSIX Large File Support redefines open -> open64
static inline int qt_open(const char *pathname, int flags, mode_t mode)
{ return ::open(pathname, flags, mode); }


// POSIX Large File Support redefines truncate -> truncate64
static inline int qt_truncate(const char *pathname, off_t length)
{ return ::truncate(pathname, length); }


#define QT_STATBUF		struct stat
#define QT_STATBUF4TSTAT	struct stat
#define QT_STAT			::stat
#define QT_FSTAT		::fstat
#define QT_STAT_REG		S_IFREG
#define QT_STAT_DIR		S_IFDIR
#define QT_STAT_MASK		S_IFMT
#define QT_STAT_LNK		S_IFLNK
#define QT_SOCKET_CONNECT	qt_socket_connect
#define QT_FILENO		fileno
#define QT_OPEN			::open
#define QT_CLOSE		::close
#define QT_TRUNCATE		::truncate
#define QT_LSEEK		::lseek
#define QT_READ			::read
#define QT_WRITE		::write
#define QT_ACCESS		::access
#define QT_GETCWD		::getcwd
#define QT_CHDIR		::chdir
#define QT_MKDIR		::mkdir
#define QT_RMDIR		::rmdir
#define QT_OPEN_RDONLY		O_RDONLY
#define QT_OPEN_WRONLY		O_WRONLY
#define QT_OPEN_RDWR		O_RDWR
#define QT_OPEN_CREAT		O_CREAT
#define QT_OPEN_TRUNC		O_TRUNC
#define QT_OPEN_APPEND		O_APPEND

#define QT_SIGNAL_RETTYPE	void
#define QT_SIGNAL_ARGS		int
#define QT_SIGNAL_IGNORE	SIG_IGN

#if !defined(_XOPEN_UNIX)
// Solaris 2.5.1
// Function usleep() is defined in C library but not declared in header files
// on Solaris 2.5.1. Not really a surprise, usleep() is specified by XPG4v2
// and XPG4v2 is only supported by Solaris 2.6 and better.
// Function gethostname() is also defined in C library but not declared in
// header files on Solaris 2.5.1.
typedef unsigned int useconds_t;
extern "C" int usleep(useconds_t);
extern "C" int gethostname(char *, int);
#endif

#if defined(_XOPEN_UNIX)
// Solaris 2.6 and better
#define QT_SNPRINTF		::snprintf
#define QT_VSNPRINTF		::vsnprintf
#endif

// POSIX Large File Support redefines open -> open64
#if defined(open)
# undef open
#endif

// POSIX Large File Support redefines truncate -> truncate64
#if defined(truncate)
# undef truncate
#endif


#endif // QPLATFORMDEFS_H
