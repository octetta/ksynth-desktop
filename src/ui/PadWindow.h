#pragma once
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>

class PadGrid;

class PadWindow : public Fl_Double_Window {
public:
    PadWindow();

    void draw() override;
    int  handle(int event) override;
    void toggle();
    int  handle_key(int key);
    PadGrid *grid() { return grid_; }

private:
    PadGrid    *grid_;
    Fl_Button  *btn_drum_;
    Fl_Button  *btn_mel_;
    bool        melodic_mode_;

    static void btn_mode_cb(Fl_Widget*, void*);
    void apply_mode();
};
