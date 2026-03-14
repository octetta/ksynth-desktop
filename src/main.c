/*
 * ksynth-desktop — Phase 1 headless CLI test
 *
 * Usage:  ksplay <file.ks> [slot_index]
 *
 * Evaluates the patch, banks it to slot 0 (or given index),
 * plays it, then waits for Enter so you can hear it.
 * Run with no args to see usage.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "audio.h"

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return NULL; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *buf = malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

/* Extract filename stem from a path */
static void stem(const char *path, char *out, int outlen) {
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
#ifdef _WIN32
    const char *bs = strrchr(base, '\\');
    if (bs) base = bs + 1;
#endif
    strncpy(out, base, outlen - 1);
    out[outlen - 1] = '\0';
    char *dot = strrchr(out, '.');
    if (dot) *dot = '\0';
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <file.ks> [slot 0-%d]\n",
                argv[0], ENGINE_NUM_SLOTS - 1);
        return 1;
    }

    const char *path = argv[1];
    int slot = argc >= 3 ? atoi(argv[2]) : 0;
    if (slot < 0 || slot >= ENGINE_NUM_SLOTS) {
        fprintf(stderr, "slot must be 0-%d\n", ENGINE_NUM_SLOTS - 1);
        return 1;
    }

    char *code = read_file(path);
    if (!code) return 1;

    engine_init();

    char label[64];
    stem(path, label, sizeof(label));

    printf("evaluating %s → slot %d ...\n", path, slot);
    EvalResult r = engine_eval(code, slot, label);
    free(code);

    if (!r.success) {
        fprintf(stderr, "eval error: %s\n", r.error);
        return 1;
    }
    printf("ok: %d samples (%.1f ms)\n",
           r.len, r.len * 1000.0 / ENGINE_SR);

    if (audio_init() != 0) return 1;

    printf("playing slot %d ('%s') — press Enter to replay, q+Enter to quit\n",
           slot, label);

    audio_play_slot(slot, 0);

    char line[16];
    while (fgets(line, sizeof(line), stdin)) {
        if (line[0] == 'q') break;
        audio_play_slot(slot, 0);
    }

    audio_shutdown();
    engine_clear_slot(slot);
    return 0;
}
