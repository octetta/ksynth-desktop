#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

#define AUDIO_SR          44100
#define AUDIO_CHANNELS    1
#define AUDIO_MAX_VOICES  16   /* max simultaneous playing voices */

/* Must be called once. Returns 0 on success, -1 on error. */
int  audio_init(void);
void audio_shutdown(void);

/* Trigger playback of a sample buffer.
   audio_play() makes a copy — caller retains ownership of samples.
   semitones: pitch shift (-24..+24), 0 = no shift.
   If MAX_VOICES are already playing, oldest voice is evicted. */
void audio_play(const float *samples, int len, float base_rate, int semitones);

/* Convenience: play directly from engine slot */
void audio_play_slot(int slot_idx, int semitones);

/* Master gain 0.0–1.0 */
void  audio_set_gain(float gain);
float audio_get_gain(void);

/* Export samples to a 16-bit mono WAV file. Returns 0 on success. */
int audio_save_wav(const char *path, const float *samples, int len);

#ifdef __cplusplus
}
#endif
