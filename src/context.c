#include <string.h>
#include <unistd.h>
#include <jack/midiport.h>

#include "xmalloc.h"
#include "error.h"
#include "midi-cc-map.h"
#include "context.h"
#include "event.h"
#include "synthesize.h"

const char *const client_name = "grainman";

static int sample_rate_callback(jack_nframes_t sample_rate, void *arg);
static int process_callback(jack_nframes_t nframes, void *arg);

char *context_init(struct context *context, enum log_level log_level)
{
  memset(context, 0, sizeof(*context));

  int fds[] = { STDIN_FILENO };

  context->midi_cc_map = midi_cc_map_create();
  context->event_loop = event_loop_create(fds, 1);
  context->midi_devices = midi_device_list_create();
  context->samples = sample_list_create();
  context->profiles = profile_list_create();

  char *error_str = NULL;

  jack_status_t status;
  context->client = jack_client_open(client_name, 0, &status);
  if (context->client == NULL) {
    error_str = strdup("Failed to open Jack client");
    goto failure;
  }

  if (status & JackServerStarted) {
    log_info("Started Jack server");
  }

  if (jack_set_process_callback(context->client, process_callback, context) != 0) {
    error_str = strdup("Failed to set process callback for Jack client");
    goto failure;
  }

  if (jack_set_sample_rate_callback(context->client, sample_rate_callback, context) != 0) {
    error_str = strdup("Failed to set sample rate callback for Jack client");
    goto failure;
  }

  context->audio_port = jack_port_register(context->client, "audio_out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  if (context->audio_port == NULL) {
    error_str = strdup("Failed to register Jack audio port");
    goto failure;
  }

  logging_init(context->event_loop, log_level);

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutex_init(&context->lock, &attr);

  return NULL;

failure:
  context_destroy(context);
  return error_str;
}

void context_destroy(struct context *context)
{
  for (size_t i = 0; i < context->midi_devices.size; ++i) {
    midi_device_destroy(context, &context->midi_devices.data[i]);
  }
  midi_device_list_destroy(&context->midi_devices);

  for (size_t i = 0; i < context->samples.size; ++i) {
    sample_destroy(&context->samples.data[i]);
  }
  sample_list_destroy(&context->samples);

  for (size_t i = 0; i < context->profiles.size; ++i) {
    xfree(context->profiles.data[i].name);
  }
  profile_list_destroy(&context->profiles);

  event_loop_destroy(context->event_loop);
  midi_cc_map_destroy(context->midi_cc_map);

  if (context->client) {
    jack_client_close(context->client);
  }
}

#define push_message(...)                                \
  event.message = printf_alloc(__VA_ARGS__);             \
  event_loop_enqueue_event(context->event_loop, &event); \

void context_dump(struct context *context)
{
  struct event event;
  event.type = EVENT_MESSAGE;

  push_message("Samples:");
  for (size_t i = 0; i < context->samples.size; i++) {
    push_message(" * %s", context->samples.data[i].name);
  }

  push_message("Profiles:");
  for (size_t i = 0; i < context->profiles.size; i++) {
    push_message(" * %s", context->profiles.data[i].name);
  }

  push_message("MIDI devices:");
  for (size_t i = 0; i < context->midi_devices.size; i++) {
    push_message(" * %s (%s)", context->midi_devices.data[i].name, context->midi_devices.data[i].source_port);
  }
}

static int sample_rate_callback(jack_nframes_t sample_rate, void *arg)
{
  int ret = 0;
  struct context *context = arg;
  char *error = NULL;

  log_debug("sample_rate_callback(%d, ...)", sample_rate);

  for (size_t i = 0; i < context->samples.size; ++i) {
    struct sample *sample = &context->samples.data[i];
    // sample_lock(sample);

    if (sample->sample_rate != sample_rate) {
      error = resample(sample, sample_rate);
    }

    if (error == NULL && sample->channels != 1) {
      downmix(sample);
    }

    // sample_unlock(sample);

    if (error != NULL) {
      log_error("Failed to resample sample %s: %s", sample->name, error);
      ret = 1;
    }
  }

  return ret;
}

char *context_start_audio_processing(struct context *context)
{
  pthread_mutex_lock(&context->lock);

  if (jack_activate(context->client) != 0) {
    return strdup("Failed to activate Jack client");
  }

  const char **ports = jack_get_ports(context->client, NULL, NULL,
                                      JackPortIsInput | JackPortIsPhysical);
  if (ports == NULL || ports[0] == NULL) {
    jack_deactivate(context->client);
    return strdup("No available playback ports");
  }

  if (jack_connect(context->client, jack_port_name(context->audio_port), ports[0]) != 0) {
    jack_deactivate(context->client);
    char *error = printf_alloc("Failed to connect to playback port '%s'", ports[0]);
    jack_free(ports);
    return error;
  }

  log_info("Connected to playback port %s", ports[0]);

  if (ports[1] != NULL) {
    if (jack_connect(context->client, jack_port_name(context->audio_port), ports[1]) != 0) {
      jack_deactivate(context->client);
      char *error = printf_alloc("Failed to connect to playback port '%s'", ports[1]);
      jack_free(ports);
      return error;
    }

    log_info("Connected to playback port %s", ports[1]);
  }

  jack_free(ports);

  unsigned int sample_rate = jack_get_sample_rate(context->client);
  sample_rate_callback(sample_rate, context);

  pthread_mutex_unlock(&context->lock);

  char *error = NULL;
  for (size_t i = 0; i < context->midi_devices.size; i++) {
    struct midi_device *device = &context->midi_devices.data[i];
    if (jack_connect(context->client, device->source_port, jack_port_name(device->port)) != 0 && error == NULL) {
      error = printf_alloc("Unable to connect MIDI port for device '%s'", device->name);
    }
  }

  return error;
}

static int process_callback(jack_nframes_t nframes, void *arg)
{
  struct context *context = arg;
  pthread_mutex_lock(&context->lock);

  unsigned int sample_rate = jack_get_sample_rate(context->client);
  jack_default_audio_sample_t *out = jack_port_get_buffer(context->audio_port, nframes);

  memset(out, 0, sizeof(*out) * nframes);

  for (size_t i = 0; i < context->profiles.size; ++i) {
    struct profile *profile = &context->profiles.data[i];
    synthesize(context, sample_rate, profile, out, nframes);
  }

  for (size_t i = 0; i < context->midi_devices.size; ++i) {
    struct midi_device *device = &context->midi_devices.data[i];
    void *buf = jack_port_get_buffer(device->port, nframes);

    size_t event_count = jack_midi_get_event_count(buf);
    jack_midi_event_t midi_event;

    for (size_t j = 0; j < event_count; j++) {
      jack_midi_event_get(&midi_event, buf, j);
      struct event event = {
        .type = EVENT_MIDI,
        .midi = midi_event,
        .device_index = i,
      };
      event_loop_enqueue_event(context->event_loop, &event);
    }
  }

  pthread_mutex_unlock(&context->lock);

  return 0;
}
