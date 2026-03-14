#include "theme.h"
#include <FL/Fl_Window.H>

ThemePalette T;
int          g_theme_idx = -1;

static const ThemePalette PALETTES[3] = {
    /* 0 — phosphor green terminal
       bg near-black, text bright green, surfaces clearly darker than bg */
    {
        /* bg       */ fl_rgb_color(0x08,0x0e,0x08),
        /* surface  */ fl_rgb_color(0x0e,0x18,0x0e),
        /* text     */ fl_rgb_color(0xb0,0xe8,0xb0),
        /* accent   */ fl_rgb_color(0x44,0xdd,0x44),
        /* btn      */ fl_rgb_color(0x14,0x26,0x14),
        /* muted    */ fl_rgb_color(0x60,0x88,0x60),
        /* border   */ fl_rgb_color(0x22,0x44,0x22),
        /* cell_ok  */ fl_rgb_color(0x0a,0x18,0x0a),
        /* cell_er  */ fl_rgb_color(0x1a,0x08,0x08),
        /* code_bg  */ fl_rgb_color(0x06,0x0e,0x06),
        /* wave     */ fl_rgb_color(0x44,0xdd,0x44),
        /* error    */ fl_rgb_color(0xff,0x44,0x44),
        /* pad      */ fl_rgb_color(0x10,0x1e,0x10),
        /* pad_fill */ fl_rgb_color(0x18,0x32,0x18),
    },
    /* 1 — dark blue (slate) */
    {
        /* bg       */ fl_rgb_color(0x10,0x12,0x1a),
        /* surface  */ fl_rgb_color(0x18,0x1c,0x2c),
        /* text     */ fl_rgb_color(0xcc,0xd4,0xf0),
        /* accent   */ fl_rgb_color(0x60,0xa0,0xff),
        /* btn      */ fl_rgb_color(0x20,0x26,0x40),
        /* muted    */ fl_rgb_color(0x68,0x70,0x98),
        /* border   */ fl_rgb_color(0x28,0x32,0x58),
        /* cell_ok  */ fl_rgb_color(0x14,0x18,0x2c),
        /* cell_er  */ fl_rgb_color(0x28,0x10,0x10),
        /* code_bg  */ fl_rgb_color(0x0c,0x0e,0x18),
        /* wave     */ fl_rgb_color(0x60,0xa0,0xff),
        /* error    */ fl_rgb_color(0xff,0x55,0x55),
        /* pad      */ fl_rgb_color(0x18,0x1e,0x34),
        /* pad_fill */ fl_rgb_color(0x22,0x2c,0x4c),
    },
    /* 2 — light */
    {
        /* bg       */ fl_rgb_color(0xec,0xec,0xea),
        /* surface  */ fl_rgb_color(0xe0,0xe0,0xdc),
        /* text     */ fl_rgb_color(0x18,0x18,0x1e),
        /* accent   */ fl_rgb_color(0x00,0x68,0xb8),
        /* btn      */ fl_rgb_color(0xcc,0xcc,0xc8),
        /* muted    */ fl_rgb_color(0x70,0x70,0x78),
        /* border   */ fl_rgb_color(0xaa,0xaa,0xa8),
        /* cell_ok  */ fl_rgb_color(0xe4,0xf0,0xe4),
        /* cell_er  */ fl_rgb_color(0xf0,0xe4,0xe4),
        /* code_bg  */ fl_rgb_color(0xf8,0xf8,0xf6),
        /* wave     */ fl_rgb_color(0x00,0x68,0xb8),
        /* error    */ fl_rgb_color(0xb8,0x10,0x10),
        /* pad      */ fl_rgb_color(0xd8,0xd8,0xd4),
        /* pad_fill */ fl_rgb_color(0xc4,0xd8,0xc4),
    },
};

void theme_set(int idx) {
    if (idx < 0) idx = 0;
    if (idx > 2) idx = 2;
    g_theme_idx = idx;
    T = PALETTES[idx];
    for (Fl_Window *w = Fl::first_window(); w; w = Fl::next_window(w))
        w->redraw();
}
