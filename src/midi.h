#ifndef MIDI_H
#define MIDI_H

#include <jack/jack.h>
#include <pthread.h>

struct context;

#define MIDI_CONTROL_CHANGE 176

struct midi_device {
  char                    *name;
  char                    *source_port;
  jack_port_t             *port;
};

char *midi_device_add(struct context *context, const char *name, const char *pattern);
void midi_device_destroy(struct context *context, struct midi_device *device);
void midi_devices_list(struct context *context);

#define LIST_TYPE        midi_device_list
#define LIST_ELEMENT     struct midi_device
#define LIST_KEY(device) device.name
#include "list.h"

#endif
