#ifndef DSP_H
#define DSP_H

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

void dsp_initialize(unsigned int sample_rate, unsigned int channels);
void dsp_destroy();
void dsp_write(void* data, size_t length);

#endif /* DSP_H */

