#include <string.h>

#include "../midi-cc-map.h"
#include "cmd.h"

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args);

static enum cmd_arg_type arg_spec[CMD_MAX_ARGS] = {
  CMD_ARG_MIDI_DEVICE_NAME,
  CMD_ARG_NUMBER,
  CMD_ARG_PROFILE_NAME,
  CMD_ARG_PROFILE_ATTRIBUTE_NAME,
  CMD_ARG_NUMBER,
  CMD_ARG_NUMBER,

  CMD_ARG_END,
};

struct cmd map_midi_cc_cmd = {
  .name = "map-midi-cc",
  .description = "Map a MIDI control change message to a profile attribute",
  .usage = "map-midi-cc <device name> <function> <profile name> <attribute name> <min> <max>",
  .arg_spec = arg_spec,
  .run = run,
};

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args)
{
  (void) path_stack;

  struct midi_cc_map_entry entry = {
    .device_index = args[0].index,
    .function = (unsigned long) args[1].number,
    .profile_index = args[2].index,
    .attribute = args[3].attribute,
    .min = args[4].number,
    .max = args[5].number,
  };

  if (entry.function < 0 || entry.function >= 128) {
    return strdup("Invalid CC function");
  }

  // log_debug("Mapping control change function %d to attribute %d of profile %zu for MIDI device %zu",
  //           entry.function, entry.attribute, entry.profile_index, entry.device_index);

  midi_cc_map_add(context->midi_cc_map, &entry);
  return NULL;
}
