#ifndef EVENT_H
#define EVENT_H

#include <jack/midiport.h>

enum event_type {
  EVENT_FD,
  EVENT_MESSAGE,
  EVENT_MIDI,
  EVENT_ERROR,
  EVENT_SHUTDOWN,
};

struct event {
  enum event_type      type;
  int                  fd;
  jack_midi_event_t    midi;
  size_t               device_index;
  char                *error;
  char                *message;
};

struct event_loop;

struct event_loop *event_loop_create(int *fds, size_t nfds);
void event_loop_destroy(struct event_loop *event_loop);
struct event event_loop_poll(struct event_loop *event_loop);
void event_loop_enqueue_event(struct event_loop *event_loop, struct event *event);
void event_loop_lock(struct event_loop *event_loop);
void event_loop_unlock(struct event_loop *event_loop);

#endif
