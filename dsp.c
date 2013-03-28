#include "dsp.h"
#include <alloca.h>

static const char* handle_identifier = "default";

static snd_pcm_t* handle;

void dsp_initialize(unsigned int sample_rate, unsigned int channels)
{
    if (snd_pcm_open(
                &handle, handle_identifier, SND_PCM_STREAM_PLAYBACK, 0) != 0)
    {
        fprintf(stderr, "dsp: failed to open alsa device\n");
        exit(2);
    }

    snd_pcm_hw_params_t* params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);

    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_FLOAT);

    snd_pcm_hw_params_set_channels(handle, params, channels);

    unsigned int rrate = sample_rate;
    snd_pcm_hw_params_set_rate_near(handle, params, &rrate, NULL);

    if (snd_pcm_hw_params(handle, params) != 0)
    {
        fprintf(stderr, "dsp: failed to initialize alsa\n");
        exit(2);
    }
}

void dsp_destroy()
{
    snd_pcm_drop(handle);
    snd_pcm_close(handle);
}

void dsp_write(void* data, size_t length)
{
    int frames_written = snd_pcm_writei(handle, data, length);

    if (frames_written == -EBADFD)
    {
        fprintf(stderr, "dsp: bad file descriptor\n");
        snd_pcm_prepare(handle);
    }
    else if (frames_written == -EPIPE)
    {
        fprintf(stderr, "dsp: underrun occured\n");
        snd_pcm_prepare(handle);
    }
    else if (frames_written == -ESTRPIPE)
    {
        fprintf(stderr, "dsp: suspend event occurred\n");
        snd_pcm_prepare(handle);
    }
    else if (frames_written != length)
    {
        fprintf(stderr, "dsp: failed to write all frames\n");
    }
}

