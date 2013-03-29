#include "dsp.h"
#include "synthesizer.h"

static const unsigned int sample_rate = 44100;
static const unsigned channels = 1;

synthesizer_patch test = {
    .volume = 1.0f,
    .operations = {
        synthesizer_asdr_envelope(0.1f, 0.0f, 1.0f, 0.1f),
        synthesizer_generator_square(0.0f),
        synthesizer_patch_end
    }
};

void track(unsigned long position)
{
    if (position % (sample_rate * 4) == 0)
    {
        synthesizer_play_note(&test, -32, 3.0f);
    }
}

int main(int argc, char** argv)
{
    float buffer[1024];
    dsp_initialize(sample_rate, channels);

    synthesizer_initialize(sample_rate, track);

    while (1)
    {
        synthesizer_render(buffer, sizeof(buffer) / sizeof(float));
        dsp_write(buffer, sizeof(buffer) / sizeof(float));
    }

    dsp_destroy();

    return 0;
}

