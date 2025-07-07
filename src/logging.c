#include <stdio.h>
#include <stdarg.h>

#include "xmalloc.h"
#include "logging.h"
#include "error.h"

static struct event_loop *s_event_loop = NULL;
static enum log_level s_level = 0;

void logging_init(struct event_loop *event_loop, enum log_level level)
{
  s_event_loop = event_loop;
  s_level = level;
}

void log_raw(enum log_level level, const char *fmt, ...)
{
  if (level > s_level) {
    return;
  }

  va_list ap_alloc, ap_print;

  va_start(ap_alloc, fmt);
  va_copy(ap_print, ap_alloc);

  size_t size = 1 + vsnprintf(NULL, 0, fmt, ap_alloc);
  va_end(ap_alloc);

  char *message = xmalloc(size);

  (void) vsnprintf(message, size, fmt, ap_print);
  va_end(ap_print);

  struct event event = {
    .type = EVENT_MESSAGE,
    .message = message,
  };

  event_loop_enqueue_event(s_event_loop, &event);
}

