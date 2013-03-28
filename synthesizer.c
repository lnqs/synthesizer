#include "synthesizer.h"
#include <math.h>
#include <string.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

///////////////////////////////////////////////////////////////////////////////
// General synthesizer-stuff
///////////////////////////////////////////////////////////////////////////////

static const int synthesizer_voice_number = 32;

static float synthesizer_sample_rate;

static struct
{
    synthesizer_patch* patch;
    bool active;
    float pitch;
} synthesizer_voices[synthesizer_voice_number];

static float synthesizer_patch_operate(
        _synthesizer_patch_operation** operation)
{
    _synthesizer_patch_operation* operator = *operation;
    *operation += 1;

    switch (operator->type)
    {
        case nullary:
            return operator->nullary_fn(operator->data);
        case unary:
            return operator->unary_fn(operator->data,
                    synthesizer_patch_operate(operation));
        case binary:
            return operator->binary_fn(operator->data,
                    synthesizer_patch_operate(operation),
                    synthesizer_patch_operate(operation));
    }

    return 0.0f; // never reached
}

void synthesizer_initialize(unsigned int sample_rate)
{
    synthesizer_sample_rate = sample_rate;

    // just initialize all members to 0
    memset(synthesizer_voices, '\0', sizeof(synthesizer_voices));
}

void synthesizer_play_note(synthesizer_patch* patch, int note)
{
    for (int i = 0; i < synthesizer_voice_number; i++)
    {
        // search for the first free voice. if there's none, well...
        if (!synthesizer_voices[i].active)
        {
            synthesizer_voices[i].patch = patch;
            synthesizer_voices[i].active = true;
            synthesizer_voices[i].pitch = 440.0f * powf(2.0f, note / 12.0f);
            break;
        }
    }
}

float synthesizer_render_sample()
{
    float out = 0.0f;

    for (int i = 0; i < synthesizer_voice_number; i++)
    {
        if (synthesizer_voices[i].active)
        {
            _synthesizer_patch_operation* operation
                    = synthesizer_voices[i].patch->operations;
            out += synthesizer_voices[i].patch->volume
                    * synthesizer_patch_operate(&operation);
        }
    }

    return out;
}


///////////////////////////////////////////////////////////////////////////////
// Patch Operations
///////////////////////////////////////////////////////////////////////////////

// sine generator
float _synthesizer_generate_sine(void* data)
{
    _synthesizer_generator_sine_data* sine_data = data;
    float sample = sinf(2.0f * sine_data->frequency * M_PI * sine_data->phase);
    sine_data->phase = fmodf(sine_data->phase + 1.0f / synthesizer_sample_rate, 2.0f);
    return sample;
}

void _synthesizer_generator_sine_reset_data(void* data)
{
    _synthesizer_generator_sine_data* sine_data = data;
    sine_data->phase = 0.0f;
}

// add operation
float _synthesizer_operate_add(void* data, float a, float b)
{
    return a + b;
}

