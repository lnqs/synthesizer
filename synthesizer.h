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
        binary,
        end
    } type;

    union
    {
        float (*nullary_fn)(void* data, float frequency);
        float (*unary_fn)(void* data, float a, float frequency);
        float (*binary_fn)(void*, float a, float b, float frequency);
    };

    void (*release_fn)(void* data);
    void (*reset_data_fn)(void* data);

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
void synthesizer_play_note(synthesizer_patch* patch, int note, float duration);

float synthesizer_render_sample();


///////////////////////////////////////////////////////////////////////////////
// Patch Operations
///////////////////////////////////////////////////////////////////////////////

// array-end marker
#define synthesizer_patch_end \
    (_synthesizer_patch_operation) { \
        .type = end, \
        .release_fn = NULL, \
        .reset_data_fn = NULL, \
        .data = NULL \
    }

// sine generator
#define synthesizer_generator_sine(pitch_) \
    (_synthesizer_patch_operation) { \
        .type = nullary, \
        .nullary_fn = _synthesizer_generate_sine, \
        .release_fn = NULL, \
        .reset_data_fn = _synthesizer_generator_sine_reset_data, \
        .data = &(_synthesizer_generator_sine_data) { \
            .pitch = (pitch_), \
            .phase = 0.0f \
        } \
    }

typedef struct _synthesizer_generator_sine_data
{
    float pitch;
    float phase;
} _synthesizer_generator_sine_data;

float _synthesizer_generate_sine(void* data, float frequency);
void _synthesizer_generator_sine_reset_data(void* data);

// add operation
#define synthesizer_operator_add \
    (_synthesizer_patch_operation) { \
        .type = binary, \
        .binary_fn = _synthesizer_operate_add, \
        .release_fn = NULL, \
        .reset_data_fn = NULL, \
        .data = NULL \
    }

float _synthesizer_operate_add(void* data, float a, float b, float frequency);

// ADSR-envelope
#define synthesizer_asdr_envelope(attack_time_, \
        decay_time_, sustain_level_, release_time_) \
    (_synthesizer_patch_operation) { \
        .type = unary, \
        .unary_fn = _synthesizer_adsr_envelope, \
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

#endif /* SYNTHESIZER_H */

