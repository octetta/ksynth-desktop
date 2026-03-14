#include "MainWindow.h"
#include "SlotStrip.h"
#include "Editor.h"
#include "Notebook.h"
#include "PadWindow.h"
#include "PadGrid.h"
#include "../engine.h"
#include "../theme.h"
#include "../audio.h"
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Tile.H>
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>

static const int MENUBAR_H = 22;
static const int STRIP_H   = 68;

MainWindow::MainWindow(int w, int h, const char *title)
    : Fl_Double_Window(w, h, title)
{
    int my = 0;

    Fl_Menu_Bar *menu = new Fl_Menu_Bar(0, my, w, MENUBAR_H);
    menu->textfont(FL_COURIER); menu->textsize(11);
    menu->add("File/Open .ks...\\t^o", 0, menu_open_cb, this);
    menu->add("File/Quit\\t^q",        0, menu_quit_cb, this);
    my += MENUBAR_H;

    strip_ = new SlotStrip(0, my, w, STRIP_H);
    my += STRIP_H + 1;

    int main_h = h - my;
    int nb_w   = w / 2;

    /* Fl_Tile allows dragging the divider between notebook and editor */
    tile_ = new Fl_Tile(0, my, w, main_h);
    notebook_ = new Notebook(0,    my, nb_w,   main_h);
    editor_   = new Editor  (nb_w, my, w-nb_w, main_h);
    tile_->end();

    end();
    resizable(tile_);

    pad_win_ = new PadWindow();

    setup_callbacks();
    theme_set(0);
}

/* ── wiring ─────────────────────────────────────────────────────────── */

void MainWindow::setup_callbacks() {
    strip_->on_play([](int idx, void *ud){
        ((MainWindow*)ud)->on_slot_play(idx);
    }, this);
    strip_->on_menu([](int idx, int x, int y, void *ud){
        ((MainWindow*)ud)->on_slot_menu(idx, x, y);
    }, this);

    editor_->on_run([](const char *code, void *ud){
        ((MainWindow*)ud)->on_run(code);
    }, this);
    editor_->set_pads_cb(btn_pads_cb, this);
    editor_->set_theme_cb(btn_theme_cb, this);

    notebook_->on_copy_to([](const char *code, void *ud){
        ((MainWindow*)ud)->on_copy_to_editor(code);
    }, this);
    notebook_->on_play([](const float *s, int len, void *){
        audio_play(s, len, 1.0f, 0);
    }, this);
    notebook_->on_bank([](int slot, const float *s, int len,
                          const char *code, void *ud){
        ((MainWindow*)ud)->on_bank(slot, s, len, code);
    }, this);

    pad_win_->grid()->on_play([](int idx, void *ud){
        ((MainWindow*)ud)->on_pad_play(idx);
    }, this);
    pad_win_->grid()->on_menu([](int idx, int x, int y, void *ud){
        ((MainWindow*)ud)->on_pad_menu(idx, x, y);
    }, this);
}

/* ── handlers ───────────────────────────────────────────────────────── */

void MainWindow::on_run(const char *code) {
    EvalResult r = engine_eval_scratch(code);

    NotebookEntry entry;
    entry.success   = r.success;
    entry.code      = code;
    entry.error     = r.success ? "" : r.error;
    entry.samples   = r.success ? r.len : 0;
    entry.floatLen  = r.success ? r.len : 0;
    entry.floatData = nullptr;

    if (r.success && r.samples && r.len > 0) {
        entry.floatData = (float*)malloc((size_t)r.len * sizeof(float));
        if (entry.floatData)
            memcpy(entry.floatData, r.samples, (size_t)r.len * sizeof(float));
        free(r.samples);
    }

    notebook_->add_entry(entry);
}

void MainWindow::on_bank(int slot, const float *samples, int len,
                         const char *code) {
    engine_bank_slot(slot, samples, len, code);
    strip_->refresh_slot(slot);
}

void MainWindow::on_slot_play(int idx)  { audio_play_slot(idx, 0); }
void MainWindow::on_copy_to_editor(const char *code) { editor_->set_code(code); }

void MainWindow::on_slot_menu(int idx, int x, int y) {
    ctx_slot_ = idx; show_slot_menu(x, y);
}
void MainWindow::on_pad_play(int pad_idx) {
    audio_play_slot(pad_win_->grid()->pad_slot(pad_idx),
                    pad_win_->grid()->pad_semitones(pad_idx));
}
void MainWindow::on_pad_menu(int pad_idx, int x, int y) {
    ctx_pad_ = pad_idx; show_pad_menu(x, y);
}

void MainWindow::cycle_theme() {
    theme_ = (theme_ + 1) % 3;
    theme_set(theme_);
}

/* ── keyboard ───────────────────────────────────────────────────────── */

int MainWindow::handle(int event) {
    return Fl_Double_Window::handle(event);
}

bool MainWindow::route_key_to_pads(int key) {
    /* Don't steal keys when a text editor widget has focus —
       Fl_Text_Editor is the only widget in this app that accepts typed text,
       and it's always the child of editor_. Walk up from focus. */
    for (Fl_Widget *w = Fl::focus(); w; w = w->parent()) {
        if (w == editor_) return false;   /* editor_ or its children have focus */
        if (w == this) break;
    }
    return pad_win_->handle_key(key) != 0;
}

/* ── context menus ──────────────────────────────────────────────────── */

void MainWindow::show_slot_menu(int mx, int my) {
    int idx = ctx_slot_;
    const Slot *s = engine_slots() + idx;
    bool filled = s->samples != nullptr;

    int cur_st = filled ? (int)round(log2((double)s->base_rate) * 12.0) : 0;
    char tune_lbl[64];
    snprintf(tune_lbl, sizeof(tune_lbl), "Set tuning  (now %+dst)...", cur_st);

    /* Use Fl_Menu_Item array + static popup() — works without a parent window */
    const int N = 6;
    Fl_Menu_Item items[N] = {};

    int n = 0;
    items[n++] = { "Play",      0,0,0, filled ? 0 : FL_MENU_INACTIVE };
    items[n++] = { tune_lbl,    0,0,0, filled ? 0 : FL_MENU_INACTIVE };
    items[n++] = { "Save WAV...",0,0,0,(filled ? 0 : FL_MENU_INACTIVE)|FL_MENU_DIVIDER };
    items[n++] = { "Rename...", 0,0,0, 0 };
    items[n++] = { "Clear slot",0,0,0, filled ? 0 : FL_MENU_INACTIVE };
    items[n]   = { nullptr };   /* sentinel */

    /* Fl_Menu_Item::popup(items, x, y) is the correct free-standing popup call */
    const Fl_Menu_Item *picked = items[0].popup(mx, my);
    if (!picked || !picked->text) return;

    /* Dispatch by position — avoids string compare fragility with tune_lbl */
    int choice = (int)(picked - items);
    switch (choice) {
    case 0:   /* Play */
        on_slot_play(idx);
        break;
    case 1: { /* Set tuning */
        char buf[16]; snprintf(buf, sizeof(buf), "%d", cur_st);
        const char *res = fl_input("Tuning in semitones (e.g. +7, -12, 0):", buf);
        if (res) {
            engine_set_slot_base_rate(idx, (float)pow(2.0, atoi(res) / 12.0));
            strip_->refresh_slot(idx);
        }
        break;
    }
    case 2: { /* Save WAV */
        char def[80]; snprintf(def, sizeof(def), "%s.wav", s->label);
        const char *path = fl_file_chooser("Save WAV", "*.wav", def);
        if (path) {
            char out[512]; strncpy(out, path, sizeof(out)-5); out[sizeof(out)-5]='\0';
            if (!strstr(out,".wav") && !strstr(out,".WAV")) strcat(out,".wav");
            if (audio_save_wav(out, s->samples, s->len) != 0)
                fl_alert("Failed to write: %s", out);
        }
        break;
    }
    case 3: { /* Rename */
        const char *nn = fl_input("Rename slot:", s->label);
        if (nn) {
            strncpy(const_cast<char*>(s->label), nn, 63);
            strip_->refresh_slot(idx);
        }
        break;
    }
    case 4:   /* Clear slot */
        engine_clear_slot(idx);
        strip_->refresh_slot(idx);
        break;
    }
}

void MainWindow::show_pad_menu(int x, int y) {
    int idx = ctx_pad_;
    PadGrid *g = pad_win_->grid();
    int cur_slot = g->pad_slot(idx);
    int cur_st   = g->pad_semitones(idx);

    /* Slot selection */
    char slotbuf[8];
    snprintf(slotbuf, sizeof(slotbuf), "%d", cur_slot);
    const char *ss = fl_input("Assign pad to slot (0-15):", slotbuf);
    if (!ss) return;
    int ns = atoi(ss);
    if (ns < 0 || ns >= ENGINE_NUM_SLOTS) return;

    /* Semitone offset */
    char stbuf[8];
    snprintf(stbuf, sizeof(stbuf), "%d", cur_st);
    const char *ts = fl_input("Semitone offset (-24 to +24):", stbuf);
    if (!ts) return;
    int nst = atoi(ts);

    g->set_pad(idx, ns, nst);
}

/* ── file I/O ───────────────────────────────────────────────────────── */

void MainWindow::load_ks_file(const char *path) {
    if (!path) {
        path = fl_file_chooser("Load .ks patch", "*.ks", nullptr);
        if (!path) return;
    }
    FILE *f = fopen(path, "r");
    if (!f) { fl_alert("Cannot open: %s", path); return; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char *buf = (char*)malloc((size_t)sz + 1);
    fread(buf, 1, (size_t)sz, f); buf[sz] = '\0'; fclose(f);
    editor_->set_code(buf);
    free(buf);
}

/* ── static callbacks ────────────────────────────────────────────────── */
void MainWindow::btn_pads_cb(Fl_Widget*,void*ud)  { ((MainWindow*)ud)->pad_win_->toggle(); }
void MainWindow::btn_theme_cb(Fl_Widget*,void*ud) { ((MainWindow*)ud)->cycle_theme(); }
void MainWindow::menu_open_cb(Fl_Widget*,void*ud) { ((MainWindow*)ud)->load_ks_file(nullptr); }
void MainWindow::menu_quit_cb(Fl_Widget*,void*)   { exit(0); }
