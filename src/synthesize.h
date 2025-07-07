#ifndef SYNTHESIZE_H
#define SYNTHESIZE_H

#include "context.h"

void synthesize(struct context *context,
                unsigned int sample_rate,
                struct profile *profile,
                jack_default_audio_sample_t *out,
                jack_nframes_t nframes);

#endif
