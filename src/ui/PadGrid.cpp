#include "PadGrid.h"
#include "../engine.h"
#include "../theme.h"
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <cstdio>

static const char PAD_KEYS[PadGrid::NUM_PADS] = {
    '1','2','3','4','5','6','7','8',
    '9','0','a','b','c','d','e','f'
};

class PadButton : public Fl_Box {
public:
    PadButton(int x,int y,int w,int h,int idx,PadGrid *grid)
        : Fl_Box(x,y,w,h), idx_(idx), grid_(grid), lit_(false) {}

    void draw() override {
        const Slot *slots = engine_slots();
        int  si    = grid_->pad_slot(idx_);
        bool filled = si >= 0 && si < ENGINE_NUM_SLOTS && slots[si].samples;
        int  st    = grid_->pad_semitones(idx_);

        /* background */
        fl_color(lit_ ? T.accent : (filled ? T.pad_fill : T.pad));
        fl_rectf(x(), y(), w(), h());
        fl_color(T.border);
        fl_rect(x(), y(), w(), h());

        fl_push_clip(x()+1, y()+1, w()-2, h()-2);

        /* Slot hex — top centre, large */
        fl_font(FL_COURIER_BOLD, 16);
        fl_color(lit_ ? T.bg : (filled ? T.accent : T.muted));
        char sb[4]; snprintf(sb, sizeof(sb), "%X", si);
        fl_draw(sb, x(), y()+2, w(), h()/2, FL_ALIGN_CENTER);

        /* Key hint — top left, small */
        fl_font(FL_COURIER, 9);
        fl_color(T.muted);
        char kb[4];
        if (idx_ < 9)       snprintf(kb, sizeof(kb), "%d", idx_+1);
        else if (idx_ == 9) snprintf(kb, sizeof(kb), "0");
        else                snprintf(kb, sizeof(kb), "%c", 'a' + idx_ - 10);
        fl_draw(kb, x()+3, y()+3, w()/3, 12, FL_ALIGN_LEFT);

        if (filled) {
            /* Slot label — centre bottom half */
            fl_font(FL_COURIER, 9);
            fl_color(T.text);
            const char *lbl = slots[si].label;
            fl_draw(lbl, x()+2, y()+h()/2, w()-4, h()/2-14, FL_ALIGN_CENTER);

            /* Semitone — bottom right, "+Nst" format */
            fl_font(FL_COURIER, 8);
            fl_color(st != 0 ? T.accent : T.muted);
            char stb[12]; snprintf(stb, sizeof(stb), "%+dst", st);
            fl_draw(stb, x(), y()+h()-13, w()-3, 12, FL_ALIGN_RIGHT);
        } else {
            /* Empty dash — centre */
            fl_font(FL_COURIER, 14);
            fl_color(T.muted);
            fl_draw("—", x(), y()+h()/2, w(), h()/2, FL_ALIGN_CENTER);
        }

        fl_pop_clip();
    }

    int handle(int event) override {
        switch (event) {
        case FL_PUSH:
            if (Fl::event_button() == FL_RIGHT_MOUSE) {
                grid_->fire_menu(idx_, Fl::event_x_root(), Fl::event_y_root());
                return 1;
            }
            light(true); grid_->fire_play(idx_); return 1;
        case FL_RELEASE: light(false); return 1;
        case FL_MOUSEWHEEL: {
            /* scroll up = higher pitch, scroll down = lower */
            int st = grid_->pad_semitones(idx_) - Fl::event_dy();
            if (st < -24) st = -24;
            if (st >  24) st =  24;
            grid_->set_pad(idx_, grid_->pad_slot(idx_), st);
            return 1;
        }
        case FL_ENTER: case FL_LEAVE: return 1;
        }
        return Fl_Box::handle(event);
    }

    void light(bool on) { if (lit_==on) return; lit_=on; redraw(); }
    void flash() {
        light(true);
        Fl::add_timeout(0.1, [](void*v){ ((PadButton*)v)->light(false); }, this);
    }

private:
    int      idx_;
    PadGrid *grid_;
    bool     lit_;
};

PadGrid::PadGrid(int x,int y,int w,int h) : Fl_Group(x,y,w,h) {
    for (int i=0; i<NUM_PADS; i++) { slot_map_[i]=i; semitones_[i]=0; }
    const int cols=4, rows=4, gap=3;
    int cw = (w - gap*(cols+1)) / cols;
    int ch = (h - gap*(rows+1)) / rows;
    for (int i=0; i<NUM_PADS; i++) {
        int col=i%cols, row=i/cols;
        pads_[i] = new PadButton(
            x + gap + col*(cw+gap),
            y + gap + row*(ch+gap),
            cw, ch, i, this);
        add(pads_[i]);
    }
    end();
}

void PadGrid::draw() {
    fl_color(T.bg); fl_rectf(x(),y(),w(),h());
    draw_children();
}

void PadGrid::set_pad(int idx, int slot, int st) {
    if (idx<0||idx>=NUM_PADS) return;
    slot_map_[idx]=slot; semitones_[idx]=st;
    pads_[idx]->redraw();
}
int PadGrid::pad_slot(int idx)      const { return (idx>=0&&idx<NUM_PADS)?slot_map_[idx]:0; }
int PadGrid::pad_semitones(int idx) const { return (idx>=0&&idx<NUM_PADS)?semitones_[idx]:0; }
void PadGrid::flash_pad(int idx) {
    if (idx>=0&&idx<NUM_PADS) static_cast<PadButton*>(pads_[idx])->flash();
}
int PadGrid::handle_key(int key) {
    for (int i=0; i<NUM_PADS; i++)
        if (key==PAD_KEYS[i]) { static_cast<PadButton*>(pads_[i])->flash(); fire_play(i); return 1; }
    return 0;
}
void PadGrid::fire_play(int idx)             { if (play_cb_) play_cb_(idx, play_ud_); }
void PadGrid::fire_menu(int idx, int x,int y){ if (menu_cb_) menu_cb_(idx,x,y,menu_ud_); }
