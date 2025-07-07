#ifndef LOGGING_H
#define LOGGING_H

#include "event.h"

enum log_level {
  LOG_LEVEL_NONE = 0,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_INFO,
  LOG_LEVEL_DEBUG,
};

void logging_init(struct event_loop *event_loop, enum log_level level);
void log_raw(enum log_level level, const char *fmt, ...);

#define log_error(fmt, ...) log_raw(LOG_LEVEL_ERROR, "\x1b[1m[error] " fmt "\x1b[0m", __VA_ARGS__)
#define log_info(...) log_raw(LOG_LEVEL_INFO, "[info] " __VA_ARGS__)
#define log_debug(...) log_raw(LOG_LEVEL_DEBUG, "[debug] " __VA_ARGS__)

#endif
