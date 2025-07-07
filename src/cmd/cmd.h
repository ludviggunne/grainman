#ifndef CMD_H
#define CMD_H

#include "path-stack.h"
#include "../context.h"
#include "../profile.h"

#define CMD_MAX_ARGS 8

enum cmd_arg_type {
  CMD_ARG_END,
  CMD_ARG_NAME,
  CMD_ARG_PATTERN,
  CMD_ARG_SAMPLE_NAME,
  CMD_ARG_MIDI_DEVICE_NAME,
  CMD_ARG_PROFILE_NAME,
  CMD_ARG_PROFILE_ATTRIBUTE_NAME,
  CMD_ARG_FILE_NAME,
  CMD_ARG_NUMBER,
};

struct cmd_arg {
  char                  *string;
  float                  number;
  size_t                 index;
  enum profile_attribute attribute;
};

struct cmd {
  const char *name;
  const char *description;
  const char *usage;

  /* A list of argument types terminated with CMD_ARG_END */
  const enum cmd_arg_type *arg_spec;

  char *(*run)(struct context *context, struct path_stack **path_stack, const struct cmd_arg *);
};

/* Returns an error string on failure, otherwise NULL.
 * The returned error string should be freed by caller. */
char *cmd_interpret(struct context *context, struct path_stack **path_stack, const char *cmd_str);

#endif
