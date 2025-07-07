#ifndef PROFILE_H
#define PROFILE_H

#include <jack/jack.h>
#include <stddef.h>

#include "grain.h"

enum profile_attribute {
  PROFILE_ATTRIBUTE_MIN_LENGTH = 0,
  PROFILE_ATTRIBUTE_MAX_LENGTH,
  PROFILE_ATTRIBUTE_MIN_COOLDOWN,
  PROFILE_ATTRIBUTE_MAX_COOLDOWN,
  PROFILE_ATTRIBUTE_MIN_GAIN,
  PROFILE_ATTRIBUTE_MAX_GAIN,
  PROFILE_ATTRIBUTE_MIN_MULTIPLIER,
  PROFILE_ATTRIBUTE_MAX_MULTIPLIER,
  PROFILE_ATTRIBUTE_REVERSE_PROBABILITY,
  PROFILE_ATTRIBUTE_NUM_SLOTS,
  PROFILE_ATTRIBUTE_USE_PITCHES,
  PROFILE_ATTRIBUTE_LOCK_PITCHES,

  PROFILE_ATTRIBUTE_COUNT,      /* Number of attributes */
  PROFILE_ATTRIBUTE_NONE = -1,  /* Not an attribute */
};

struct profile {
  char          *name;
  unsigned int   sample_index;
  float          attributes[PROFILE_ATTRIBUTE_COUNT];
  int            pitches[12];
  int            pitches_locked[12];
  struct grain  *grains;
  size_t         previous_num_grains;
  size_t         grain_capacity;
};

enum profile_attribute profile_attribute_from_name(const char *name);
const char *profile_attribute_name(enum profile_attribute attribute);
void profile_clamp_attributes(struct profile *profile);

#define LIST_TYPE         profile_list
#define LIST_ELEMENT      struct profile
#define LIST_KEY(profile) profile.name
#include "list.h"

#endif
