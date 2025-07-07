#include <string.h>
#include <sndfile.h>

#include "../context.h"
#include "../sample.h"
#include "../error.h"
#include "../xmalloc.h"
#include "cmd.h"

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args);

static enum cmd_arg_type arg_spec[CMD_MAX_ARGS] = {
  CMD_ARG_NAME,
  CMD_ARG_FILE_NAME,

  CMD_ARG_END,
};

struct cmd import_sample_cmd = {
  .name = "import-sample",
  .description = "Import a sample from an audio file",
  .usage = "import-sample <name> <file>",
  .arg_spec = arg_spec,
  .run = run,
};

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args)
{
  struct sample sample;
  char *filename;
  SF_INFO info;
  SNDFILE *file;

  sample.name = args[0].string;
  filename = get_path(path_stack, args[1].string);

  if (sample_list_find(&context->samples, sample.name) >= 0) {
    xfree(filename);
    return printf_alloc("Sample with name %s already exists", sample.name);
  }

  file = sf_open(filename, SFM_READ, &info);
  if (file == NULL) {
    char *error = printf_alloc("Unable to open audio file %s", filename);
    xfree(filename);
    return error;
  }

  sample.channels = info.channels;
  sample.sample_rate = info.samplerate;
  sample.frames = info.frames;
  sample.data = xmalloc(sample.channels * sample.frames * sizeof(*sample.data));

  sf_count_t offset = 0;
  while (offset < info.frames) {
    offset += sf_readf_float(file, sample.data + offset * sample.channels, sample.frames - offset);
  }

  sample.name = strdup(sample.name);

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutex_init(&sample.lock, &attr);

  // log_debug("Imported sample %s from file %s", sample.name, filename);

  sample_list_append(&context->samples, sample);

  xfree(filename);
  return NULL;
}
