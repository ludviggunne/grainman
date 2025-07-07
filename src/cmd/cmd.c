#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../midi.h"
#include "../profile.h"
#include "../sample.h"
#include "../xmalloc.h"
#include "../error.h"
#include "cmd.h"

extern struct cmd import_sample_cmd;
extern struct cmd add_midi_device_cmd;
extern struct cmd map_midi_cc_cmd;
extern struct cmd set_attribute_cmd;
extern struct cmd add_profile_cmd;
extern struct cmd source_cmd;
extern struct cmd context_dump_cmd;
extern struct cmd list_midi_devices_cmd;
extern struct cmd help_cmd;
extern struct cmd start_cmd;
extern struct cmd stop_cmd;

const struct cmd *const cmd_list[] = {
  &import_sample_cmd,
  &add_midi_device_cmd,
  &map_midi_cc_cmd,
  &set_attribute_cmd,
  &add_profile_cmd,
  &source_cmd,
  &context_dump_cmd,
  &list_midi_devices_cmd,
  &help_cmd,
  &start_cmd,
  &start_cmd,
  &stop_cmd,

  NULL,
};

static int is_comment(const char *str)
{
  for (; strchr(" \n", *str) != NULL; ++str)
    ;
  return *str == '#';
}

static void free_arg_list(struct cmd_arg *args, size_t size)
{
  for (size_t i = 0; i < size; ++i) {
    xfree(args[i].string);
  }
}

char *cmd_interpret(struct context *context, struct path_stack **path_stack, const char *cmd_str)
{
  char *error = NULL;
  const char *delim = " \t\n";
  char *saveptr = NULL;
  char *copy = strdup(cmd_str);

  char *semi = strchr(copy, ';');
  if (semi) {
    char *left = strndup(copy, semi - copy);
    char *right = strdup(semi + 1);
    xfree(copy);

    error = cmd_interpret(context, path_stack, left);
    xfree(left);

    if (error != NULL) {
      xfree(right);
      return error;
    }

    error = cmd_interpret(context, path_stack, right);
    xfree(right);

    return error;
  }

  char *cmd_name = strtok_r(copy, delim, &saveptr);

  if (cmd_name == NULL || is_comment(cmd_name)) {
    return NULL;
  }

  const struct cmd *const *cmdptr = cmd_list;
  for (; *cmdptr; ++cmdptr) {
    if (strcmp(cmd_name, (*cmdptr)->name) == 0) {
      break;
    }
  }

  const struct cmd *cmd = *cmdptr;
  if (cmd == NULL) {
    return printf_alloc("Undefined command: '%s'", cmd_name);
  }

  const enum cmd_arg_type *arg_spec = cmd->arg_spec;
  struct cmd_arg args[CMD_MAX_ARGS] = {0};
  size_t argi;

  for (argi = 0; *arg_spec != CMD_ARG_END; ++arg_spec, ++argi) {
    char *argstr = strtok_r(NULL, delim, &saveptr);

    if (argstr == NULL) {
      error = printf_alloc("%s: Missing argument. Usage: %s", cmd->name, cmd->usage);
      goto finish;
    }

    enum cmd_arg_type arg_type = *arg_spec;
    int index;
    char *endptr;

    switch (arg_type) {
    case CMD_ARG_NAME:
    case CMD_ARG_PATTERN:
    case CMD_ARG_FILE_NAME:
      args[argi].string = strdup(argstr);
      break;

    case CMD_ARG_SAMPLE_NAME:
      index = sample_list_find(&context->samples, argstr);
      if (index == -1) {
        error = printf_alloc("%s: No loaded sample with name %s", cmd->name, argstr);
        goto finish;
      }
      args[argi].index = index;
      break;

    case CMD_ARG_MIDI_DEVICE_NAME:
      index = midi_device_list_find(&context->midi_devices, argstr);
      if (index == -1) {
        error = printf_alloc("%s: No connected MIDI device with name %s", cmd->name, argstr);
        goto finish;
      }
      args[argi].index = index;
      break;

    case CMD_ARG_PROFILE_NAME:
      index = profile_list_find(&context->profiles, argstr);
      if (index == -1) {
        error = printf_alloc("%s: No profile with name %s", cmd->name, argstr);
        goto finish;
      }
      args[argi].index = index;
      break;

    case CMD_ARG_PROFILE_ATTRIBUTE_NAME:
      args[argi].attribute = profile_attribute_from_name(argstr);
      if (args[argi].attribute == PROFILE_ATTRIBUTE_NONE) {
        error = printf_alloc("%s: No profile attribute named '%s'", cmd->name, argstr);
        goto finish;
      }
      break;

    case CMD_ARG_NUMBER:
      args[argi].number = strtod(argstr, &endptr);
      if (endptr == argstr) {
        error = printf_alloc("%s: Invalid number: %s", cmd->name, argstr);
        goto finish;
      }
      break;

    case CMD_ARG_END:
      break;
    }
  }

  if (*arg_spec != CMD_ARG_END) {
    error = printf_alloc("%s: Trailing arguments", cmd->name);
    goto finish;
  }

  error = cmd->run(context, path_stack, args);

finish:
  free_arg_list(args, argi);
  return error;
}
