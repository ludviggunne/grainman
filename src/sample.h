#ifndef SAMPLE_H
#define SAMPLE_H

#include <stddef.h>
#include <pthread.h>

struct sample {
  char *name;
  float *data;
  size_t frames;
  unsigned int channels;
  unsigned int sample_rate;
  pthread_mutex_t lock;
};

void sample_lock(struct sample *sample);
void sample_unlock(struct sample *sample);

char *resample(struct sample *sample, unsigned int sample_rate);
void downmix(struct sample *sample);

void sample_destroy(struct sample *sample);

#define LIST_TYPE        sample_list
#define LIST_ELEMENT     struct sample
#define LIST_KEY(sample) sample.name
#include "list.h"

#endif
