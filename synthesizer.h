#ifndef SYNTHESIZER_H
#define SYNTHESIZER_H

#include <stddef.h>

///////////////////////////////////////////////////////////////////////////////
// General synthesizer-stuff
///////////////////////////////////////////////////////////////////////////////

// TODO: More text how it works would be fine :o)

typedef struct _synthesizer_patch_operation
{
    enum
    {
        nullary,
        unary,
        binary
    } type;

    union
    {
        struct
        {
            float (*operate_fn)(void* data, float frequency);
        } nullary_data;

        struct
        {
            float (*operate_fn)(void* data, float a, float frequency);
            struct _synthesizer_patch_operation* child;
        } unary_data;

        struct
        {
            float (*operate_fn)(void*, float a, float b, float frequency);
            struct _synthesizer_patch_operation* first_child;
            struct _synthesizer_patch_operation* second_child;
        } binary_data;
    };

    void (*release_fn)(void* data);
    void (*reset_data_fn)(void* data);

    void* data;
} _synthesizer_patch_operation;

typedef struct synthesizer_patch
{
    float volume;
    _synthesizer_patch_operation* operations[];
} synthesizer_patch;

void synthesizer_initialize(unsigned int sample_rate, void (*track)(unsigned long));

// note is the halftone to play. 0 is defined as the chamber tone, 440 Hz,
// all others are relativ to it, while one octave has 12 halftones and each
// octave doubles the frequency
void synthesizer_play_note(synthesizer_patch* patch, int note, float duration);

void synthesizer_render(float buffer[], size_t length);


///////////////////////////////////////////////////////////////////////////////
// Patch Operations
///////////////////////////////////////////////////////////////////////////////

// common stuff for all generators
typedef struct _synthesizer_generator_data
{
    enum flags
    {
        ignore_note_frequency = 0x1
    };

    float pitch;
    float phase;
    unsigned int flags;
} _synthesizer_generator_data;

void _synthesizer_generator_reset_data(void* data);

// sine generator
#define synthesizer_generator_sine(pitch_, flags_) \
    &(_synthesizer_patch_operation) { \
        .type = nullary, \
        .nullary_data.operate_fn = _synthesizer_generate_sine, \
        .release_fn = NULL, \
        .reset_data_fn = _synthesizer_generator_reset_data, \
        .data = &(_synthesizer_generator_data) { \
            .pitch = (pitch_), \
            .phase = 0.0f, \
            .flags = (flags_) \
        } \
    }

float _synthesizer_generate_sine(void* data, float frequency);

// square generator
#define synthesizer_generator_square(pitch_, flags_) \
    &(_synthesizer_patch_operation) { \
        .type = nullary, \
        .nullary_data.operate_fn = _synthesizer_generate_square, \
        .release_fn = NULL, \
        .reset_data_fn = _synthesizer_generator_reset_data, \
        .data = &(_synthesizer_generator_data) { \
            .pitch = (pitch_), \
            .phase = 0.0f, \
            .flags = (flags_) \
        } \
    }

float _synthesizer_generate_square(void* data, float frequency);

// white noise generator
#define synthesizer_generator_white_noise() \
    &(_synthesizer_patch_operation) { \
        .type = nullary, \
        .nullary_data.operate_fn = _synthesizer_generate_white_noise, \
        .release_fn = NULL, \
        .reset_data_fn = NULL, \
        .data = NULL \
    }

float _synthesizer_generate_white_noise(void* data, float frequency);

// add operation
#define synthesizer_add(first_child_, second_child_) \
    &(_synthesizer_patch_operation) { \
        .type = binary, \
        .binary_data = {  \
            .operate_fn = _synthesizer_add, \
            .first_child = (first_child_), \
            .second_child = (second_child_), \
        }, \
        .release_fn = NULL, \
        .reset_data_fn = NULL, \
        .data = NULL \
    }

float _synthesizer_add(void* data, float a, float b, float frequency);

// multiply operation
#define synthesizer_multiply(first_child_, second_child_) \
    &(_synthesizer_patch_operation) { \
        .type = binary, \
        .binary_data = {  \
            .operate_fn = _synthesizer_multiply, \
            .first_child = (first_child_), \
            .second_child = (second_child_), \
        }, \
        .release_fn = NULL, \
        .reset_data_fn = NULL, \
        .data = NULL \
    }

float _synthesizer_multiply(void* data, float a, float b, float frequency);

// ADSR-envelope
#define synthesizer_asdr_envelope(child_, attack_time_, \
        decay_time_, sustain_level_, release_time_) \
    &(_synthesizer_patch_operation) { \
        .type = unary, \
        .unary_data = { \
            .operate_fn = _synthesizer_adsr_envelope, \
            .child = (child_) \
        }, \
        .release_fn = _synthesizer_adsr_envelope_release, \
        .reset_data_fn = _synthesizer_adsr_envelope_reset_data, \
        .data = &(_synthesizer_adsr_envelope_data) { \
            .attack_time = (attack_time_), \
            .decay_time = (decay_time_), \
            .sustain_level = (sustain_level_), \
            .release_time = (release_time_), \
            .phase = attack, \
            .level = 0.0f \
        } \
    }

typedef struct _synthesizer_adsr_envelope_data
{
    float attack_time;
    float decay_time;
    float sustain_level;
    float release_time;

    enum
    {
        attack,
        decay,
        sustain,
        release
    } phase;

    float level;
} _synthesizer_adsr_envelope_data;

float _synthesizer_adsr_envelope(void* data, float a, float frequency);
void _synthesizer_adsr_envelope_release(void* data);
void _synthesizer_adsr_envelope_reset_data(void* data);

// flanger effect
#define synthesizer_flanger(input, delay_controller, max_delay_) \
    &(_synthesizer_patch_operation) { \
        .type = binary, \
        .binary_data = { \
            .operate_fn = _synthesizer_flange, \
            .first_child = (input), \
            .second_child = (delay_controller) \
        }, \
        .release_fn = NULL, \
        .reset_data_fn = _synthesizer_flanger_reset, \
        .data = &(_synthesizer_flanger_data) { \
            .max_delay = (max_delay_), \
            .position = 0 \
        } \
    }

typedef struct _synthesizer_flanger_data
{
    float max_delay;
    size_t position;
    float buffer[1024];
} _synthesizer_flanger_data;

float _synthesizer_flange(void* data, float a, float b, float frequency);
void _synthesizer_flanger_reset(void* data);

#endif /* SYNTHESIZER_H */

