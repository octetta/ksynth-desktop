#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Buffer.H>

typedef void (*RunCB)(const char *code, void *ud);

class Editor : public Fl_Group {
public:
    Editor(int x, int y, int w, int h);
    ~Editor();

    const char *code() const;
    void        set_code(const char *code);

    void on_run(RunCB cb, void *ud) { run_cb_ = cb; run_ud_ = ud; }
    void set_pads_cb(Fl_Callback *cb, void *ud);
    void set_theme_cb(Fl_Callback *cb, void *ud);

    void draw() override;
    int handle(int event) override;

private:
    Fl_Button      *run_btn_;
    Fl_Button      *clear_btn_;
    Fl_Button      *pads_btn_;
    Fl_Button      *theme_btn_;
    Fl_Text_Editor *editor_;
    Fl_Text_Buffer *buf_;

    RunCB  run_cb_ = nullptr;
    void  *run_ud_ = nullptr;

    static void run_cb_static(Fl_Widget*, void*);
    static void clear_cb_static(Fl_Widget*, void*);
    void do_run();
    void do_clear();
};
