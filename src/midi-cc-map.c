#include <string.h>

#include "xmalloc.h"
#include "midi-cc-map.h"

#define MIDI_CC_MAP_NUM_BUCKETS 101

struct bucket {
  size_t                    device_index;
  unsigned int              function;
  struct midi_cc_map_entry *entry;
  struct bucket            *next;
};

struct midi_cc_map {
  struct bucket *buckets[MIDI_CC_MAP_NUM_BUCKETS];
};

static void free_bucket(struct bucket *bucket);
static void free_entry(struct midi_cc_map_entry *entry);
static size_t bucket_index(size_t device_index, unsigned int function);
static struct bucket *find_bucket(struct midi_cc_map *map, size_t device_index, unsigned int function);
static void apply_entry(struct context *context, struct midi_cc_map_entry *entry, unsigned int value);

struct midi_cc_map *midi_cc_map_create(void)
{
  return xcalloc(1, sizeof(struct midi_cc_map));
}

void midi_cc_map_destroy(struct midi_cc_map *map)
{
  for (size_t i = 0; i < MIDI_CC_MAP_NUM_BUCKETS; ++i) {
    free_bucket(map->buckets[i]);
  }
}

void midi_cc_map_add(struct midi_cc_map *map, struct midi_cc_map_entry *entry)
{
  struct midi_cc_map_entry *added_entry = xcalloc(1, sizeof(*added_entry));
  memcpy(added_entry, entry, sizeof(struct midi_cc_map_entry));

  struct bucket *bucket = find_bucket(map, entry->device_index, entry->function);

  if (!bucket) {

    bucket = xcalloc(1, sizeof(*bucket));
    bucket->device_index = entry->device_index;
    bucket->function = entry->function;
    bucket->entry = NULL;

    size_t index = bucket_index(bucket->device_index, bucket->function);
    bucket->next = map->buckets[index];
    map->buckets[index] = bucket;
  }

  added_entry->next = bucket->entry;
  bucket->entry = added_entry;
}

void midi_cc_map_apply(struct midi_cc_map *map, struct context *context, size_t device_index, jack_midi_event_t event)
{
  unsigned int function = event.buffer[1];
  unsigned int value = event.buffer[2];

  struct bucket *bucket = find_bucket(map, device_index, function);
  if (!bucket) {
    return;
  }

  struct midi_cc_map_entry *entry = bucket->entry;
  while (entry) {
    apply_entry(context, entry, value);
    entry = entry->next;
  }
}

static void free_bucket(struct bucket *bucket)
{
  if (!bucket) {
    return;
  }

  free_entry(bucket->entry);
  free_bucket(bucket->next);
  xfree(bucket);
}

static void free_entry(struct midi_cc_map_entry *entry)
{
  if (!entry) {
    return;
  }

  free_entry(entry->next);
  xfree(entry);
}

static size_t bucket_index(size_t device_index, unsigned int function)
{
  return ((device_index + 1) * (function + 1)) % MIDI_CC_MAP_NUM_BUCKETS;
}

static struct bucket *find_bucket(struct midi_cc_map *map, size_t device_index, unsigned int function)
{
  size_t index = bucket_index(device_index, function);
  struct bucket *bucket = map->buckets[index];

  while (bucket) {
    if (bucket->device_index == device_index && bucket->function == function) {
      break;
    }
    bucket = bucket->next;
  }

  return bucket;
}

static void apply_entry(struct context *context, struct midi_cc_map_entry *entry, unsigned int value)
{
  struct profile *profiles = context->profiles.data;
  struct profile *profile = &profiles[entry->profile_index];

  float fvalue = (float) value;
  fvalue /= 128.f;
  fvalue *= entry->max - entry->min;
  fvalue += entry->min;

  const char *attribute_name = profile_attribute_name(entry->attribute);
  log_info("%s.%s = %.4f", profile->name, attribute_name, fvalue);

  profile->attributes[entry->attribute] = fvalue;
}
