#include <string.h>

#include "../context.h"
#include "../error.h"
#include "../profile.h"

#include "cmd.h"

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args);

static enum cmd_arg_type arg_spec[CMD_MAX_ARGS] = {
  CMD_ARG_NAME,
  CMD_ARG_SAMPLE_NAME,

  CMD_ARG_END,
};

struct cmd add_profile_cmd = {
  .name = "add-profile",
  .description = "Add a new profile",
  .usage = "add-profile <name> <sample>",
  .arg_spec = arg_spec,
  .run = run,
};

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args)
{
  (void) path_stack;

  const char *name = args[0].string;

  if (profile_list_find(&context->profiles, name) >= 0) {
    return printf_alloc("Profile with name %s already exists", name);
  }

  struct profile profile = {
    .name = strdup(name),
    .sample_index = args[1].index,
  };

  // log_debug("Adding profile %s", name);

  profile_list_append(&context->profiles, profile);

  return NULL;
}
