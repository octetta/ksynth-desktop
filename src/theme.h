#pragma once
#include <FL/Fl.H>

/* Current palette — written by theme_set(), read by all widget files */
struct ThemePalette {
    Fl_Color bg;        /* window / panel background   */
    Fl_Color surface;   /* editor / code area          */
    Fl_Color text;      /* primary label / code text   */
    Fl_Color accent;    /* green / blue highlight      */
    Fl_Color btn;       /* button background           */
    Fl_Color muted;     /* secondary / inactive label  */
    Fl_Color border;    /* dividers                    */
    Fl_Color cell_ok;   /* notebook success cell bg    */
    Fl_Color cell_er;   /* notebook error cell bg      */
    Fl_Color code_bg;   /* notebook code area bg       */
    Fl_Color wave;      /* waveform line               */
    Fl_Color error;     /* error label                 */
    Fl_Color pad;       /* pad button bg (empty)       */
    Fl_Color pad_fill;  /* pad button bg (filled)      */
};

extern ThemePalette T;   /* single global instance */
extern int          g_theme_idx;

/* 0=retro dark green, 1=dark blue-grey, 2=light */
void theme_set(int idx);
