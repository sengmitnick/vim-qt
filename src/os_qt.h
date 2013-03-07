#ifndef VIM_OS_QT_H
#define VIM_OS_QT_H

#define HAVE_PUTENV
#define HAVE_SETENV
#define HAVE_STDARG_H

#include <time.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>

#include <QtGlobal>

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
# define WORDS_BIGENDIAN 1
#endif

#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX)
#include "os_unix.h"
#endif

#undef NO_EXPANDPATH

#endif // VIM_OS_QT_H

