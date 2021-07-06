#ifndef _STDARG_H
#define _STDARG_H

typedef __builtin_va_list       va_list;
typedef __builtin_va_list       __gnuc_va_list;
#define va_start(ap, X)         __builtin_va_start(ap, X)
#define va_arg(ap, type)        __builtin_va_arg(ap, type)
#define va_end(ap)              __builtin_va_end(ap)

#define _VA_LIST_DEFINED

#endif
