#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* One notebook history entry passed to/from session save/load */
typedef struct {
    const char    *code;       /* ksynth source (read-only on save, caller owns on load) */
    int            success;
    int            samples;    /* sample count */
    const char    *error;      /* error string or "" */
    const float   *audio;      /* PCM float samples, NULL if none */
    int            audio_len;  /* sample count of audio */
} SessionEntry;

/* Save session.
   editor_text   : current editor contents (may be NULL)
   history       : array of SessionEntry, oldest first
   history_count : number of entries
   pad_slots/pad_semitones/num_pads : pad config */
int session_save(const char       *path,
                 const char       *editor_text,
                 const SessionEntry *history,
                 int               history_count,
                 const int        *pad_slots,
                 const int        *pad_semitones,
                 int               num_pads);

/* Load result — caller must call session_load_free() when done */
typedef struct {
    char          *editor_text;      /* heap, may be NULL */
    SessionEntry  *history;          /* heap array, oldest first */
    int            history_count;
    int            pad_slots[16];
    int            pad_semitones[16];
} SessionLoadResult;

/* Load session file. Returns 0 on success.
   On success *out is populated — caller must call session_load_free(out). */
int session_load(const char       *path,
                 SessionLoadResult *out,
                 char             *errmsg,
                 int               errmsg_len);

/* Free all heap memory in a SessionLoadResult */
void session_load_free(SessionLoadResult *r);

#ifdef __cplusplus
}
#endif
