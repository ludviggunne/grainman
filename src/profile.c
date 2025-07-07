#include <string.h>

#include "profile.h"

/* TODO: set these to reasonable values or maybe make configurable through command */
static float limits[PROFILE_ATTRIBUTE_COUNT][2] =
{
  [PROFILE_ATTRIBUTE_MIN_LENGTH]          = { .001f, 10.f },
  [PROFILE_ATTRIBUTE_MAX_LENGTH]          = { .001f, 10.f },
  [PROFILE_ATTRIBUTE_MIN_COOLDOWN]        = { .001f, 10.f },
  [PROFILE_ATTRIBUTE_MAX_COOLDOWN]        = { .001f, 10.f },
  [PROFILE_ATTRIBUTE_MIN_GAIN]            = { 0.f, 1.f },
  [PROFILE_ATTRIBUTE_MAX_GAIN]            = { 0.f, 1.f },
  [PROFILE_ATTRIBUTE_MIN_MULTIPLIER]      = { .05f, 8.f },
  [PROFILE_ATTRIBUTE_MAX_MULTIPLIER]      = { .05f, 8.f },
  [PROFILE_ATTRIBUTE_REVERSE_PROBABILITY] = { 0.f, 1.f },
  [PROFILE_ATTRIBUTE_NUM_SLOTS]           = { 0.f, 1024.f },
  [PROFILE_ATTRIBUTE_USE_PITCHES]         = { 0.f, 1.f },
  [PROFILE_ATTRIBUTE_LOCK_PITCHES]        = { 0.f, 1.f },
};

static const char *attribute_names[PROFILE_ATTRIBUTE_COUNT] =
{
  [PROFILE_ATTRIBUTE_MIN_LENGTH]          = "min-length",
  [PROFILE_ATTRIBUTE_MAX_LENGTH]          = "max-length",
  [PROFILE_ATTRIBUTE_MIN_COOLDOWN]        = "min-cooldown",
  [PROFILE_ATTRIBUTE_MAX_COOLDOWN]        = "max-cooldown",
  [PROFILE_ATTRIBUTE_MIN_GAIN]            = "min-gain",
  [PROFILE_ATTRIBUTE_MAX_GAIN]            = "max-gain",
  [PROFILE_ATTRIBUTE_MIN_MULTIPLIER]      = "min-multiplier",
  [PROFILE_ATTRIBUTE_MAX_MULTIPLIER]      = "max-multiplier",
  [PROFILE_ATTRIBUTE_REVERSE_PROBABILITY] = "reverse-probability",
  [PROFILE_ATTRIBUTE_NUM_SLOTS]           = "num-slots",
  [PROFILE_ATTRIBUTE_USE_PITCHES]         = "use-pitches",
  [PROFILE_ATTRIBUTE_LOCK_PITCHES]        = "lock-pitches",
};

static enum profile_attribute min_max_pairs[][2] =
{
  { PROFILE_ATTRIBUTE_MIN_LENGTH,     PROFILE_ATTRIBUTE_MAX_LENGTH },
  { PROFILE_ATTRIBUTE_MIN_COOLDOWN,   PROFILE_ATTRIBUTE_MAX_COOLDOWN },
  { PROFILE_ATTRIBUTE_MIN_GAIN,       PROFILE_ATTRIBUTE_MAX_GAIN },
  { PROFILE_ATTRIBUTE_MIN_MULTIPLIER, PROFILE_ATTRIBUTE_MAX_MULTIPLIER },

  /* end */
  { -1 }
};

enum profile_attribute profile_attribute_from_name(const char *name)
{
  for (enum profile_attribute attr = 0; attr < PROFILE_ATTRIBUTE_COUNT; ++attr) {
    if (strcmp(name, attribute_names[attr]) == 0) {
      return attr;
    }
  }

  return PROFILE_ATTRIBUTE_NONE;
}

const char *profile_attribute_name(enum profile_attribute attribute)
{
  return attribute_names[attribute];
}

void profile_clamp_attributes(struct profile *profile)
{
  for (size_t i = 0; i < PROFILE_ATTRIBUTE_COUNT; ++i) {
    float *attribute = &profile->attributes[i];

    float min = limits[i][0];
    float max = limits[i][1];

    if (*attribute < min) {
      *attribute = min;
    }

    if (*attribute > max) {
      *attribute = max;
    }
  }

  enum profile_attribute (*min_max_pair)[2] = min_max_pairs;
  for (; (*min_max_pair)[0] != -1; min_max_pair++) {
    float *min = &profile->attributes[(*min_max_pair)[0]];
    float *max = &profile->attributes[(*min_max_pair)[1]];

    if (*min > *max) {
      *max = *min;
    }
  }
}

#define LIST_TYPE         profile_list
#define LIST_ELEMENT      struct profile
#define LIST_KEY(profile) profile.name
#define LIST_IMPLEMENTATION
#include "list.h"
