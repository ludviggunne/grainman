#include <string.h>
#include <jack/jack.h>

#include "cmd.h"

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args);

static enum cmd_arg_type arg_spec[CMD_MAX_ARGS] = { CMD_ARG_END, };

struct cmd start_cmd = {
  .name = "start",
  .description = "Start the fun",
  .usage = "start",
  .arg_spec = arg_spec,
  .run = run,
};

extern const struct cmd *const cmd_list[];

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args)
{
  (void) path_stack;
  (void) args;
  return context_start_audio_processing(context);
}


