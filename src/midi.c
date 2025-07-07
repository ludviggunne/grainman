#include <stdio.h>
#include <string.h>

#include "context.h"
#include "xmalloc.h"
#include "error.h"
#include "midi.h"

char *midi_device_add(struct context *context, const char *name, const char *pattern)
{
  char *error;
  const char **ports = jack_get_ports(context->client, pattern,
                                      JACK_DEFAULT_MIDI_TYPE,
                                      JackPortIsOutput | JackPortIsPhysical);

  if (ports == NULL || ports[0] == NULL) {
    return printf_alloc("No MIDI device matching pattern '%s'", pattern);
  }

  struct midi_device device = {
    .name = strdup(name),
    .source_port = strdup(ports[0]),
    .port = jack_port_register(context->client, name,
                               JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0),
  };

  jack_free(ports);

  if (device.port == NULL) {
    error = printf_alloc("Unable to register MIDI port for device '%s'", name);
    xfree(device.name);
    xfree(device.source_port);
    return error;
  }

  midi_device_list_append(&context->midi_devices, device);

  return NULL;
}

void midi_device_destroy(struct context *context, struct midi_device *device)
{
  jack_disconnect(context->client, device->source_port, device->name);
  xfree(device->name);
  xfree(device->source_port);
}

#define LIST_TYPE        midi_device_list
#define LIST_ELEMENT     struct midi_device
#define LIST_KEY(device) device.name
#define LIST_IMPLEMENTATION
#include "list.h"
