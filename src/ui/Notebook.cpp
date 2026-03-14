#include "Notebook.h"
#include "../engine.h"
#include "../theme.h"
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <utility>
#include <vector>

static const int WAVE_H     = 40;
static const int BANK_ROW_H = 20;
static const int HDR_H      = 18;
static const int CELL_PAD   = 4;
static const int CELL_GAP   = 3;

/* ── NbCell ──────────────────────────────────────────────────────────
   Children are positioned in ABSOLUTE screen coordinates.
   resize() re-lays them out whenever we move.
   ─────────────────────────────────────────────────────────────────── */
class NbCell : public Fl_Group {
public:
    bool        success_;
    std::string code_;
    Notebook   *nb_;
    float      *wave_data_;
    int         wave_len_;

    NbCell(int X, int Y, int W, const NotebookEntry &e, Notebook *nb)
        : Fl_Group(X, Y, W, cell_height(e)),
          success_(e.success), code_(e.code), nb_(nb),
          wave_data_(nullptr), wave_len_(0), wave_box_(nullptr)
    {
        box(FL_NO_BOX);
        build_children(e);
        end();
    }

    ~NbCell() { free(wave_data_); }

    void resize(int X, int Y, int W, int H) override {
        Fl_Widget::resize(X, Y, W, H);   /* move our bounding box */
        layout_children();               /* reposition children to match */
    }

    void draw() override {
        bool ok = success_;
        fl_color(ok ? T.cell_ok : T.cell_er);
        fl_rectf(x(), y(), w(), h());
        fl_color(T.border);
        fl_rect(x(), y(), w(), h());

        hdr_->labelcolor(ok ? T.accent : T.error);
        hdr_->color(ok ? T.cell_ok : T.cell_er);
        del_btn_->color(ok ? T.cell_ok : T.cell_er);
        del_btn_->labelcolor(T.muted);
        code_box_->color(T.code_bg);
        code_box_->labelcolor(T.text);
        if (wave_box_) wave_box_->color(T.code_bg);
        for (int i = 0; i < children(); i++) {
            Fl_Widget *c = child(i);
            if (c==hdr_||c==del_btn_||c==code_box_||c==wave_box_) continue;
            c->box(FL_FLAT_BOX);
            c->color(T.btn);
            c->labelcolor(T.accent);
        }

        draw_children();
        if (wave_box_ && wave_data_ && wave_len_ > 0) draw_wave();
    }

    int handle(int event) override {
        if (event == FL_PUSH) {
            if (wave_box_ && Fl::event_inside(wave_box_) && wave_data_)
                { nb_->fire_play(wave_data_, wave_len_); return 1; }
            if (code_box_ && Fl::event_inside(code_box_))
                { nb_->fire_copy(code_.c_str()); return 1; }
        }
        return Fl_Group::handle(event);
    }

    static int cell_height(const NotebookEntry &e) {
        int h = CELL_PAD + HDR_H + 2 + code_height(e.code) + CELL_PAD;
        if (e.success && e.floatData && e.floatLen > 0)
            h += WAVE_H + 2 + BANK_ROW_H + CELL_PAD;
        return h + CELL_PAD;
    }

private:
    Fl_Box    *hdr_;
    Fl_Button *del_btn_;
    Fl_Box    *code_box_;
    Fl_Box    *wave_box_;

    static int code_height(const std::string &s) {
        int lines = 1;
        for (char c : s) if (c=='\n') lines++;
        if (lines > 40) lines = 40;
        return lines * 14 + 8;
    }

    void build_children(const NotebookEntry &e) {
        /* Dummy positions — layout_children() fixes them after construction */
        hdr_ = new Fl_Box(0, 0, 1, HDR_H);
        hdr_->box(FL_NO_BOX);
        hdr_->labelfont(FL_COURIER); hdr_->labelsize(10);
        hdr_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        if (e.success) {
            char buf[80];
            snprintf(buf, sizeof(buf),
                     "OK  %d smp  %.0fms    click wave=play   click code=copy",
                     e.samples, e.samples * 1000.0 / 44100.0);
            hdr_->copy_label(buf);
        } else {
            hdr_->copy_label(("ERR  " + e.error).c_str());
        }

        del_btn_ = new Fl_Button(0, 0, HDR_H, HDR_H, "x");
        del_btn_->box(FL_FLAT_BOX);
        del_btn_->labelfont(FL_COURIER); del_btn_->labelsize(9);
        del_btn_->callback([](Fl_Widget*, void *ud){
            NbCell *cell = (NbCell*)ud;
            cell->nb_->remove_cell(cell);
        }, this);

        code_box_ = new Fl_Box(0, 0, 1, code_height(e.code));
        code_box_->box(FL_FLAT_BOX);
        code_box_->labelfont(FL_COURIER); code_box_->labelsize(11);
        code_box_->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);
        code_box_->copy_label(e.code.c_str());

        wave_box_ = nullptr;
        if (e.success && e.floatData && e.floatLen > 0) {
            wave_len_  = e.floatLen;
            wave_data_ = (float*)malloc((size_t)wave_len_ * sizeof(float));
            if (wave_data_)
                memcpy(wave_data_, e.floatData, (size_t)wave_len_ * sizeof(float));

            wave_box_ = new Fl_Box(0, 0, 1, WAVE_H);
            wave_box_->box(FL_FLAT_BOX);

            struct Arg { NbCell *cell; int slot; };
            for (int i = 0; i < ENGINE_NUM_SLOTS; i++) {
                char lbl[6]; snprintf(lbl, sizeof(lbl), ">%X", i);
                Fl_Button *b = new Fl_Button(0, 0, 1, BANK_ROW_H);
                b->copy_label(lbl);
                b->box(FL_FLAT_BOX);
                b->labelfont(FL_COURIER); b->labelsize(9);
                Arg *arg = new Arg{this, i};
                b->callback([](Fl_Widget*, void *ud){
                    Arg *a = (Arg*)ud;
                    a->cell->nb_->fire_bank(a->slot,
                        a->cell->wave_data_, a->cell->wave_len_,
                        a->cell->code_.c_str());
                }, arg);
            }
        }

        layout_children();
    }

    /* All positions computed from our own x(),y() */
    void layout_children() {
        int X  = x(), Y = y(), W = w();
        int cw = W - CELL_PAD * 2;
        int cx = X + CELL_PAD;
        int cy = Y + CELL_PAD;

        hdr_->resize(cx, cy, cw - HDR_H - 2, HDR_H);
        del_btn_->resize(cx + cw - HDR_H, cy, HDR_H, HDR_H);
        cy += HDR_H + 2;

        code_box_->resize(cx, cy, cw, code_box_->h());
        cy += code_box_->h() + CELL_PAD;

        if (wave_box_) {
            wave_box_->resize(cx, cy, cw, WAVE_H);
            cy += WAVE_H + 2;

            int n  = ENGINE_NUM_SLOTS;
            int bw = (cw - (n - 1)) / n;
            int bx = cx;
            for (int i = 0; i < n; i++) {
                child(4 + i)->resize(bx, cy, bw, BANK_ROW_H);
                bx += bw + 1;
            }
        }
    }

    void draw_wave() {
        int wx = wave_box_->x() + 1, wy = wave_box_->y() + 1;
        int ww = wave_box_->w() - 2, wh = wave_box_->h() - 2;
        if (ww <= 0 || wh <= 0 || wave_len_ == 0) return;
        int mid = wy + wh / 2;
        fl_push_clip(wx, wy, ww, wh);
        fl_color(T.wave); fl_line_style(FL_SOLID, 1);
        int prev = mid;
        for (int px = 0; px < ww; px++) {
            int si = (int)((float)px / (float)ww * (float)wave_len_);
            if (si < 0) si = 0;
            if (si >= wave_len_) si = wave_len_ - 1;
            float v = wave_data_[si];
            if (v >  1.f) v =  1.f;
            if (v < -1.f) v = -1.f;
            int py = mid - (int)(v * (float)(wh / 2 - 1));
            if (px > 0) fl_line(wx+px-1, prev, wx+px, py);
            prev = py;
        }
        fl_line_style(0);
        fl_pop_clip();
    }
};

/* ── Notebook ────────────────────────────────────────────────────────
   Uses Fl_Scroll correctly: children are placed at ABSOLUTE coordinates
   starting at (x(), y()). Fl_Scroll clips drawing to its viewport and
   shifts children by the scroll offset internally — we must NOT add the
   scroll offset ourselves, or everything double-offsets.

   content_h_ tracks total stacked height of all cells (no scroll offset).
   Cells sit at y = notebook->y() + cumulative_height_of_prior_cells.
   ─────────────────────────────────────────────────────────────────── */

Notebook::Notebook(int X, int Y, int W, int H)
    : Fl_Scroll(X, Y, W, H), content_h_(0)
{
    box(FL_FLAT_BOX);
    type(Fl_Scroll::VERTICAL_ALWAYS);
    end();
}

void Notebook::draw() {
    color(T.bg);
    scrollbar.color(T.btn);
    scrollbar.selection_color(T.accent);
    Fl_Scroll::draw();
}

void Notebook::relayout() {
    int inner_w = w() - Fl::scrollbar_size();
    int cy = 0;   /* content-relative: 0 = top of scroll area */
    for (int i = 0; i < children(); i++) {
        Fl_Widget *c = child(i);
        if (c == &scrollbar || c == &hscrollbar) continue;
        /* Place at y() + cy so absolute position is correct */
        c->resize(x(), y() + cy, inner_w, c->h());
        cy += c->h() + CELL_GAP;
    }
    content_h_ = cy;
    redraw();
}

void Notebook::add_entry(const NotebookEntry &entry) {
    int inner_w = w() - Fl::scrollbar_size();
    /* New cell goes at the bottom of existing content */
    NbCell *cell = new NbCell(x(), y() + content_h_, inner_w, entry, this);
    add(cell);
    content_h_ += cell->h() + CELL_GAP;
    redraw();

    /* Scroll so the new cell's top is visible */
    int target = content_h_ - cell->h() - CELL_GAP;
    if (target < 0) target = 0;
    Fl::add_timeout(0.02, [](void *v){
        auto *p = (std::pair<Notebook*,int>*)v;
        p->first->scroll_to(0, p->second);
        delete p;
    }, new std::pair<Notebook*,int>(this, target));
}

void Notebook::remove_cell(NbCell *cell) {
    remove(cell);
    delete cell;
    relayout();
}

void Notebook::clear_all() {
    std::vector<Fl_Widget*> cells;
    for (int i = 0; i < children(); i++) {
        Fl_Widget *c = child(i);
        if (c != &scrollbar && c != &hscrollbar)
            cells.push_back(c);
    }
    for (Fl_Widget *c : cells) { remove(c); delete c; }
    content_h_ = 0;
    scroll_to(0, 0);
    redraw();
}

void Notebook::fire_copy(const char *c)        { if (copy_cb_) copy_cb_(c, copy_ud_); }
void Notebook::fire_play(const float *s, int n){ if (play_cb_) play_cb_(s, n, play_ud_); }
void Notebook::fire_bank(int sl, const float *s, int n, const char *c) {
    if (bank_cb_) bank_cb_(sl, s, n, c, bank_ud_);
}
