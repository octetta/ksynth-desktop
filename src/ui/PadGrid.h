#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>

typedef void (*PadPlayCB)(int pad_idx, void *userdata);
typedef void (*PadMenuCB)(int pad_idx, int x, int y, void *userdata);

class PadGrid : public Fl_Group {
public:
    static const int NUM_PADS = 16;

    PadGrid(int x, int y, int w, int h);

    /* slot[pad_idx] mapping and semitone offset */
    void draw() override;
    void set_pad(int pad_idx, int slot, int semitones);
    int  pad_slot(int pad_idx) const;
    int  pad_semitones(int pad_idx) const;

    /* Flash a pad (e.g. on keyboard trigger) */
    void flash_pad(int pad_idx);

    void on_play(PadPlayCB cb, void *ud) { play_cb_ = cb; play_ud_ = ud; }
    void on_menu(PadMenuCB cb, void *ud) { menu_cb_ = cb; menu_ud_ = ud; }

    /* Handle keyboard shortcuts even when focus is elsewhere.
       Call from main window's handle(). Returns 1 if consumed. */
    int handle_key(int key);

    /* called by PadButton — must be public since friend was removed */
    void fire_play(int idx);
    void fire_menu(int idx, int x, int y);

private:
    Fl_Box     *pads_[NUM_PADS];
    int         slot_map_[NUM_PADS];
    int         semitones_[NUM_PADS];
    PadPlayCB   play_cb_ = nullptr;
    void       *play_ud_ = nullptr;
    PadMenuCB   menu_cb_ = nullptr;
    void       *menu_ud_ = nullptr;
};
