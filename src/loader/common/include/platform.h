#pragma once

/**
 * \file platform.h
 * \brief specifies the interface a `platform` has to implement.
 *
 * Platform specific code are allowed to depend on all the functions defined in `bootenv.h`,
 * which defines the interface a boot environment implementation of the loader have to implement.
 */

#include <stdbool.h>
#include <stdlib.h>

/**
 * malloc dynamically allocates the given amount of memory.
 *
 * \param size is the amount of bytes to be allocated.
 * \returns pointer to the first byte of the newly allocated memory on success, 0 on error.
 */
void* malloc(size_t size);

/**
 * realloc resizes allocates memory, copying data in an existing allocation to the new memory location
 * if needed.
 *
 * `realloc(NULL, n)` behaves exactly like `malloc(n)`.
 *
 * If there is not enough memory at the current location of the allocation, new memory is allocated, the
 * existing data is copied over and the old allocation is freed.
 *
 * \param ptr is the pointer to an existing memory allocation.
 * \param size is the new amount of bytes for the allocation.
 * \returns pointer to the first byte of the allocated memory on success, 0 on error.
 */
void* realloc(void* ptr, size_t size);

/**
 * free marks the passed memory allocation as free, meaning it can be reused for later allocations.
 *
 * \remarks free()-ing a pointer not returned by malloc() or realloc() will result in undefined behavior. Do
 *          not do that.
 *
 * \param ptr points to the first byte of the memory allocation to be freed.
 */
void free(void* ptr);

/**
 * memcpy copies a memory region into another, ideally in a heavily optimized manner.
 *
 * \remarks Copying overlapping memory regions results in undefined behavior.
 *
 * \param dest points to the first byte of where to copy the data.
 * \param src points to the first byte of the data to copy.
 * \param n specifies the amount of bytes to copy.
 * \returns pointer to the destination, useful to use memcpy calls in function arguments - but please don't do
 *          that for better code readability.
 */
void* memcpy(void* dest, void const* src, size_t n);

/**
 * printf allows the boot environment implementation to log things. It can be a trimmed down version, but
 * support for the following features is required to be implemented:
 *
 * - Conversion specifiers
 *   * `d` for signed decimal numbers
 *   * `u` for unsigned decimal numbers
 *   * `x` for unsigned hexadecimal numbers
 *   * `p` for pointers, like `%#x` or `%#lx`
 *   * `s` for strings
 *   * `c` for single characters
 *   * `%` for printing an actual % character
 *
 * - Flag characters
 *   * `#` for prefixing hexadecimal numbers with `0x`
 *   * `0` and a space character for zero and space padding respectively
 *
 * - Length modifiers
 *   * `hh` specifying the given value is a `signed char` or `unsigned char`
 *   * `h` specifying the given value is a `signed short` or `unsigned short`
 *   * `l` (lowercase `L`) specifying the given value is a `signed long` or `unsigned long`
 *   * `ll` (two lowercase `L`s) specifying the given value is a `signed long long` or `unsigned long long`
 *
 * - Field width with fixed value in the format string (so no `*` for specifying in next argument or
 *   `m$` for specifying in argument `m`)
 *
 * \param format is the string to be logged, including format specifiers. Please refer to a C documentation for
 *        more information. The minimum required subset of features is specified in the function description above.
 * \returns the number of characters logged
 */
int printf(const char* format, ...);
