#include "cmd.h"

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args);

static enum cmd_arg_type arg_spec[CMD_MAX_ARGS] = { CMD_ARG_END, };

struct cmd context_dump_cmd = {
  .name = "context-dump",
  .description = "Display context information",
  .usage = "context-dump",
  .arg_spec = arg_spec,
  .run = run,
};

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args)
{
  (void) args;
  context_dump(context);
  return NULL;
}
