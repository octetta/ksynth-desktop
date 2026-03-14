#include <FL/Fl.H>
#include "ui/MainWindow.h"
#include "engine.h"
#include "audio.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

static MainWindow *g_win = nullptr;

static int global_event_handler(int event) {
    if (event == FL_KEYDOWN && g_win)
        if (g_win->route_key_to_pads(Fl::event_key())) return 1;
    return 0;
}

int main(int argc, char **argv) {
    Fl::scheme("gtk+");

    engine_init();

    if (audio_init() != 0) {
        fprintf(stderr, "Failed to initialise audio device\n");
        return 1;
    }

    MainWindow win(1100, 680, "ksynth-desktop " KSYNTH_DESKTOP_VERSION);
    g_win = &win;
    win.show(argc, argv);
    Fl::add_handler(global_event_handler);

    for (int i = 1; i < argc && i <= 16; i++) {
        const char *path = argv[i];
        size_t len = strlen(path);
        if (len > 3 && strcmp(path + len - 3, ".ks") == 0) {
            FILE *f = fopen(path, "r");
            if (!f) { perror(path); continue; }
            fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
            char *buf = (char*)malloc((size_t)sz + 1);
            fread(buf, 1, (size_t)sz, f); buf[sz] = '\0'; fclose(f);

            const char *base = strrchr(path, '/');
            base = base ? base+1 : path;
            char label[64]; strncpy(label, base, 63); label[63] = '\0';
            char *dot = strrchr(label, '.'); if (dot) *dot = '\0';

            EvalResult r = engine_eval(buf, i-1, label);
            free(buf);
            if (r.success) audio_play_slot(i-1, 0);
            else fprintf(stderr, "slot %d: %s\n", i-1, r.error);
        }
    }

    int ret = Fl::run();
    audio_shutdown();
    return ret;
}
