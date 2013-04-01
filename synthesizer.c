#include "synthesizer.h"
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

///////////////////////////////////////////////////////////////////////////////
// General synthesizer-stuff
///////////////////////////////////////////////////////////////////////////////

#define synthesizer_voice_number 32

static unsigned int synthesizer_sample_rate;
static void (*synthesizer_track)(unsigned long);

static unsigned long synthesizer_position = 0;

static struct
{
    synthesizer_patch* patch;
    bool active;
    float frequency;
    float duration_left;
    bool releasing;
} synthesizer_voices[synthesizer_voice_number];

static float synthesizer_patch_operate(
        _synthesizer_patch_operation* operation, float frequency)
{
    switch (operation->type)
    {
        case nullary:
            return operation->nullary_data.operate_fn(operation->data, frequency);
        case unary:
            return operation->unary_data.operate_fn(operation->data,
                    synthesizer_patch_operate(operation->unary_data.child, frequency),
                    frequency);
        case binary:
            return operation->binary_data.operate_fn(operation->data,
                    synthesizer_patch_operate(operation->binary_data.first_child, frequency),
                    synthesizer_patch_operate(operation->binary_data.second_child, frequency),
                    frequency);
        default:
            return 0.0f; // never reached
    }
}

static void synthesizer_patch_visit_operations(
        _synthesizer_patch_operation* operation,
        void (*visit_fn)(_synthesizer_patch_operation*))
{
    switch (operation->type)
    {
        case nullary:
            break;
        case unary:
            synthesizer_patch_visit_operations(
                    operation->unary_data.child, visit_fn);
            break;
        case binary:
            synthesizer_patch_visit_operations(
                    operation->binary_data.first_child, visit_fn);
            synthesizer_patch_visit_operations(
                    operation->binary_data.second_child, visit_fn);
            break;
    }

    visit_fn(operation);
}

static void synthesizer_operation_release(_synthesizer_patch_operation* operation)
{
    if (operation->release_fn)
    {
        operation->release_fn(operation->data);
    }
}

static void synthesizer_operation_reset(_synthesizer_patch_operation* operation)
{
    if (operation->reset_data_fn)
    {
        operation->reset_data_fn(operation->data);
    }
}

void synthesizer_initialize(unsigned int sample_rate, void (*track)(unsigned long))
{
    synthesizer_sample_rate = sample_rate;
    synthesizer_track = track;

    // just initialize all members to 0
    memset(synthesizer_voices, '\0', sizeof(synthesizer_voices));
}

void synthesizer_play_note(synthesizer_patch* patch, int note, float duration)
{
    for (int i = 0; i < synthesizer_voice_number; i++)
    {
        if (synthesizer_voices[i].active && synthesizer_voices[i].patch == patch)
        {
            fprintf(stderr, "patch at %p is already in use\n", patch);
            exit(2);
        }
    }

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

void synthesizer_render(float buffer[], size_t length)
{
    const float sample_duration = 1.0f / synthesizer_sample_rate;

    for (size_t p = 0; p < length; p++)
    {
        synthesizer_track(synthesizer_position++);

        float sample = 0.0f;

        for (int v = 0; v < synthesizer_voice_number; v++)
        {
            if (synthesizer_voices[v].active)
            {
                sample += synthesizer_voices[v].patch->volume
                        * synthesizer_patch_operate(
                                synthesizer_voices[v].patch->operations[0],
                                synthesizer_voices[v].frequency);

                synthesizer_voices[v].duration_left -= sample_duration;
                if (!synthesizer_voices[v].releasing
                        && synthesizer_voices[v].duration_left <= 0.0f)
                {
                    synthesizer_patch_visit_operations(
                            synthesizer_voices[v].patch->operations[0],
                            synthesizer_operation_release);

                    synthesizer_voices[v].releasing = true;
                }

                if (synthesizer_voices[v].releasing && sample == 0.0f)
                {
                    synthesizer_patch_visit_operations(
                            synthesizer_voices[v].patch->operations[0],
                            synthesizer_operation_reset);

                    synthesizer_voices[v].active = false;
                }
            }
        }

        buffer[p] = sample;
    }
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

    if (sine_data->flags & ignore_note_frequency)
    {
        frequency = sine_data->pitch;
    }
    else
    {
        frequency += sine_data->pitch;
    }

    float sample = sinf(2.0f * frequency * M_PI * sine_data->phase);
    sine_data->phase = fmodf(sine_data->phase + 1.0f / synthesizer_sample_rate, 2.0f);
    return sample;
}

// square generator
float _synthesizer_generate_square(void* data, float frequency)
{
    return _synthesizer_generate_sine(data, frequency) > 0.0f ? 1.0f : -1.0f;
}

// add operation
float _synthesizer_add(void* data, float a, float b, float frequency)
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

// flanger effect
float _synthesizer_flange(void* data, float a, float b, float frequency)
{
    _synthesizer_flanger_data* flanger_data = data;
    const size_t buffer_length = sizeof(flanger_data->buffer) / sizeof(float);

    flanger_data->position = (flanger_data->position + 1) % buffer_length;
    flanger_data->buffer[flanger_data->position] = a;

    size_t out_position = (flanger_data->position + (size_t)(flanger_data->max_delay
                * synthesizer_sample_rate * (b / 2.0 + 1.0))) % buffer_length;
    return flanger_data->buffer[out_position] + a;
}

void _synthesizer_flanger_reset(void* data)
{
    _synthesizer_flanger_data* flanger_data = data;
    flanger_data->position = 0;
    memset(flanger_data->buffer, '\0', sizeof(flanger_data->buffer));
}

