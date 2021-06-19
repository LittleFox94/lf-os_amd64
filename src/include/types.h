#ifndef _SYS_TYPES_H_INCLUDED
#define _SYS_TYPES_H_INCLUDED

#include <stdint.h>
#include <bits/types.h>

typedef ssize_t off_t;

typedef size_t    blksize_t;
typedef ssize_t   blkcnt_t;
typedef size_t  fsblkcnt_t;
typedef size_t  fsfilcnt_t;

typedef size_t nlink_t;

typedef uint64_t ino_t;
typedef uint32_t mode_t;

typedef uint64_t  id_t;
typedef id_t     uid_t;
typedef id_t     gid_t;
typedef id_t     pid_t;

typedef uint64_t clock_t;
typedef uint64_t time_t;
typedef ssize_t  suseconds_t;

//typedef uint64_t clockid_t;
//typedef uint64_t timer_t;

// LF OS has no "devices"
//typedef uint64_t dev_t;

//typedef uint64_t key_t;

//typedef	int		        __nl_item;
//typedef	unsigned short	__nlink_t;

//typedef struct {
//  int __count;
//  union {
//    wint_t        __wch;
//    unsigned char __wchb[4];
//  } __value;
//} _mbstate_t;
//
//#include <sys/_pthreadtypes.h>
//
//#if __GNUC_MINOR__ > 95 || __GNUC__ >= 3
//typedef	__builtin_va_list	__va_list;
//#else
//typedef	char *			__va_list;
//#endif
//
//#endif

#endif
