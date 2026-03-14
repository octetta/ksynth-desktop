#include "Splitter.h"
#include <FL/fl_draw.H>
#include <FL/Fl.H>

Splitter::Splitter(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h)
{
    box(FL_FLAT_BOX);
    div_x_ = x + w / 2;
}

bool Splitter::over_divider(int ex) const {
    return ex >= div_x_ - DIV_W && ex <= div_x_ + DIV_W;
}

void Splitter::apply_split(int new_div_x) {
    /* Clamp so each pane has at least 80px */
    int min_x = x() + 80;
    int max_x = x() + w() - 80;
    if (new_div_x < min_x) new_div_x = min_x;
    if (new_div_x > max_x) new_div_x = max_x;
    if (new_div_x == div_x_) return;

    div_x_ = new_div_x;

    if (children() >= 2) {
        Fl_Widget *left  = child(0);
        Fl_Widget *right = child(1);
        int lw = div_x_ - x();
        int rw = x() + w() - div_x_;
        left ->resize(x(),      y(), lw, h());
        right->resize(div_x_,   y(), rw, h());
    }

    redraw();
}

int Splitter::handle(int event) {
    switch (event) {
    case FL_ENTER:
    case FL_MOVE:
        if (over_divider(Fl::event_x()))
            fl_cursor(FL_CURSOR_WE);
        else
            fl_cursor(FL_CURSOR_DEFAULT);
        return 1;

    case FL_LEAVE:
        fl_cursor(FL_CURSOR_DEFAULT);
        return 1;

    case FL_PUSH:
        if (Fl::event_button() == FL_LEFT_MOUSE &&
            over_divider(Fl::event_x())) {
            dragging_ = true;
            drag_off_ = Fl::event_x() - div_x_;
            fl_cursor(FL_CURSOR_WE);
            return 1;
        }
        break;

    case FL_DRAG:
        if (dragging_) {
            apply_split(Fl::event_x() - drag_off_);
            return 1;
        }
        break;

    case FL_RELEASE:
        if (dragging_) {
            dragging_ = false;
            fl_cursor(FL_CURSOR_DEFAULT);
            return 1;
        }
        break;
    }
    return Fl_Group::handle(event);
}

void Splitter::resize(int x, int y, int w, int h) {
    /* Keep divider at same proportional position */
    double ratio = (double)(div_x_ - this->x()) / (double)this->w();
    Fl_Group::resize(x, y, w, h);
    int new_div = x + (int)(ratio * w);
    /* Clamp */
    if (new_div < x + 80)      new_div = x + 80;
    if (new_div > x + w - 80)  new_div = x + w - 80;
    div_x_ = new_div;

    if (children() >= 2) {
        child(0)->resize(x,       y, new_div - x,          h);
        child(1)->resize(new_div, y, x + w - new_div, h);
    }
    redraw();
}

void Splitter::draw() {
    /* Draw background, then children, then divider line on top */
    Fl_Group::draw();
    /* Subtle divider */
    fl_color(fl_darker(FL_BACKGROUND_COLOR));
    fl_line(div_x_, y(), div_x_, y() + h());
}
