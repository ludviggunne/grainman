#include <string.h>
#include <jack/jack.h>

#include "cmd.h"

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args);

static enum cmd_arg_type arg_spec[CMD_MAX_ARGS] = { CMD_ARG_END, };

struct cmd stop_cmd = {
  .name = "stop",
  .description = "Stop having fun",
  .usage = "stop",
  .arg_spec = arg_spec,
  .run = run,
};

extern const struct cmd *const cmd_list[];

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args)
{
  int error = jack_deactivate(context->client);
  if (error != 0) {
    return strdup("Faliled to deactivate Jack client");
  }
  return NULL;
}




