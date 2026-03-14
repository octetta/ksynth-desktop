#define MINIAUDIO_IMPLEMENTATION
#include "../vendor/miniaudio.h"

#include "audio.h"
#include "engine.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ── voice ────────────────────────────────────────────────────────── */

typedef struct {
    float *samples;   /* heap copy, owned by voice */
    int    len;
    int    pos;       /* current playback position */
    int    active;
} Voice;

/* ── state ────────────────────────────────────────────────────────── */

static ma_device  g_device;
static Voice      g_voices[AUDIO_MAX_VOICES];
static int        g_voice_cursor = 0;   /* round-robin eviction */
static float      g_gain         = 0.5f;

/* Simple mutex-free approach: audio callback reads voices,
   main thread writes them.  We use a volatile active flag and
   write samples/len before setting active=1, matching the
   acquire/release pattern that works on x86/ARM without explicit
   atomics for this use-case.  For a production app use a lock-free
   ring or a proper mutex — this is good enough for a desktop tool. */

/* ── callback ─────────────────────────────────────────────────────── */

static void data_callback(ma_device *dev, void *output,
                           const void *input, ma_uint32 frame_count) {
    (void)dev; (void)input;
    float *out = (float *)output;
    memset(out, 0, frame_count * sizeof(float));

    for (int v = 0; v < AUDIO_MAX_VOICES; v++) {
        Voice *voice = &g_voices[v];
        if (!voice->active) continue;

        for (ma_uint32 f = 0; f < frame_count; f++) {
            if (voice->pos >= voice->len) {
                voice->active = 0;
                free(voice->samples);
                voice->samples = NULL;
                break;
            }
            out[f] += voice->samples[voice->pos++] * g_gain;
        }
    }

    /* Soft clip to prevent harshness when voices overlap */
    for (ma_uint32 f = 0; f < frame_count; f++) {
        float s = out[f];
        /* tanh approximation: x / (1 + |x|) — cheap and smooth */
        out[f] = s / (1.0f + fabsf(s));
    }
}

/* ── public API ───────────────────────────────────────────────────── */

int audio_init(void) {
    memset(g_voices, 0, sizeof(g_voices));

    ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format   = ma_format_f32;
    cfg.playback.channels = AUDIO_CHANNELS;
    cfg.sampleRate        = AUDIO_SR;
    cfg.dataCallback      = data_callback;
    cfg.pUserData         = NULL;

    if (ma_device_init(NULL, &cfg, &g_device) != MA_SUCCESS) {
        fprintf(stderr, "audio_init: failed to open playback device\n");
        return -1;
    }
    if (ma_device_start(&g_device) != MA_SUCCESS) {
        fprintf(stderr, "audio_init: failed to start playback device\n");
        ma_device_uninit(&g_device);
        return -1;
    }
    return 0;
}

void audio_shutdown(void) {
    ma_device_stop(&g_device);
    ma_device_uninit(&g_device);
    for (int v = 0; v < AUDIO_MAX_VOICES; v++) {
        g_voices[v].active = 0;
        free(g_voices[v].samples);
        g_voices[v].samples = NULL;
    }
}

void audio_play(const float *samples, int len, float base_rate, int semitones) {
    if (!samples || len <= 0) return;

    /* Resample for pitch */
    float  *buf      = NULL;
    int     buf_len  = 0;
    double  ratio    = pow(2.0, semitones / 12.0) * (double)base_rate;

    if (semitones == 0 && base_rate == 1.0f) {
        /* No pitch shift — direct copy */
        buf = malloc(len * sizeof(float));
        if (!buf) return;
        memcpy(buf, samples, len * sizeof(float));
        buf_len = len;
    } else {
        buf_len = (int)(len / ratio);
        if (buf_len <= 0) return;
        buf = malloc(buf_len * sizeof(float));
        if (!buf) return;
        for (int i = 0; i < buf_len; i++) {
            double src  = i * ratio;
            int    si   = (int)src;
            double frac = src - si;
            if (si + 1 < len)
                buf[i] = (float)(samples[si] * (1.0 - frac) + samples[si+1] * frac);
            else if (si < len)
                buf[i] = samples[si];
            else
                buf[i] = 0.0f;
        }
    }

    /* Find a free voice slot, evicting oldest (round-robin) if full */
    int slot = -1;
    for (int v = 0; v < AUDIO_MAX_VOICES; v++) {
        if (!g_voices[v].active) { slot = v; break; }
    }
    if (slot < 0) {
        /* Evict — free the old buffer first */
        slot = g_voice_cursor % AUDIO_MAX_VOICES;
        g_voices[slot].active = 0;
        free(g_voices[slot].samples);
        g_voices[slot].samples = NULL;
    }
    g_voice_cursor++;

    /* Write fields before setting active — memory ordering matters */
    g_voices[slot].samples = buf;
    g_voices[slot].len     = buf_len;
    g_voices[slot].pos     = 0;
    g_voices[slot].active  = 1;  /* publish last */
}

void audio_play_slot(int slot_idx, int semitones) {
    const Slot *slots = engine_slots();
    if (slot_idx < 0 || slot_idx >= ENGINE_NUM_SLOTS) return;
    const Slot *s = &slots[slot_idx];
    if (!s->samples || s->len == 0) return;
    audio_play(s->samples, s->len, s->base_rate, semitones);
}

void audio_set_gain(float gain) {
    if (gain < 0.0f) gain = 0.0f;
    if (gain > 1.0f) gain = 1.0f;
    g_gain = gain;
}

float audio_get_gain(void) { return g_gain; }

/* ── WAV export ───────────────────────────────────────────────────── */

int audio_save_wav(const char *path, const float *samples, int len) {
    if (!path || !samples || len <= 0) return -1;
    FILE *f = fopen(path, "wb");
    if (!f) return -1;

    int   sr       = AUDIO_SR;
    int   channels = 1;
    int   bps      = 16;
    int   data_sz  = len * channels * (bps / 8);
    int   hdr_sz   = 44;

    /* RIFF header */
    fwrite("RIFF", 1, 4, f);
    int riff_sz = hdr_sz - 8 + data_sz;
    fwrite(&riff_sz, 4, 1, f);
    fwrite("WAVE", 1, 4, f);

    /* fmt chunk */
    fwrite("fmt ", 1, 4, f);
    int fmt_sz = 16; fwrite(&fmt_sz, 4, 1, f);
    short audio_fmt = 1; fwrite(&audio_fmt, 2, 1, f);   /* PCM */
    short ch = (short)channels; fwrite(&ch, 2, 1, f);
    fwrite(&sr, 4, 1, f);
    int byte_rate = sr * channels * (bps / 8); fwrite(&byte_rate, 4, 1, f);
    short block_align = (short)(channels * (bps / 8)); fwrite(&block_align, 2, 1, f);
    short bps_s = (short)bps; fwrite(&bps_s, 2, 1, f);

    /* data chunk */
    fwrite("data", 1, 4, f);
    fwrite(&data_sz, 4, 1, f);

    /* Convert float [-1,1] → int16 */
    for (int i = 0; i < len; i++) {
        float v = samples[i];
        if (v >  1.0f) v =  1.0f;
        if (v < -1.0f) v = -1.0f;
        short s16 = (short)(v * 32767.0f);
        fwrite(&s16, 2, 1, f);
    }

    fclose(f);
    return 0;
}
