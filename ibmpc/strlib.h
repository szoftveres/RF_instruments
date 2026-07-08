#ifndef _STRLIB_H_
#define _STRLIB_H_

#include <stddef.h>
#include <stdarg.h>

size_t strlen (const char* s);

void* memcpy (void* d, const void* s, size_t len);

char* strcpy (char *d, const char* s);

void *memset (void *s, int c, size_t n);

int strcmp (const char* s1, const char* s2);

#endif

