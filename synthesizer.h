#ifndef SYNTHESIZER_H
#define SYNTHESIZER_H

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
        float (*nullary_fn)(void*);
        float (*unary_fn)(void*, float);
        float (*binary_fn)(void*, float, float);
    };

    void (*reset_data_fn)(void*);

    void* data;
} _synthesizer_patch_operation;

typedef struct synthesizer_patch
{
    float volume;
    _synthesizer_patch_operation operations[];
} synthesizer_patch;

void synthesizer_initialize(unsigned int sample_rate);

// note is the halftone to play. 0 is defined as the chamber tone, 440 Hz,
// all others are relativ to it, while one octave has 12 halftones and each
// octave doubles the frequency
void synthesizer_play_note(synthesizer_patch* patch, int note);

float synthesizer_render_sample();


///////////////////////////////////////////////////////////////////////////////
// Patch Operations
///////////////////////////////////////////////////////////////////////////////

// sine generator
#define synthesizer_generator_sine(frequency_) \
    (_synthesizer_patch_operation) { \
        .type = nullary, \
        .nullary_fn = _synthesizer_generate_sine, \
        .reset_data_fn = _synthesizer_generator_sine_reset_data, \
        .data = &(_synthesizer_generator_sine_data) { \
            .frequency = (frequency_), \
            .phase = 0.0f \
        } \
    }

typedef struct _synthesizer_generator_sine_data
{
    float frequency;
    float phase;
} _synthesizer_generator_sine_data;

float _synthesizer_generate_sine(void* data);
void _synthesizer_generator_sine_reset_data(void* data);

// add operation
#define synthesizer_operator_add \
    (_synthesizer_patch_operation) { \
        .type = binary, \
        .binary_fn = _synthesizer_operate_add, \
        .reset_data_fn = NULL, \
        .data = NULL \
    }

float _synthesizer_operate_add(void* data, float a, float b);

// ADSR-envelope
#define synthesizer_asdr_envelope(attack_time_, \
        decay_time_, sustain_level_, release_time_) \
    (_synthesizer_patch_operation) { \
        .type = unary, \
        .unary_fn = _synthesizer_adsr_envelope, \
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

float _synthesizer_adsr_envelope(void* data, float a);
void _synthesizer_adsr_envelope_reset_data(void* data);

#endif /* SYNTHESIZER_H */

