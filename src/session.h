#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Save current engine slots + pad config to a JSON file.
   pad_slots[16] and pad_semitones[16] describe pad assignments.
   Returns 0 on success, -1 on error. */
int session_save(const char *path,
                 const int *pad_slots,
                 const int *pad_semitones,
                 int        num_pads);

/* Load a session JSON file into engine slots + pad arrays.
   pad_slots/pad_semitones must be arrays of at least num_pads ints.
   Returns 0 on success, -1 on error (errmsg filled). */
int session_load(const char *path,
                 int *pad_slots,
                 int *pad_semitones,
                 int  num_pads,
                 char *errmsg,
                 int   errmsg_len);

#ifdef __cplusplus
}
#endif
