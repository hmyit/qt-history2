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

/*
 * Copyright (c) 1997 Theo de Raadt
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "qbytearray.h"
#include "qplatformdefs.h"

#ifndef QT_VSNPRINTF

#include <sys/param.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <setjmp.h>

#ifndef roundup
#define roundup(x, y) ((((x)+((y)-1))/(y))*(y))
#endif

static int pgsize;
static char *curobj;
static sigjmp_buf bail;

#define EXTRABYTES	2	/* XXX: why 2? you don't want to know */

static char *
msetup(char * /*str*/, size_t n)
{
	char *e;

	if (n == 0)
		return NULL;
	if (pgsize == 0)
		pgsize = getpagesize();
	curobj = (char *)malloc(n + EXTRABYTES + pgsize * 2);
	if (curobj == NULL)
		return NULL;
	e = curobj + n + EXTRABYTES;
	e = (char *)roundup((unsigned long)e, pgsize);
	if (mprotect(e, pgsize, PROT_NONE) == -1) {
		free(curobj);
		curobj = NULL;
		return NULL;
	}
	e = e - n - EXTRABYTES;
	*e = '\0';
	return (e);
}

static void
mcatch(int)
{
	siglongjmp(bail, 1);
}

static void
mcleanup(char *str, size_t n, char *p)
{
	int l = strlen(p);

	if (l > int(n - 1))
	    l = n - 1;
	memcpy(str, p, l);
	str[l] = '\0';
	if (mprotect((caddr_t)(p + n + EXTRABYTES), pgsize,
	    PROT_READ|PROT_WRITE|PROT_EXEC) == -1)
		mprotect((caddr_t)(p + n + EXTRABYTES), pgsize,
		    PROT_READ|PROT_WRITE);
	free(curobj);
}

int qvsnprintf(char *str, size_t n, char const *fmt, va_list ap)
{
	struct sigaction osa, nsa;
	char *p;
	int ret = n + 1;	/* if we bail, indicated we overflowed */

	memset(&nsa, 0, sizeof nsa);
	nsa.sa_handler = mcatch;
	sigemptyset(&nsa.sa_mask);

	p = msetup(str, n);
	if (p == NULL) {
		*str = '\0';
		return 0;
	}
	if (sigsetjmp(bail, 1) == 0) {
		if (sigaction(SIGSEGV, &nsa, &osa) == -1) {
			mcleanup(str, n, p);
			return (0);
		}
		ret = vsprintf(p, fmt, ap);
	}
	mcleanup(str, n, p);
	(void) sigaction(SIGSEGV, &osa, NULL);
	return (ret);
}

#else

#include <stdio.h>

int qvsnprintf(char *str, size_t n, const char *fmt, va_list ap)
{
    return QT_VSNPRINTF(str, n, fmt, ap);
}

#endif

// qsnprintf and qvsnprintf are declared in qbytearray.h


/*! \fn int qvsnprintf(char *str, size_t n, char const *fmt, va_list ap)

  \relates QByteArray

  A portable vsnprintf() function. Will either call ::vsnprintf()
  on systems that implement this function or fall back to an
  internal version.

  \sa qvsnprintf
*/

/*! \fn int qsnprintf(char *str, size_t n, char const *fmt, ...);

  \relates QByteArray

  A portable snprintf() function, calls qvsnprintf.

  \sa qvsnprintf
*/

int qsnprintf(char *str, size_t n, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    int ret = qvsnprintf(str, n, fmt, ap);
    va_end(ap);

    return ret;
}
