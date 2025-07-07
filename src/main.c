#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linenoise/linenoise.h>

#include "midi-cc-map.h"
#include "error.h"
#include "logging.h"
#include "xmalloc.h"
#include "context.h"
#include "cmd/cmd.h"

static struct context *context = NULL;
static struct path_stack *path_stack = NULL;

void cleanup(void)
{
  path_stack_destroy(&path_stack);
  context_destroy(context);
  xfree(context);
}

int main(int argc, char **argv)
{

  srand(time(NULL));

  context = xcalloc(1, sizeof(*context));
  char *error = context_init(context, LOG_LEVEL_INFO);
  if (error != NULL) {
    fprintf(stderr, "Error: %s\n", error);
    exit(1);
  }

  if (argc > 1) {
    char *cmd = strdup(argv[1]);
    for (int i = 2; i < argc; ++i) {
      char *tmp = cmd;
      cmd = printf_alloc("%s %s", cmd, argv[i]);
      xfree(tmp);
    }

    char *cmd_error = cmd_interpret(context, &path_stack, cmd);
    if (cmd_error != NULL) {
      log_error("Command failed: %s", cmd_error);
      xfree(cmd_error);
      struct event shutdown_event = { .type = EVENT_SHUTDOWN };
      event_loop_enqueue_event(context->event_loop, &shutdown_event);
    }

    xfree(cmd);
  }

  linenoiseHistorySetMaxLen(100);

  struct linenoiseState ln_state;
  char ln_buf[4096];
  const char *ln_prompt = "grainman> ";
  linenoiseEditStart(&ln_state, -1, -1, ln_buf, sizeof(ln_buf), ln_prompt);

  atexit(cleanup);

  int shutdown = 0;
  while (!shutdown) {
    struct event event = event_loop_poll(context->event_loop);

    switch (event.type) {
    case EVENT_FD:
      (void) event.fd; /* always STDIN */

      char *feed_result = linenoiseEditFeed(&ln_state);

      if (feed_result == linenoiseEditMore) {
        break;
      }

      linenoiseEditStop(&ln_state);

      if (feed_result == NULL) {
        struct event shutdown_event = { .type = EVENT_SHUTDOWN, };
        event_loop_enqueue_event(context->event_loop, &shutdown_event);
        break;
      }

      linenoiseHistoryAdd(feed_result);

      char *cmd_error = cmd_interpret(context, &path_stack, feed_result);
      if (cmd_error != NULL) {
        log_error("Command failed: %s", cmd_error);
        xfree(cmd_error);
      }

      linenoiseEditStart(&ln_state, -1, -1, ln_buf, sizeof(ln_buf), ln_prompt);
      break;

    case EVENT_MESSAGE:
      linenoiseHide(&ln_state);
      printf("%s%s\n", ln_prompt, event.message);
      xfree(event.message);
      linenoiseShow(&ln_state);
      break;

    case EVENT_MIDI:
    {
      log_debug("%s: %d:%d:%d", context->midi_devices.data[event.device_index].name,
                event.midi.buffer[0],
                event.midi.buffer[1],
                event.midi.buffer[2]);

      if (event.midi.buffer[0] == MIDI_CONTROL_CHANGE) {
        midi_cc_map_apply(context->midi_cc_map, context,
                          event.device_index,
                          event.midi);
      }

      /* TODO: pitches */

      break;
    }

    case EVENT_SHUTDOWN:
      shutdown = 1;
      break;

    default:
      break;
    }
  }

  linenoiseEditStop(&ln_state);
}
