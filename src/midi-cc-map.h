#ifndef MIDI_CC_MAP_H
#define MIDI_CC_MAP_H

#include "context.h"
#include "profile.h"
#include "midi.h"

struct midi_cc_map;

struct midi_cc_map_entry {
  size_t                    device_index;
  unsigned int              function;
  size_t                    profile_index;
  enum profile_attribute    attribute;
  float                     min;
  float                     max;
  struct midi_cc_map_entry *next;
};

struct midi_cc_map *midi_cc_map_create(void);
void midi_cc_map_destroy(struct midi_cc_map *map);
void midi_cc_map_add(struct midi_cc_map *map, struct midi_cc_map_entry *entry);
void midi_cc_map_apply(struct midi_cc_map *map, struct context *context, size_t device_index, jack_midi_event_t event);

#endif
