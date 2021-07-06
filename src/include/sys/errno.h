#ifndef _SYS_ERRNO_H_
#define _SYS_ERRNO_H_

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_kernel) && defined(NEWLIB_VERSION)
#include <sys/reent.h>

#ifndef _REENT_ONLY
#define errno (*__errno())
extern int *__errno (void);
#endif


extern __IMPORT const char * const _sys_errlist[];
extern __IMPORT int _sys_nerr;

#define __errno_r(ptr) ((ptr)->_errno)

/* --- end of slight redundancy (the use of struct _reent->_errno is
       hard-coded in perror.c so why pretend anything else could work too ? */

#define __set_errno(x) (errno = (x))
#endif

#define EPERM         1
#define ENOENT        2
#define ESRCH         3
#define EBADF         9
#define ECHILD       10
#define EAGAIN       11
#define ENOMEM       12
#define EACCES       13
#define EBUSY        16
#define EEXIST       17
#define ENOTDIR      20
#define EINVAL       22
#define ENFILE       23
#define EFBIG        27
#define ENOSPC       28
#define ESPIPE       29
#define EROFS        30
#define EMLINK       31
#define EDOM         33
#define ERANGE       34
#define ENAMETOOLONG 36
#define ENOSYS       38
#define ENOMSG       42
#define EOVERFLOW    75
#define EMSGSIZE     90
#define ENOTSUP      95

// user defined after this
#define __ELASTERROR 2000

#ifdef __cplusplus
}
#endif

#endif
