#include <string.h>

#include "../midi.h"
#include "../context.h"
#include "../error.h"

#include "cmd.h"

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args);

static enum cmd_arg_type arg_spec[CMD_MAX_ARGS] = {
  CMD_ARG_NAME,
  CMD_ARG_PATTERN,
  CMD_ARG_END,
};

struct cmd add_midi_device_cmd = {
  .name = "add-midi-device",
  .description = "Add a MIDI device connection",
  .usage = "add-midi-device <name> <pattern>",
  .arg_spec = arg_spec,
  .run = run,
};

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args)
{
  (void) path_stack;

  const char *name = args[0].string;
  const char *pattern = args[1].string;

  if (midi_device_list_find(&context->midi_devices, name) >= 0) {
    return printf_alloc("MIDI device with name %s already exists", name);
  }

  // log_debug("Added MIDI device %s", name);

  return midi_device_add(context, name, pattern);
}
