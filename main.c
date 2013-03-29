#include "dsp.h"
#include "synthesizer.h"

static const unsigned int sample_rate = 44100;
static const unsigned channels = 1;

synthesizer_patch test = {
    .volume = 1.0f,
    .operations = {
        synthesizer_asdr_envelope(0.5f, 0.75f, 0.5f, 0.5f),
        synthesizer_generator_sine(0.0f),
        synthesizer_patch_end
    }
};

int main(int argc, char** argv)
{
    float buffer[1024];
    dsp_initialize(sample_rate, channels);

    synthesizer_initialize(sample_rate);

    synthesizer_play_note(&test, 0, 1.0f);

    while (1)
    {
        for (size_t i = 0; i < sizeof(buffer) / sizeof(float); i++)
        {
            buffer[i] = synthesizer_render_sample();
        }

        dsp_write(buffer, sizeof(buffer) / sizeof(float));
    }

    dsp_destroy();
    return 0;
}

