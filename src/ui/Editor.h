#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Box.H>

typedef void (*RunCB)(const char *code, void *ud);

class Editor : public Fl_Group {
public:
    Editor(int x, int y, int w, int h);
    ~Editor();

    const char *code() const;
    void        set_code(const char *c);

    void on_run(RunCB cb, void *ud) { run_cb_ = cb; run_ud_ = ud; }
    void set_pads_cb(Fl_Callback *cb, void *ud);

    int handle(int event) override;
    void resize(int x, int y, int w, int h) override;

private:
    Fl_Button      *run_btn_;
    Fl_Button      *clear_btn_;
    Fl_Button      *pads_btn_;
    Fl_Text_Editor *editor_;
    Fl_Text_Buffer *buf_;
    Fl_Box         *repl_out_;   /* output label */
    Fl_Input       *repl_in_;   /* single-line input */

    RunCB  run_cb_ = nullptr;
    void  *run_ud_ = nullptr;

    static void run_cb_static(Fl_Widget*, void*);
    static void clear_cb_static(Fl_Widget*, void*);
    static void repl_cb_static(Fl_Widget*, void*);
    void do_run();
    void do_clear();
    void do_repl();
};
