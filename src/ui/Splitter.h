#pragma once
#include <FL/Fl_Group.H>

/* A horizontal two-pane splitter.
   Left and right children are resized when the divider is dragged.
   Both children must be added before calling end(). */
class Splitter : public Fl_Group {
public:
    Splitter(int x, int y, int w, int h);
    int  handle(int event) override;
    void resize(int x, int y, int w, int h) override;
    void draw() override;

private:
    static const int DIV_W = 4;   /* divider grab width in pixels */
    int   div_x_  = 0;            /* current divider x (absolute) */
    bool  dragging_ = false;
    int   drag_off_ = 0;

    void apply_split(int new_div_x);
    bool over_divider(int ex) const;
};
