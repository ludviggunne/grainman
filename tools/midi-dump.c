#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <jack/midiport.h>

struct midi_event {
  unsigned char buffer[3];
  jack_nframes_t offset;
};

static jack_client_t *client = NULL;
static jack_port_t *port = NULL;
static int client_is_activated = 0;
static FILE *input = NULL;
static char *line = NULL;
static jack_ringbuffer_t *ringbuffer = NULL;
static jack_nframes_t sample_rate = 0;

static void cleanup(void)
{
  if (port)
    jack_port_unregister(client, port);

  if (client_is_activated)
    jack_deactivate(client);

  if (client)
    jack_client_close(client);

  if (input && input != stdin)
    fclose(input);

  if (ringbuffer)
    jack_ringbuffer_free(ringbuffer);

  free(line);
}

static int process_callback(jack_nframes_t nframes, void *arg)
{
  (void) arg;

  jack_nframes_t offset = 0;
  static jack_nframes_t skipped = 0;

  void *port_buffer = jack_port_get_buffer(port, nframes);
  jack_midi_clear_buffer(port_buffer);

  struct midi_event event;

  while (jack_ringbuffer_peek(ringbuffer, (char *) &event, sizeof(event)) == sizeof(event)) {

    jack_nframes_t event_offset;
    if (event.offset < skipped) {
      event_offset = 0;
    } else {
      event_offset = event.offset - skipped;
    }

    if (event_offset + offset > nframes)
      break;

    skipped = 0;
    offset += event_offset;
    jack_ringbuffer_read(ringbuffer, (char *) &event, sizeof(event));

    unsigned char *write_buffer = jack_midi_event_reserve(port_buffer, offset, 3);
    memcpy(write_buffer, event.buffer, 3);
  }

  skipped += nframes - offset;

  return 0;
}

static int srate_callback(jack_nframes_t new_sample_rate, void *arg)
{
  (void) arg;

  sample_rate = new_sample_rate;

  return 0;
}

int main(int argc, char **argv)
{
  atexit(cleanup);

  jack_client_t *client = jack_client_open("midi-dump", 0, NULL);

  if (client == NULL) {
    fprintf(stderr, "Failed to open client\n");
    exit(1);
  }

  port = jack_port_register(client, "midi-out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

  if (port == NULL) {
    fprintf(stderr, "Failed to register port\n");
    exit(1);
  }

  jack_set_process_callback(client, process_callback, NULL);
  jack_set_sample_rate_callback(client, srate_callback, NULL);

  ringbuffer = jack_ringbuffer_create(1024 * 3);

  if (jack_activate(client) != 0) {
    fprintf(stderr, "Failed to activate client\n");
    exit(1);
  }

  client_is_activated = 1;
  sample_rate = jack_get_sample_rate(client);

  input = stdin;

  if (argc > 1) {
    const char *path = argv[1];
    input = fopen(path, "r");

    if (input == NULL) {
      fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
      exit(1);
    }
  }

  size_t size;

  while (getline(&line, &size, input) >= 0) {

    unsigned int status, data1, data2;
    float f_offset;
    int result = sscanf(line, "%u %u %u %f", &status, &data1, &data2, &f_offset);

    if (result < 3 || status > 255 || data1 > 255 || data2 > 255 || f_offset < 0.f) {
      fprintf(stderr, "Invalid input: %s", line);
      continue;
    }

    jack_nframes_t offset = sample_rate * f_offset;

    struct midi_event event = {
        .buffer = { status, data1, data2, },
        .offset = offset,
    };

    if (jack_ringbuffer_write(ringbuffer, (const char *) &event, sizeof(event)) < sizeof(event)) {
      fprintf(stderr, "Ringbuffer full!\n");
    }
  }

  struct midi_event dummy;
  while (jack_ringbuffer_peek(ringbuffer, (char *) &dummy, sizeof(dummy)) > 0)
    ;
}
