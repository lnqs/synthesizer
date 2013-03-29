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
    float frequency;
    float duration_left;
    bool releasing;
} synthesizer_voices[synthesizer_voice_number];

static float synthesizer_patch_operate(
        _synthesizer_patch_operation** operation, float frequency)
{
    _synthesizer_patch_operation* operator = *operation;
    *operation += 1;

    switch (operator->type)
    {
        case nullary:
            return operator->nullary_fn(operator->data, frequency);
        case unary:
            return operator->unary_fn(operator->data,
                    synthesizer_patch_operate(operation, frequency), frequency);
        case binary:
            return operator->binary_fn(operator->data,
                    synthesizer_patch_operate(operation, frequency),
                    synthesizer_patch_operate(operation, frequency),
                    frequency);
        default:
            return 0.0f; // never reached
    }
}

static void synthesizer_patch_release(synthesizer_patch* patch)
{
    for (_synthesizer_patch_operation* operation = patch->operations;
            operation->type != end; operation++)
    {
        if (operation->release_fn)
        {
            operation->release_fn(operation->data);
        }
    }
}

static void synthesizer_patch_reset(synthesizer_patch* patch)
{
    for (_synthesizer_patch_operation* operation = patch->operations;
            operation->type != end; operation++)
    {
        if (operation->reset_data_fn)
        {
            operation->reset_data_fn(operation->data);
        }
    }
}

void synthesizer_initialize(unsigned int sample_rate)
{
    synthesizer_sample_rate = sample_rate;

    // just initialize all members to 0
    memset(synthesizer_voices, '\0', sizeof(synthesizer_voices));
}

void synthesizer_play_note(synthesizer_patch* patch, int note, float duration)
{
    for (int i = 0; i < synthesizer_voice_number; i++)
    {
        // search for the first free voice. if there's none, well...
        if (!synthesizer_voices[i].active)
        {
            synthesizer_voices[i].patch = patch;
            synthesizer_voices[i].active = true;
            synthesizer_voices[i].frequency = 440.0f * powf(2.0f, note / 12.0f);
            synthesizer_voices[i].duration_left = duration;
            synthesizer_voices[i].releasing = false;
            break;
        }
    }
}

float synthesizer_render_sample()
{
    const float sample_duration = 1.0f / synthesizer_sample_rate;

    float out = 0.0f;

    for (int i = 0; i < synthesizer_voice_number; i++)
    {
        if (synthesizer_voices[i].active)
        {
            _synthesizer_patch_operation* operation
                    = synthesizer_voices[i].patch->operations;

            out += synthesizer_voices[i].patch->volume
                    * synthesizer_patch_operate(
                            &operation, synthesizer_voices[i].frequency);

            synthesizer_voices[i].duration_left -= sample_duration;
            if (!synthesizer_voices[i].releasing
                    && synthesizer_voices[i].duration_left <= 0.0f)
            {
                synthesizer_patch_release(synthesizer_voices[i].patch);
                synthesizer_voices[i].releasing = true;
            }

            if (synthesizer_voices[i].releasing && out == 0.0f)
            {
                synthesizer_patch_reset(synthesizer_voices[i].patch);
                synthesizer_voices[i].active = false;
            }
        }
    }

    return out;
}


///////////////////////////////////////////////////////////////////////////////
// Patch Operations
///////////////////////////////////////////////////////////////////////////////

// common stuff for all generators
void _synthesizer_generator_reset_data(void* data)
{
    _synthesizer_generator_data* sine_data = data;
    sine_data->phase = 0.0f;
}

// sine generator
float _synthesizer_generate_sine(void* data, float frequency)
{
    _synthesizer_generator_data* sine_data = data;
    float sample = sinf(2.0f * (frequency + sine_data->pitch) * M_PI * sine_data->phase);
    sine_data->phase = fmodf(sine_data->phase + 1.0f / synthesizer_sample_rate, 2.0f);
    return sample;
}

// square generator
float _synthesizer_generate_square(void* data, float frequency)
{
    return _synthesizer_generate_sine(data, frequency) > 0.0f ? 1.0f : -1.0f;
}

// add operation
float _synthesizer_operate_add(void* data, float a, float b, float frequency)
{
    return a + b;
}

// ADSR-envelope
float _synthesizer_adsr_envelope(void* data, float a, float frequency)
{
    _synthesizer_adsr_envelope_data* adsr_data = data;

    const float sample_duration = 1.0f / synthesizer_sample_rate;

    switch (adsr_data->phase)
    {
        case attack:
            adsr_data->level += sample_duration / adsr_data->attack_time;
            if (adsr_data->level >= 1.0f)
            {
                adsr_data->level = 1.0f;
                adsr_data->phase = decay;
            }
            break;
        case decay:
            adsr_data->level -= sample_duration / adsr_data->decay_time;
            if (adsr_data->level <= adsr_data->sustain_level)
            {
                adsr_data->level = adsr_data->sustain_level;
                adsr_data->phase = sustain;
            }
            break;
        case sustain:
            // Yay! Do nothing!
            break;
        case release:
            adsr_data->level -= sample_duration / adsr_data->release_time;
            if (adsr_data->level <= 0.0f)
            {
                adsr_data->level = 0.0f;
            }
            break;
    }

    return adsr_data->level * a;
}

void _synthesizer_adsr_envelope_release(void* data)
{
    _synthesizer_adsr_envelope_data* adsr_data = data;
    adsr_data->phase = release;
}

void _synthesizer_adsr_envelope_reset_data(void* data)
{
    _synthesizer_adsr_envelope_data* adsr_data = data;
    adsr_data->phase = attack;
    adsr_data->level = 0.0f;
}

