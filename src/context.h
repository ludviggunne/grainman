#ifndef CONTEXT_H
#define CONTEXT_H

#include <jack/jack.h>
#include <pthread.h>

#include "event.h"
#include "sample.h"
#include "midi.h"
#include "profile.h"
#include "logging.h"

struct context {
  struct sample_list       samples;
  struct midi_device_list  midi_devices;
  struct profile_list      profiles;
  struct event_loop       *event_loop;
  struct midi_cc_map      *midi_cc_map;
  jack_client_t           *client;
  jack_port_t             *audio_port;
  pthread_mutex_t          lock;
};

char *context_init(struct context *context, enum log_level log_level);
void context_destroy(struct context *context);
void context_dump(struct context *context);
char *context_start_audio_processing(struct context *context);

extern const char *const client_name;

#endif
