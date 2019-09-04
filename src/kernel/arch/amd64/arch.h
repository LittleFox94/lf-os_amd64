#ifndef _ARCH_H_INCLUDED
#define _ARCH_H_INCLUDED

typedef signed   char int8_t;
typedef unsigned char uint8_t;

typedef signed   short int16_t;
typedef unsigned short uint16_t;

typedef signed   int int32_t;
typedef unsigned int uint32_t;

typedef signed   long int64_t;
typedef unsigned long uint64_t;


typedef uint64_t ptr_t;
typedef uint64_t size_t;

#define HIGHER_HALF_START 0xFFFF800000000000

void nyi(int loop);

#endif
