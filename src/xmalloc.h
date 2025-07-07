#ifndef XMALLOC_H
#define XMALLOC_H

#include <stddef.h>

#define xfree free

extern void free(void *ptr);

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
void *xcalloc(size_t nmemb, size_t size);

#endif

