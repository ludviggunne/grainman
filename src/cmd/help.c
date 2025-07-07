#include "../error.h"
#include "cmd.h"

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args);

static enum cmd_arg_type arg_spec[CMD_MAX_ARGS] = { CMD_ARG_END, };

struct cmd help_cmd = {
  .name = "help",
  .description = "List list available commands",
  .usage = "help",
  .arg_spec = arg_spec,
  .run = run,
};

extern const struct cmd *const cmd_list[];

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args)
{
  (void) path_stack;
  (void) args;

  struct event event = {
    .type = EVENT_MESSAGE,
  };

  for (const struct cmd *const *cmd = cmd_list; *cmd; ++cmd) {
    event.message = printf_alloc("\x1b[1m%s\x1b[0m: %s. Usage: %s", (*cmd)->name,
                                 (*cmd)->description, (*cmd)->usage);
    event_loop_enqueue_event(context->event_loop, &event);
  }

  return NULL;
}

