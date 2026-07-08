#include "strlib.h"

size_t strlen (const char* s) {
    size_t  ret;
    for (ret = 0; *s; ++s, ++ret);
    return ret;
}

void* memcpy (void* d, const void* s, size_t len) {
    for (;len; --len, ++d, ++s) {
        *(char*)d = *(char*)s;
    }
    return d;
}

char* strcpy (char *d, const char* s) {
    while ((*d++ = *s++));
    return d;
}


void *memset (void *s, int c, size_t n) {
    char* it = (char*)s;
    while (n--) {
        *it = (char)c;
        it++;
    }
    return s;
}


int strcmp (const char* s1, const char* s2) {
    while(*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}


