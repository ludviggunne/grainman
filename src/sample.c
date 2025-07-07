#include <string.h>
#include <samplerate.h>

#include "logging.h"
#include "xmalloc.h"
#include "sample.h"
#include "error.h"

char *resample(struct sample *sample, unsigned int sample_rate)
{
  double ratio = (double) sample_rate / (double) sample->sample_rate;
  size_t new_frames = (size_t) (sample->frames * ratio);
  float *new_data = xmalloc(sizeof(*new_data) * new_frames * sample->channels);

  log_info("Resampling sample '%s' from %d to %d Hz", sample->name, sample->sample_rate, sample_rate);

  SRC_DATA src_data = {
    .data_in = sample->data,
    .data_out = new_data,
    .input_frames = sample->frames,
    .output_frames = new_frames,
    .src_ratio = ratio,
  };

  int error = src_simple(&src_data, SRC_SINC_BEST_QUALITY, sample->channels);

  if (error != 0) {
    xfree(new_data);
    return printf_alloc("Failed to resample '%s': %s", sample->name,
                        src_strerror(error));
  }

  xfree(sample->data);
  sample->data = new_data;
  sample->frames = new_frames;
  sample->sample_rate = sample_rate;

  return NULL;
}

void downmix(struct sample *sample)
{
  float *new_data = xmalloc(sizeof(*new_data) * sample->frames);

  log_debug("Downmixing sample '%s' from %d to 1 channel", sample->name, sample->channels);

  for (size_t i = 0; i < sample->frames; ++i) {
    new_data[i] = 0.f;
    for (size_t c = 0; c < sample->channels; ++c) {
      size_t index = sample->channels * i + c;
      new_data[i] += sample->data[index];
    }
    new_data[i] /= sample->channels;
  }

  xfree(sample->data);
  sample->channels = 1;
  sample->data = new_data;
}

void sample_destroy(struct sample *sample)
{
  xfree(sample->name);
  xfree(sample->data);
  memset(sample, 0, sizeof(*sample));
}

void sample_lock(struct sample *sample)
{
  pthread_mutex_lock(&sample->lock);
}

void sample_unlock(struct sample *sample)
{
  pthread_mutex_unlock(&sample->lock);
}


#define LIST_TYPE        sample_list
#define LIST_ELEMENT     struct sample
#define LIST_KEY(sample) sample.name
#define LIST_IMPLEMENTATION
#include "list.h"
