#include <stdio.h>
#include <stdarg.h>

#include "xmalloc.h"
#include "error.h"


char *printf_alloc(const char *fmt, ...)
{
  va_list ap_alloc, ap_print;

  va_start(ap_alloc, fmt);
  va_copy(ap_print, ap_alloc);

  size_t size = 1 + vsnprintf(NULL, 0, fmt, ap_alloc);
  va_end(ap_alloc);

  char *buf = xmalloc(size);

  (void) vsnprintf(buf, size, fmt, ap_print);
  va_end(ap_print);

  return buf;
}
