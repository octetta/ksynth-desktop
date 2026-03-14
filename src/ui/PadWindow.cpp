#include "PadWindow.h"
#include "PadGrid.h"
#include "../theme.h"
#include <FL/Fl.H>
#include <FL/Fl_Button.H>

static const int PAD_WIN_W = 272;
static const int TOOLBAR_H = 28;
static const int GRID_SIZE = 260;
static const int PAD_WIN_H = TOOLBAR_H + 6 + GRID_SIZE + 4;

PadWindow::PadWindow()
    : Fl_Double_Window(PAD_WIN_W, PAD_WIN_H, "pads"), melodic_mode_(false)
{
    btn_drum_ = new Fl_Button(4,  2, 64, TOOLBAR_H, "drum");
    btn_drum_->box(FL_FLAT_BOX);
    btn_drum_->labelfont(FL_COURIER_BOLD); btn_drum_->labelsize(11);
    btn_drum_->callback(btn_mode_cb, this);

    btn_mel_  = new Fl_Button(72, 2, 76, TOOLBAR_H, "melodic");
    btn_mel_->box(FL_FLAT_BOX);
    btn_mel_->labelfont(FL_COURIER); btn_mel_->labelsize(11);
    btn_mel_->callback(btn_mode_cb, this);

    grid_ = new PadGrid(4, TOOLBAR_H+6, GRID_SIZE, GRID_SIZE);

    end();
    set_non_modal();
    resizable(grid_);
}

void PadWindow::draw() {
    /* Colours from T */
    color(T.bg);
    btn_drum_->color(melodic_mode_ ? T.btn    : T.accent);
    btn_drum_->labelcolor(melodic_mode_ ? T.muted : T.bg);
    btn_mel_->color(melodic_mode_  ? T.accent : T.btn);
    btn_mel_->labelcolor(melodic_mode_  ? T.bg    : T.muted);
    Fl_Double_Window::draw();
}

void PadWindow::toggle() { if(shown()) hide(); else show(); }
int  PadWindow::handle_key(int key) { return grid_->handle_key(key); }

int PadWindow::handle(int event) {
    if (event == FL_KEYDOWN)
        if (grid_->handle_key(Fl::event_key())) return 1;
    return Fl_Double_Window::handle(event);
}

void PadWindow::btn_mode_cb(Fl_Widget *w, void *ud) {
    PadWindow *pw = (PadWindow*)ud;
    pw->melodic_mode_ = (w == pw->btn_mel_);
    pw->apply_mode();
    pw->redraw();
}

void PadWindow::apply_mode() {
    if (melodic_mode_) {
        static const int st[16]={-7,-6,-5,-4,-3,-2,-1,0,1,2,3,4,5,6,7,8};
        for(int i=0;i<PadGrid::NUM_PADS;i++) grid_->set_pad(i,0,st[i]);
    } else {
        for(int i=0;i<PadGrid::NUM_PADS;i++) grid_->set_pad(i,i,0);
    }
}
