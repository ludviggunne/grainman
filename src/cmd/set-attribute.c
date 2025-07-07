#include "../context.h"
#include "../profile.h"
#include "cmd.h"


static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args);

static enum cmd_arg_type arg_spec[CMD_MAX_ARGS] = {
  CMD_ARG_PROFILE_NAME,
  CMD_ARG_PROFILE_ATTRIBUTE_NAME,
  CMD_ARG_NUMBER,

  CMD_ARG_END,
};

struct cmd set_attribute_cmd = {
  .name = "set-attribute",
  .description = "Set a profile attribute",
  .usage = "set-attribute <profile> <name> <value>",
  .arg_spec = arg_spec,
  .run = run,
};

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args)
{
  (void) path_stack;

  size_t index = args[0].index;
  enum profile_attribute attribute = args[1].attribute;
  float value = args[2].number;

  struct profile *profiles = context->profiles.data;
  profiles[index].attributes[attribute] = value;

  // log_debug("Set attribute %d of profile %zu to %f", attribute, index, value);

  return NULL;
}
