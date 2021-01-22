#ifndef _ERRNO_H_INCLUDED
#define _ERRNO_H_INCLUDED

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

#endif
