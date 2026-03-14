#include "Editor.h"
#include "../engine.h"
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <cstdlib>
#include <cstring>

static const int TOOLBAR_H  = 28;
static const int REPL_OUT_H = 20;
static const int REPL_IN_H  = 24;
static const int PAD        = 4;
static const int REPL_H     = REPL_OUT_H + REPL_IN_H + PAD * 3;

Editor::Editor(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h)
{
    box(FL_FLAT_BOX);
    int tx = x + PAD, ty = y + PAD;

    /* ── toolbar ───────────────────────────────────────────────── */
    auto mkbtn = [&](int bw, const char *lbl, bool bold = false) {
        Fl_Button *b = new Fl_Button(tx, ty, bw, TOOLBAR_H, lbl);
        b->box(FL_FLAT_BOX);
        b->labelfont(bold ? FL_COURIER_BOLD : FL_COURIER);
        b->labelsize(11);
        tx += bw + PAD;
        return b;
    };

    run_btn_   = mkbtn(60, "run ^J", true);
    run_btn_->callback(run_cb_static, this);
    clear_btn_ = mkbtn(44, "clear");
    clear_btn_->callback(clear_cb_static, this);
    pads_btn_  = mkbtn(36, "pads");

    /* ── main text editor ──────────────────────────────────────── */
    int ey = y + PAD + TOOLBAR_H + PAD;
    int eh = h - PAD - TOOLBAR_H - PAD - REPL_H - PAD;
    buf_    = new Fl_Text_Buffer();
    editor_ = new Fl_Text_Editor(x + PAD, ey, w - PAD*2, eh);
    editor_->buffer(buf_);
    editor_->box(FL_DOWN_BOX);
    editor_->textfont(FL_COURIER);
    editor_->textsize(13);

    /* ── REPL strip ────────────────────────────────────────────── */
    int ry = ey + eh + PAD;

    /* Output line — left-aligned text, no box, monospace */
    repl_out_ = new Fl_Box(x + PAD, ry, w - PAD*2, REPL_OUT_H, "");
    repl_out_->box(FL_FLAT_BOX);
    repl_out_->labelfont(FL_COURIER);
    repl_out_->labelsize(11);
    repl_out_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    ry += REPL_OUT_H + PAD;

    /* Input line */
    repl_in_ = new Fl_Input(x + PAD, ry, w - PAD*2, REPL_IN_H);
    repl_in_->box(FL_DOWN_BOX);
    repl_in_->textfont(FL_COURIER);
    repl_in_->textsize(12);
    repl_in_->when(FL_WHEN_ENTER_KEY_ALWAYS);
    repl_in_->callback(repl_cb_static, this);

    end();
    resizable(editor_);
}

Editor::~Editor() {
    editor_->buffer(nullptr);
    delete buf_;
}

const char *Editor::code() const { return buf_->text(); }
void Editor::set_code(const char *c) { buf_->text(c ? c : ""); }
void Editor::set_pads_cb(Fl_Callback *cb, void *ud) { pads_btn_->callback(cb, ud); }

void Editor::resize(int x, int y, int w, int h) {
    Fl_Group::resize(x, y, w, h);
    /* Mark our parent (tile) fully damaged so exposed strip gets cleared */
    if (parent()) parent()->redraw();
    redraw();
}

int Editor::handle(int event) {
    if (event == FL_KEYDOWN) {
        int k = Fl::event_key();
        if (Fl::event_ctrl() && (k==FL_Enter||k=='j'||k=='\n'||k=='\r'))
            { do_run(); return 1; }
    }
    return Fl_Group::handle(event);
}

void Editor::run_cb_static(Fl_Widget*, void *ud)   { ((Editor*)ud)->do_run(); }
void Editor::clear_cb_static(Fl_Widget*, void *ud) { ((Editor*)ud)->do_clear(); }
void Editor::repl_cb_static(Fl_Widget*, void *ud)  { ((Editor*)ud)->do_repl(); }

void Editor::do_run() {
    char *txt = buf_->text();
    if (run_cb_ && txt && *txt) run_cb_(txt, run_ud_);
    free(txt);
}

void Editor::do_clear() {
    buf_->text("");
    editor_->take_focus();
}

void Editor::do_repl() {
    const char *expr = repl_in_->value();
    if (!expr || !*expr) return;

    char result[256];
    int ok = engine_eval_repl(expr, result, sizeof(result));

    /* Format output: show expr and result */
    char display[320];
    if (ok && result[0])
        snprintf(display, sizeof(display), "> %s  =>  %s", expr, result);
    else if (ok)
        snprintf(display, sizeof(display), "> %s", expr);
    else
        snprintf(display, sizeof(display), "> %s  !  %s", expr, result);

    repl_out_->copy_label(display);
    /* Colour output: red tint on error, normal otherwise */
    repl_out_->labelcolor(ok ? FL_FOREGROUND_COLOR : FL_RED);
    repl_out_->redraw();

    /* Don't clear input — user can edit and re-run, or type new expr */
    repl_in_->take_focus();
    repl_in_->insert_position(0, (int)strlen(repl_in_->value()));  /* select all */
}
