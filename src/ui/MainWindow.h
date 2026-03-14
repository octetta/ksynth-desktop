#pragma once
#include <FL/Fl_Double_Window.H>
#include "Splitter.h"

class SlotStrip;
class Editor;
class Notebook;
class PadWindow;

class MainWindow : public Fl_Double_Window {
public:
    MainWindow(int w, int h, const char *title);
    int  handle(int event) override;
    bool route_key_to_pads(int key);  /* called from global event handler */

private:
    SlotStrip *strip_;
    Editor    *editor_;
    Notebook  *notebook_;
    PadWindow *pad_win_;
    Splitter  *tile_;

    int ctx_slot_ = -1;
    int ctx_pad_  = -1;

    void setup_callbacks();

    void on_run(const char *code);
    void on_bank(int slot, const float *samples, int len, const char *code);
    void on_slot_play(int slot_idx);
    void on_slot_menu(int slot_idx, int x, int y);
    void on_pad_play(int pad_idx);
    void on_pad_menu(int pad_idx, int x, int y);
    void on_copy_to_editor(const char *code);

    void show_slot_menu(int x, int y);   /* uses ctx_slot_ */
    void show_pad_menu(int x, int y);    /* uses ctx_pad_  */
    void load_ks_file(const char *path);
    void save_session();
    void load_session();

    static void menu_open_cb(Fl_Widget*, void*);
    static void menu_quit_cb(Fl_Widget*, void*);
    static void menu_save_session_cb(Fl_Widget*, void*);
    static void menu_load_session_cb(Fl_Widget*, void*);
    static void btn_pads_cb(Fl_Widget*, void*);
};
