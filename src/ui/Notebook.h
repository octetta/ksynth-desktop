#pragma once
#include <FL/Fl_Scroll.H>
#include <string>
#include <vector>

struct NotebookEntry {
    bool        success  = false;
    std::string code;
    std::string error;
    int         samples  = 0;
    float      *floatData = nullptr;
    int         floatLen  = 0;
};

/* Snapshot of one cell for session save */
struct HistoryEntry {
    bool        success;
    std::string code;
    std::string error;
    int         samples;
    const float *audio;     /* points into cell — valid until cell is destroyed */
    int         audio_len;
};

class NbCell;  /* forward — defined in Notebook.cpp */

typedef void (*CopyToCB)(const char *code, void *ud);
typedef void (*PlayWaveCB)(const float *samples, int len, void *ud);
typedef void (*BankCB)(int slot, const float *samples, int len,
                       const char *code, void *ud);

class Notebook : public Fl_Scroll {
public:
    Notebook(int x, int y, int w, int h);

    void draw() override;
    void resize(int x, int y, int w, int h) override;

    void add_entry(const NotebookEntry &entry);
    void remove_cell(NbCell *cell);   /* called by NbCell delete button */
    void clear_all();

    /* Iterate cells oldest-first; cb returns false to stop */
    void visit_cells(bool (*cb)(NbCell *cell, void *ud), void *ud);

    /* Return snapshot of all cells for session save */
    std::vector<HistoryEntry> get_history() const;

    void on_copy_to(CopyToCB cb, void *ud) { copy_cb_=cb; copy_ud_=ud; }
    void on_play(PlayWaveCB cb, void *ud)  { play_cb_=cb; play_ud_=ud; }
    void on_bank(BankCB cb, void *ud)      { bank_cb_=cb; bank_ud_=ud; }

    /* Called by NbCell */
    void fire_copy(const char *code);
    void fire_play(const float *samples, int len);
    void fire_bank(int slot, const float *samples, int len, const char *code);

private:
    int         content_h_ = 0;
    CopyToCB    copy_cb_   = nullptr;  void *copy_ud_ = nullptr;
    PlayWaveCB  play_cb_   = nullptr;  void *play_ud_ = nullptr;
    BankCB      bank_cb_   = nullptr;  void *bank_ud_ = nullptr;

    void relayout();
};
