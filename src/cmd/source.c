#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "../xmalloc.h"
#include "../error.h"

#include "cmd.h"

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args);

static enum cmd_arg_type arg_spec[CMD_MAX_ARGS] = {
  CMD_ARG_FILE_NAME,

  CMD_ARG_END,
};

struct cmd source_cmd = {
  .name = "source",
  .description = "Run commands from a file",
  .usage = "source <file>",
  .arg_spec = arg_spec,
  .run = run,
};

static char *run(struct context *context, struct path_stack **path_stack, const struct cmd_arg *args)
{
  /* We use 'path_stack' so nested 'source' can refer to relative paths */
  char *filename = get_path(path_stack, args[0].string);
  char *error;

  struct stat st;
  if (stat(filename, &st) < 0) {
    error = printf_alloc("Failed to source file %s: %s", filename, strerror(errno));
    xfree(filename);
    return error;
  }

  if (!S_ISREG(st.st_mode)) {
    error = printf_alloc("Failed to source file %s: Not a regular file", filename);
    xfree(filename);
    return error;
  }

  const char *extension = strrchr(filename, '.');
  int is_shell_script = extension && strcmp(extension, ".sh") == 0;

  int (*close_fn)(FILE *f);
  FILE *f;
  char buf[4096];
  const char *shell = "bash";

  if (is_shell_script) {
    snprintf(buf, sizeof(buf), "%s %s", shell, filename);
    f = popen(buf, "r");
    close_fn = pclose;
  } else {
    f = fopen(filename, "r");
    close_fn = fclose;
  }

  if (f == NULL) {
    error = printf_alloc("Unable to source file %s: %s", filename, strerror(errno));
    xfree(filename);
    return error;
  }

  path_stack_push(path_stack, args[0].string);

  char *line = NULL;
  size_t size;

  int lineno = 1;

  while (getline(&line, &size, f) >= 0) {

    if (line[strlen(line)-1] == '\n') {
      line[strlen(line)-1] = 0;
    }

    if (strlen(line) > 0 && line[0] != '#') {
      struct event event = {
        .type = EVENT_MESSAGE,
        .message = printf_alloc("%s:%d: \x1b[2;3m%s\x1b[0m", filename, lineno, line),
      };
      int maxlen = 160;
      if (strlen(event.message) > maxlen) {
        char *tmp = printf_alloc("%.*s...\x1b[0m", maxlen, event.message);
        xfree(event.message);
        event.message = tmp;
      }
      event_loop_enqueue_event(context->event_loop, &event);
    }

    char *subcmd_error = cmd_interpret(context, path_stack, line);
    if (subcmd_error) {
      error = printf_alloc("%s:%d -> %s", filename, lineno, subcmd_error);
      xfree(subcmd_error);
      close_fn(f);
      xfree(line);
      xfree(filename);
      path_stack_pop(path_stack);
      return error;
    }

    lineno++;
  }

  int err = close_fn(f);
  path_stack_pop(path_stack);

  if (err < 0) {
    return printf_alloc("pclose: %s", strerror(errno));
  } else if (err > 0) {
    return printf_alloc("%s: %s returned %d", filename, shell, WEXITSTATUS(err));
  }

  xfree(line);
  xfree(filename);
  
  return NULL;
}

