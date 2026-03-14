#include "SlotStrip.h"
#include "../engine.h"
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <cstring>
#include <cstdio>

class SlotCard : public Fl_Box {
public:
    SlotCard(int x,int y,int w,int h,int idx,SlotStrip *strip)
        : Fl_Box(x,y,w,h), idx_(idx), strip_(strip), highlight_(false) {}

    void draw() override {
        const Slot *s = engine_slots() + idx_;
        bool filled = s->samples != nullptr;

        fl_color(highlight_ ? fl_rgb_color(0xe8,0xe8,0xe8) : (filled ? fl_rgb_color(0xd0,0xd8,0xd0) : fl_rgb_color(0xe8,0xe8,0xe8)));
        fl_rectf(x(),y(),w(),h());
        fl_color(filled ? fl_rgb_color(0xb0,0xb0,0xb0) : fl_rgb_color(0xe8,0xe8,0xe8));
        fl_rect(x(),y(),w(),h());

        fl_color(fl_rgb_color(0x70,0x70,0x70)); fl_font(FL_COURIER,9);
        char ib[4]; snprintf(ib,sizeof(ib),"%X",idx_);
        fl_draw(ib, x()+3, y()+9);

        if (filled) {
            fl_color(fl_rgb_color(0x00,0x78,0xd7)); fl_font(FL_COURIER,8);
            char lb[24]; strncpy(lb,s->label,23); lb[23]='\0';
            fl_push_clip(x()+2,y(),w()-4,h());
            fl_draw(lb, x()+2, y()+h()-3);
            fl_pop_clip();
            draw_wave(s->samples, s->len);
        } else {
            fl_color(fl_rgb_color(0x70,0x70,0x70)); fl_font(FL_COURIER,12);
            fl_draw("–", x()+w()/2-4, y()+h()/2+4);
        }
    }

    int handle(int event) override {
        switch(event) {
        case FL_PUSH:
            if (Fl::event_button()==FL_LEFT_MOUSE) {
                highlight_=true; redraw();
                strip_->fire_play(idx_);   /* play on mousedown */
                return 1;
            }
            if (Fl::event_button()==FL_RIGHT_MOUSE) {
                strip_->fire_menu(idx_, Fl::event_x_root(), Fl::event_y_root());
                return 1;
            }
            return 1;
        case FL_RELEASE:
            if (Fl::event_button()==FL_LEFT_MOUSE) { highlight_=false; redraw(); }
            return 1;
        case FL_ENTER: return 1;
        case FL_LEAVE: highlight_=false; redraw(); return 1;
        }
        return Fl_Box::handle(event);
    }

    void flash() {
        highlight_=true; redraw();
        Fl::add_timeout(0.1,[](void*v){
            SlotCard*c=(SlotCard*)v; c->highlight_=false; c->redraw();
        },this);
    }

private:
    int        idx_;
    SlotStrip *strip_;
    bool       highlight_;

    void draw_wave(const float *samples, int len) {
        if (!samples || len==0) return;
        int wx=x()+1, wy=y()+11, ww=w()-2, wh=h()-20;
        if (ww<=0||wh<=0) return;
        int cy=wy+wh/2;
        fl_color(fl_rgb_color(0x00,0x78,0xd7)); fl_line_style(FL_SOLID,1);
        int prev=cy;
        for (int px=0;px<ww;px++) {
            int si=(int)((float)px/(float)ww*(float)len);
            if (si < 0) si = 0;
            if (si >= len) si = len-1;
            float v = samples[si];
            if (v >  1.f) v =  1.f;
            if (v < -1.f) v = -1.f;
            int py=cy-(int)(v*(float)(wh/2-1));
            if(px>0) fl_line(wx+px-1,prev,wx+px,py);
            prev=py;
        }
        fl_line_style(0);
    }
};

SlotStrip::SlotStrip(int x,int y,int w,int h) : Fl_Group(x,y,w,h) {
    int cols=8,rows=2,pad=2;
    int cw=(w-pad*(cols+1))/cols;
    int ch=(h-pad*(rows+1))/rows;
    for(int i=0;i<NUM_SLOTS;i++) {
        int col=i%cols,row=i/cols;
        cards_[i]=new SlotCard(x+pad+col*(cw+pad), y+pad+row*(ch+pad), cw,ch, i,this);
        add(cards_[i]);
    }
    end();
}

void SlotStrip::draw() {
    fl_color(fl_rgb_color(0xe8,0xe8,0xe8)); fl_rectf(x(),y(),w(),h());
    draw_children();
}

void SlotStrip::refresh_slot(int idx) { if(idx>=0&&idx<NUM_SLOTS) cards_[idx]->redraw(); }
void SlotStrip::refresh_all()         { for(int i=0;i<NUM_SLOTS;i++) cards_[i]->redraw(); }
void SlotStrip::fire_play(int idx)    { if(play_cb_) play_cb_(idx,play_ud_); }
void SlotStrip::fire_menu(int idx,int x,int y) { if(menu_cb_) menu_cb_(idx,x,y,menu_ud_); }
