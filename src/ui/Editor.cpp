#include "Editor.h"
#include "../theme.h"
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <cstdlib>

Editor::Editor(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h)
{
    box(FL_FLAT_BOX);   /* FLTK fills bg before drawing children */
    int th = 28, pad = 4, tx = x+pad, ty = y+pad;

    auto mkbtn = [&](int bw, const char *lbl, bool bold=false) {
        Fl_Button *b = new Fl_Button(tx, ty, bw, th, lbl);
        b->box(FL_FLAT_BOX);
        b->labelfont(bold ? FL_COURIER_BOLD : FL_COURIER);
        b->labelsize(11);
        tx += bw + 4;
        return b;
    };

    run_btn_   = mkbtn(60, "run ^J", true);
    run_btn_->callback(run_cb_static, this);
    clear_btn_ = mkbtn(44, "clear");
    clear_btn_->callback(clear_cb_static, this);
    pads_btn_  = mkbtn(36, "pads");
    theme_btn_ = mkbtn(48, "theme");

    int ey = ty + th + pad;
    int eh = h - th - pad*3;
    buf_    = new Fl_Text_Buffer();
    editor_ = new Fl_Text_Editor(x+pad, ey, w-pad*2, eh);
    editor_->buffer(buf_);
    editor_->box(FL_FLAT_BOX);
    editor_->textfont(FL_COURIER);
    editor_->textsize(13);

    end();
    resizable(editor_);
}

Editor::~Editor() {
    /* Unregister buffer callbacks before deletion to avoid FLTK warnings */
    editor_->buffer(nullptr);
    delete buf_;
}

void Editor::draw() {
    /* Set colours before FLTK draws the box + children */
    color(T.bg);
    run_btn_->color(T.btn);   run_btn_->labelcolor(T.accent);
    clear_btn_->color(T.btn); clear_btn_->labelcolor(T.muted);
    pads_btn_->color(T.btn);  pads_btn_->labelcolor(T.muted);
    theme_btn_->color(T.btn); theme_btn_->labelcolor(T.muted);
    editor_->color(T.surface);
    editor_->textcolor(T.text);
    editor_->cursor_color(T.accent);
    editor_->selection_color(T.btn);
    Fl_Group::draw();
}

const char *Editor::code() const { return buf_->text(); }
void Editor::set_code(const char *c) { buf_->text(c ? c : ""); }
void Editor::set_pads_cb(Fl_Callback *cb, void *ud)  { pads_btn_->callback(cb, ud); }
void Editor::set_theme_cb(Fl_Callback *cb, void *ud) { theme_btn_->callback(cb, ud); }

int Editor::handle(int event) {
    if (event == FL_KEYDOWN) {
        int k = Fl::event_key();
        if (Fl::event_ctrl() && (k==FL_Enter||k=='j'||k=='\n'||k=='\r'))
            { do_run(); return 1; }
    }
    return Fl_Group::handle(event);
}

void Editor::run_cb_static(Fl_Widget*,void*ud)   { ((Editor*)ud)->do_run(); }
void Editor::clear_cb_static(Fl_Widget*,void*ud) { ((Editor*)ud)->do_clear(); }

void Editor::do_run() {
    char *txt = buf_->text();
    if (run_cb_ && txt && *txt) run_cb_(txt, run_ud_);
    free(txt);
}
void Editor::do_clear() { buf_->text(""); editor_->take_focus(); }
