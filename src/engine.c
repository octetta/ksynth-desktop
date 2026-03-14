#include "engine.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* Pull in ksynth interpreter — same pattern as ks_api.c / kstest.c */
#include "../vendor/ksynth.c"

static Slot g_slots[ENGINE_NUM_SLOTS];

void engine_init(void) {
    memset(g_slots, 0, sizeof(g_slots));
    for (int i = 0; i < ENGINE_NUM_SLOTS; i++)
        g_slots[i].base_rate = 1.0f;
}

void engine_clear_slot(int idx) {
    if (idx < 0 || idx >= ENGINE_NUM_SLOTS) return;
    free(g_slots[idx].samples);
    g_slots[idx].samples = NULL;
    g_slots[idx].len     = 0;
    g_slots[idx].label[0] = '\0';
}

void engine_set_slot_base_rate(int idx, float rate) {
    if (idx < 0 || idx >= ENGINE_NUM_SLOTS) return;
    if (rate < 0.001f) rate = 0.001f;
    if (rate > 64.0f)  rate = 64.0f;
    g_slots[idx].base_rate = rate;
}

EvalResult engine_eval(const char *code, int slot_idx, const char *label) {
    EvalResult res = {0};

    if (!code || !*code) {
        snprintf(res.error, sizeof(res.error), "empty code");
        return res;
    }

    /* ksynth needs a mutable buffer */
    char *buf = strdup(code);
    if (!buf) {
        snprintf(res.error, sizeof(res.error), "out of memory");
        return res;
    }

    /* Reset interpreter state — free any K objects from previous eval */
    for (int i = 0; i < 26; i++) {
        if (vars[i]) { k_free(vars[i]); vars[i] = NULL; }
    }

    /* Run the script line by line — W: w expr stores result in vars[22] */
    char *p = buf;
    while (*p) {
        while (*p == '\n' || *p == '\r') p++;
        if (!*p) break;
        K line_result = e(&p);
        if (line_result) k_free(line_result);
    }
    free(buf);

    /* Retrieve vars['W'-'A'] */
    K result = vars['W' - 'A'];
    if (!result || result->n <= 0) {
        snprintf(res.error, sizeof(res.error),
                 "no W output (did you end with W: w ...)");
        return res;
    }

    float *samples = malloc((size_t)result->n * sizeof(float));
    if (!samples) {
        snprintf(res.error, sizeof(res.error), "out of memory");
        return res;
    }
    for (int i = 0; i < result->n; i++) {
        double v = result->f[i];
        if (v >  2.0) v =  2.0;
        if (v < -2.0) v = -2.0;
        samples[i] = (float)v;
    }

    /* Store in slot */
    engine_clear_slot(slot_idx);
    g_slots[slot_idx].samples   = samples;
    g_slots[slot_idx].len       = result->n;
    if (label && *label) {
        strncpy(g_slots[slot_idx].label, label, sizeof(g_slots[slot_idx].label) - 1);
    } else {
        snprintf(g_slots[slot_idx].label, sizeof(g_slots[slot_idx].label),
                 "slot%d", slot_idx);
    }

    res.success = 1;
    res.samples = samples;
    res.len     = result->n;
    return res;
}

const Slot *engine_slots(void) {
    return g_slots;
}

/* Simple linear interpolation resampler for pitch shifting */
float *engine_slot_pitched(int idx, int semitones, int *out_len) {
    if (idx < 0 || idx >= ENGINE_NUM_SLOTS) return NULL;
    const Slot *s = &g_slots[idx];
    if (!s->samples || s->len == 0) return NULL;

    double ratio = pow(2.0, semitones / 12.0) * s->base_rate;
    int new_len = (int)(s->len / ratio);
    if (new_len <= 0) return NULL;

    float *out = malloc(new_len * sizeof(float));
    if (!out) return NULL;

    for (int i = 0; i < new_len; i++) {
        double src_pos = i * ratio;
        int    src_i   = (int)src_pos;
        double frac    = src_pos - src_i;
        if (src_i + 1 < s->len) {
            out[i] = (float)(s->samples[src_i] * (1.0 - frac) +
                             s->samples[src_i + 1] * frac);
        } else if (src_i < s->len) {
            out[i] = s->samples[src_i];
        } else {
            out[i] = 0.0f;
        }
    }

    *out_len = new_len;
    return out;
}

/* Evaluate without storing in any slot — returns a heap-allocated samples
   buffer that the CALLER must free(). res.samples must be freed by caller. */
EvalResult engine_eval_scratch(const char *code) {
    EvalResult res = {0};

    if (!code || !*code) {
        snprintf(res.error, sizeof(res.error), "empty code");
        return res;
    }

    char *buf = strdup(code);
    if (!buf) { snprintf(res.error, sizeof(res.error), "out of memory"); return res; }

    for (int i = 0; i < 26; i++) {
        if (vars[i]) { k_free(vars[i]); vars[i] = NULL; }
    }

    char *p = buf;
    while (*p) {
        while (*p == '\n' || *p == '\r') p++;
        if (!*p) break;
        K lr = e(&p);
        if (lr) k_free(lr);
    }
    free(buf);

    K result = vars['W' - 'A'];
    if (!result || result->n <= 0) {
        snprintf(res.error, sizeof(res.error),
                 "no W output (did you end with W: w ...)");
        return res;
    }

    float *samples = malloc((size_t)result->n * sizeof(float));
    if (!samples) { snprintf(res.error, sizeof(res.error), "out of memory"); return res; }
    for (int i = 0; i < result->n; i++) {
        double v = result->f[i];
        /* clamp to prevent downstream FPE in audio path */
        if (v >  2.0) v =  2.0;
        if (v < -2.0) v = -2.0;
        samples[i] = (float)v;
    }

    res.success = 1;
    res.samples = samples;
    res.len     = result->n;
    return res;
}

/* Store a pre-computed sample buffer into a slot (from notebook bank button) */
void engine_bank_slot(int slot_idx, const float *samples, int len,
                      const char *label) {
    if (slot_idx < 0 || slot_idx >= ENGINE_NUM_SLOTS) return;
    engine_clear_slot(slot_idx);

    float *copy = malloc(len * sizeof(float));
    if (!copy) return;
    memcpy(copy, samples, len * sizeof(float));

    g_slots[slot_idx].samples   = copy;
    g_slots[slot_idx].len       = len;
    g_slots[slot_idx].base_rate = 1.0f;

    /* derive label from first non-comment line of code */
    if (label && *label) {
        const char *p = label;
        while (*p == '/' || *p == ' ' || *p == '\t') {
            /* skip comment lines */
            while (*p && *p != '\n') p++;
            while (*p == '\n' || *p == '\r') p++;
        }
        /* take up to 20 chars of first real line */
        char tmp[21]; int n = 0;
        while (*p && *p != '\n' && n < 20) tmp[n++] = *p++;
        tmp[n] = '\0';
        snprintf(g_slots[slot_idx].label, sizeof(g_slots[slot_idx].label),
                 "%s", n > 0 ? tmp : "banked");
    } else {
        snprintf(g_slots[slot_idx].label, sizeof(g_slots[slot_idx].label),
                 "slot%d", slot_idx);
    }
}
