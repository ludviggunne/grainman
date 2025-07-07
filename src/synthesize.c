#include <math.h>
#include <stdlib.h>

#include "xmalloc.h"
#include "synthesize.h"

static int randi(int min, int max)
{
  if (min >= max)
    return min;
  return min + rand() % (max - min);
}

static float randf(float min, float max)
{
  static const float epsilon = .00001f;
  if (min + epsilon >= max)
    return min;
  return min + (rand() / (float) RAND_MAX) * (max - min);
}

static void init_grain(struct profile *profile, size_t sample_rate, size_t sample_size, size_t index)
{
  struct grain *g = &profile->grains[index];

  unsigned int min_length = profile->attributes[PROFILE_ATTRIBUTE_MIN_LENGTH] * sample_rate;
  unsigned int max_length = profile->attributes[PROFILE_ATTRIBUTE_MAX_LENGTH] * sample_rate;
  unsigned int min_cooldown = profile->attributes[PROFILE_ATTRIBUTE_MIN_COOLDOWN] * sample_rate;
  unsigned int max_cooldown = profile->attributes[PROFILE_ATTRIBUTE_MAX_COOLDOWN] * sample_rate;

  float min_gain = profile->attributes[PROFILE_ATTRIBUTE_MIN_GAIN];
  float max_gain = profile->attributes[PROFILE_ATTRIBUTE_MAX_GAIN];
  float min_multiplier = profile->attributes[PROFILE_ATTRIBUTE_MIN_MULTIPLIER];
  float max_multiplier = profile->attributes[PROFILE_ATTRIBUTE_MAX_MULTIPLIER];

  g->offset = randi(0, sample_size);
  g->cooldown = randi(min_cooldown, max_cooldown);
  g->length = randi(min_length, max_length);
  g->multiplier = randf(min_multiplier, max_multiplier);
  g->gain = randf(min_gain, max_gain);
  g->cursor = 0;
}

void synthesize(struct context *context,
                unsigned int sample_rate,
                struct profile *profile,
                jack_default_audio_sample_t *out,
                jack_nframes_t nframes)
{
  profile_clamp_attributes(profile);

  size_t num_grains = profile->attributes[PROFILE_ATTRIBUTE_NUM_SLOTS];
  size_t sample_size = context->samples.data[profile->sample_index].frames;


  /* Adjust grain pool and initialize new grains */

  if (profile->previous_num_grains < num_grains) {
    if (profile->grain_capacity < num_grains) {
      profile->grains = xrealloc(profile->grains, sizeof(*profile->grains) * num_grains);
      profile->grain_capacity = num_grains;
    }

    for (size_t i = profile->previous_num_grains; i < num_grains; ++i) {
      init_grain(profile, sample_rate, sample_size, i);
    }
  }

  profile->previous_num_grains = num_grains;

  struct sample *s = &context->samples.data[profile->sample_index];

  for (size_t i = 0; i < nframes; ++i) {
    float accum = 0.f;

    for (size_t j = 0; j < profile->grain_capacity; ++j) {
      struct grain *g = &profile->grains[j];

      if (g->cooldown) {
        if (j >= num_grains) {
          continue;
        }
        g->cooldown--;
        continue;
      }

      if (g->cursor == g->length) {
        if (j < num_grains) {
          init_grain(profile, sample_rate, sample_size, j);
        }
        continue;
      }

      unsigned int cursor = g->cursor;

      if (g->reverse) {
        cursor = g->length - cursor;
      }

      /* Compute interpolated sample based on multiplier */
      float fcursor = cursor * g->multiplier;
      cursor = fcursor;
      float interp = fcursor - cursor;

      unsigned int si0 = cursor + s->frames + g->offset;
      while (si0 > s->frames) {
        si0 -= s->frames;
      }

      unsigned int si1 = si0 + 1;
      if (si1 > s->frames) {
        si1 -= s->frames;
      }

      float s0 = s->data[si0];
      float s1 = s->data[si1];

      float s = s0 + interp * (s1 - s0);

      /* Compute enveloped */
      float t = (float) g->cursor / (float) g->length;
      float env = t < .25f ? 4.f * t : 4.f * (1.f - t) / 3.f;

      accum += s * g->gain * env;

      g->cursor++;
    }

    out[i] += accum;
  }
}


