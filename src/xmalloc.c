#ifndef XMALLOC_H
#define XMALLOC_H

#include <stdlib.h>
#include <assert.h>

#include "xmalloc.h"

void *xmalloc(size_t size)
{
  void *ptr = malloc(size);
  assert(ptr && "xmalloc failed");
  return ptr;
}

void *xrealloc(void *ptr, size_t size)
{
  void *new_ptr = realloc(ptr, size);
  assert(new_ptr && "xrealloc failed");
  return new_ptr;
}

void *xcalloc(size_t nmemb, size_t size)
{
  void *ptr = calloc(nmemb, size);
  assert(ptr && "xcalloc failed");
  return ptr;
}


#endif
