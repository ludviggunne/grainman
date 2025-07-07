#include <jack/jack.h>

#include "cmd.h"

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args);

static enum cmd_arg_type arg_spec[CMD_MAX_ARGS] = { CMD_ARG_END, };

struct cmd list_midi_devices_cmd = {
  .name = "list-midi-devices",
  .description = "List available MIDI devices",
  .usage = "list-midi-devices",
  .arg_spec = arg_spec,
  .run = run,
};

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args)
{
  (void) args;

  const char **ports = jack_get_ports(context->client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput);

  if (!ports || ports[0] == NULL) {
    log_info("No MIDI devices");
    return NULL;
  }

  for (; *ports; ++ports) {
    log_info(" * %s", *ports);
  }

  return NULL;
}

