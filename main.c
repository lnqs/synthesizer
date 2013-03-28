#include "dsp.h"
#include "synthesizer.h"

static const unsigned int sample_rate = 44100;
static const unsigned channels = 1;

synthesizer_patch test = {
    .volume = 1.0f,
    .operations = {
        synthesizer_operator_add,
        synthesizer_generator_sine(440.0f),
        synthesizer_generator_sine(128.0f),
    }
};

int main(int argc, char** argv)
{
    float buffer[1024];
    dsp_initialize(sample_rate, channels);

    synthesizer_initialize(sample_rate);

    synthesizer_play_note(&test, 0);

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

