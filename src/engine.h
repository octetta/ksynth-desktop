#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

#define ENGINE_NUM_SLOTS 16
#define ENGINE_SR        44100

/* A single loaded slot */
typedef struct {
    float   *samples;   /* heap-allocated PCM, NULL if empty */
    int      len;       /* sample count */
    float    base_rate; /* playback rate multiplier, 1.0 = normal */
    char     label[64]; /* display name, usually the patch filename stem */
} Slot;

/* Result returned from engine_eval() */
typedef struct {
    int    success;
    float *samples;   /* caller does NOT free — owned by slot or freed on error */
    int    len;
    char   error[256];
} EvalResult;

/* Must be called once before anything else */
void engine_init(void);

/* Evaluate ksynth code, store result in slot_idx.
   Returns EvalResult; on success samples pointer is the same as slots[slot_idx].samples */
EvalResult engine_eval(const char *code, int slot_idx, const char *label);

/* Clear a slot, freeing its samples */
void engine_clear_slot(int slot_idx);
void engine_set_slot_base_rate(int slot_idx, float base_rate);

/* Direct read-only access to slot array */
const Slot *engine_slots(void);

/* Return a semitone-shifted copy of slot samples.
   Caller must free() the returned buffer. Returns NULL on empty slot. */
float *engine_slot_pitched(int slot_idx, int semitones, int *out_len);

/* Evaluate without storing in any slot.
   res.samples is heap-allocated and must be free()'d by the caller. */
EvalResult engine_eval_scratch(const char *code);

/* Store a pre-computed PCM buffer directly into a slot */
void engine_bank_slot(int slot_idx, const float *samples, int len,
                      const char *label);

#ifdef __cplusplus
}
#endif
