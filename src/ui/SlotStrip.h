#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>

/* Callback types */
typedef void (*SlotPlayCB)(int slot_idx, void *userdata);
typedef void (*SlotMenuCB)(int slot_idx, int x, int y, void *userdata);

class SlotStrip : public Fl_Group {
public:
    SlotStrip(int x, int y, int w, int h);

    /* Refresh a single card after slot state changes */
    void refresh_slot(int idx);

    /* Refresh all cards */
    void refresh_all();

    void on_play(SlotPlayCB cb, void *ud) { play_cb_ = cb; play_ud_ = ud; }
    void on_menu(SlotMenuCB cb, void *ud) { menu_cb_ = cb; menu_ud_ = ud; }

    void draw() override;

    /* called by SlotCard — must be public since friend was removed */
    void fire_play(int idx);
    void fire_menu(int idx, int x, int y);

private:
    static const int NUM_SLOTS = 16;
    Fl_Box          *cards_[NUM_SLOTS];
    SlotPlayCB       play_cb_ = nullptr;
    void            *play_ud_ = nullptr;
    SlotMenuCB       menu_cb_ = nullptr;
    void            *menu_ud_ = nullptr;
};
