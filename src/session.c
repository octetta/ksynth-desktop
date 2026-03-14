#include "session.h"
#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAGIC "ksynth-session-v1"

/* ── base64 ───────────────────────────────────────────────────────── */

static const char B64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char *b64_encode(const unsigned char *src, size_t len) {
    size_t out_len = 4 * ((len + 2) / 3) + 1;
    char *out = malloc(out_len);
    if (!out) return NULL;
    size_t i=0, j=0;
    while (i+2 < len) {
        out[j++]=B64[ src[i]  >>2];
        out[j++]=B64[((src[i]  &3)<<4)|(src[i+1]>>4)];
        out[j++]=B64[((src[i+1]&0xf)<<2)|(src[i+2]>>6)];
        out[j++]=B64[ src[i+2]&0x3f];
        i+=3;
    }
    if (i<len) {
        out[j++]=B64[src[i]>>2];
        if (i+1<len) {
            out[j++]=B64[((src[i]&3)<<4)|(src[i+1]>>4)];
            out[j++]=B64[((src[i+1]&0xf)<<2)];
        } else {
            out[j++]=B64[((src[i]&3)<<4)];
            out[j++]='=';
        }
        out[j++]='=';
    }
    out[j]='\0';
    return out;
}

static int b64_val(char c) {
    if (c>='A'&&c<='Z') return c-'A';
    if (c>='a'&&c<='z') return c-'a'+26;
    if (c>='0'&&c<='9') return c-'0'+52;
    if (c=='+') return 62;
    if (c=='/') return 63;
    return -1;
}

static float *b64_decode_f32(const char *src, size_t src_len, int *out_n) {
    *out_n = 0;
    size_t out_bytes = (src_len/4)*3+4;
    unsigned char *bytes = malloc(out_bytes);
    if (!bytes) return NULL;
    size_t i=0,j=0;
    while (i+3<src_len) {
        int a=b64_val(src[i]),b=b64_val(src[i+1]);
        int c=b64_val(src[i+2]),d=b64_val(src[i+3]);
        if (a<0||b<0) break;
        bytes[j++]=(unsigned char)((a<<2)|(b>>4));
        if (src[i+2]!='='&&c>=0) bytes[j++]=(unsigned char)((b<<4)|(c>>2));
        if (src[i+3]!='='&&d>=0) bytes[j++]=(unsigned char)((c<<6)|d);
        i+=4;
    }
    if (j==0||(j%(sizeof(float)))!=0) { free(bytes); return NULL; }
    int n=(int)(j/sizeof(float));
    float *out=malloc(j);
    if (!out) { free(bytes); return NULL; }
    memcpy(out,bytes,j);
    free(bytes);
    *out_n=n;
    return out;
}

/* ── JSON write helpers ───────────────────────────────────────────── */

static void write_json_str(FILE *f, const char *s) {
    if (!s) { fputs("null",f); return; }
    fputc('"',f);
    for(;*s;s++){
        if(*s=='"') fputs("\\\"",f);
        else if(*s=='\\') fputs("\\\\",f);
        else if(*s=='\n') fputs("\\n",f);
        else if(*s=='\r') fputs("\\r",f);
        else if(*s=='\t') fputs("\\t",f);
        else fputc(*s,f);
    }
    fputc('"',f);
}

static void write_b64_f32(FILE *f, const float *data, int n) {
    if (!data||n<=0) { fputs("null",f); return; }
    char *b64=b64_encode((const unsigned char*)data,(size_t)n*sizeof(float));
    if (b64) { fprintf(f,"\"%s\"",b64); free(b64); }
    else fputs("null",f);
}

/* ── JSON read helpers — single-pass ─────────────────────────────── */

static const char *skip_ws(const char *p){
    while(*p==' '||*p=='\t'||*p=='\n'||*p=='\r')p++;
    return p;
}

/* Skip any JSON value, return ptr after it */
static const char *skip_value(const char *p) {
    p=skip_ws(p);
    if (*p=='"') {
        p++;
        while(*p&&*p!='"'){if(*p=='\\')p++;p++;}
        if(*p)p++;
    } else if (*p=='{'||*p=='[') {
        char open=*p,close=(*p=='{')?'}':']';
        int d=1; p++;
        while(*p&&d){
            if(*p==open)d++;
            else if(*p==close)d--;
            else if(*p=='"'){p++;while(*p&&*p!='"'){if(*p=='\\')p++;p++;}}
            if(d>0)p++;
        }
        if(*p)p++;
    } else {
        while(*p&&*p!=','&&*p!='}'&&*p!=']')p++;
    }
    return p;
}

/* Read a quoted JSON string into a heap buffer. Advances *pp. */
static char *read_str_alloc(const char **pp) {
    const char *p=skip_ws(*pp);
    if (*p!='"') { *pp=p; return strdup(""); }
    p++;
    size_t cap=256,n=0;
    char *buf=malloc(cap);
    if (!buf) { while(*p&&*p!='"'){if(*p=='\\')p++;p++;}if(*p)p++;*pp=p;return NULL; }
    while(*p&&*p!='"') {
        if (n+4>=cap){cap*=2;char*nb=realloc(buf,cap);if(nb)buf=nb;}
        if (*p=='\\'){
            p++;
            switch(*p){
            case '"': buf[n++]='"'; break; case '\\':buf[n++]='\\';break;
            case 'n': buf[n++]='\n';break; case 'r': buf[n++]='\r';break;
            case 't': buf[n++]='\t';break; default:  buf[n++]=*p;  break;
            }
        } else { buf[n++]=*p; }
        p++;
    }
    buf[n]='\0';
    if(*p=='"')p++;
    *pp=p;
    return buf;
}

/* Read a key name (advances *pp), returns heap string */
static char *read_key(const char **pp) {
    const char *p=skip_ws(*pp);
    if(*p!='"'){*pp=p;return NULL;}
    return read_str_alloc(pp);
}

/*
 * Single-pass object field reader.
 * Calls cb(key, value_ptr, ud) for each field.
 * value_ptr points to the start of the JSON value (after the colon).
 * The value has NOT been consumed — cb should consume it via skip_value if needed.
 * Returns ptr after the closing }.
 */
typedef void (*FieldCB)(const char *key, const char *val, void *ud);

static const char *parse_object(const char *p, FieldCB cb, void *ud) {
    p=skip_ws(p);
    if(*p=='{')p++;
    while(*p) {
        p=skip_ws(p);
        if(*p=='}'){p++;break;}
        if(*p==','){p++;continue;}
        if(*p!='"'){p++;continue;}
        char *key=read_key(&p);
        p=skip_ws(p);
        if(*p==':')p++;
        p=skip_ws(p);
        if(key){cb(key,p,ud);free(key);}
        p=skip_value(p);
        p=skip_ws(p);
        if(*p==',')p++;
    }
    return p;
}

/* ── save ─────────────────────────────────────────────────────────── */

int session_save(const char *path,
                 const char *editor_text,
                 const SessionEntry *history, int history_count,
                 const int *pad_slots, const int *pad_semitones, int num_pads) {
    FILE *f=fopen(path,"w");
    if(!f)return -1;
    const Slot *slots=engine_slots();

    fprintf(f,"{\n  \"magic\": \"%s\",\n  \"saved\": \"desktop\",\n",MAGIC);
    fprintf(f,"  \"editor\": "); write_json_str(f,editor_text?editor_text:"");
    fprintf(f,",\n  \"slots\": [\n");
    for(int i=0;i<ENGINE_NUM_SLOTS;i++){
        const Slot *s=slots+i;
        fprintf(f,"    {\"label\": "); write_json_str(f,s->label);
        fprintf(f,", \"baseRate\": %g, \"audio\": ",s->base_rate);
        write_b64_f32(f,s->samples,s->len);
        fprintf(f,"}%s\n",i<ENGINE_NUM_SLOTS-1?",":"");
    }
    fprintf(f,"  ],\n  \"pads\": [\n");
    for(int i=0;i<num_pads;i++)
        fprintf(f,"    {\"slot\": %d, \"semitones\": %d}%s\n",
                pad_slots[i],pad_semitones[i],i<num_pads-1?",":"");
    fprintf(f,"  ],\n  \"history\": [\n");
    for(int i=0;i<history_count;i++){
        const SessionEntry *h=history+i;
        fprintf(f,"    {\"code\": "); write_json_str(f,h->code?h->code:"");
        fprintf(f,", \"success\": %d, \"samples\": %d, \"error\": ",h->success,h->samples);
        write_json_str(f,h->error?h->error:"");
        fprintf(f,", \"audio\": "); write_b64_f32(f,h->audio,h->audio_len);
        fprintf(f,"}%s\n",i<history_count-1?",":"");
    }
    fprintf(f,"  ]\n}\n");
    fclose(f);
    return 0;
}

/* ── load ─────────────────────────────────────────────────────────── */

/* Per-slot parse state */
typedef struct { char label[64]; float base_rate; float *audio; int audio_n; } SlotParse;
static void slot_field_cb(const char *key, const char *val, void *ud) {
    SlotParse *s=(SlotParse*)ud;
    if(strcmp(key,"label")==0){
        const char *p=val; char *str=read_str_alloc(&p);
        if(str){strncpy(s->label,str,63);s->label[63]='\0';free(str);}
    } else if(strcmp(key,"baseRate")==0){
        s->base_rate=(float)atof(val);
    } else if(strcmp(key,"audio")==0){
        const char *p=skip_ws(val);
        if(*p=='"'){p++;const char *e=p;while(*e&&*e!='"')e++;
            s->audio=b64_decode_f32(p,(size_t)(e-p),&s->audio_n);}
    }
}

/* Per-pad parse state */
typedef struct { int slot; int semitones; } PadParse;
static void pad_field_cb(const char *key, const char *val, void *ud){
    PadParse *p=(PadParse*)ud;
    if(strcmp(key,"slot")==0)     p->slot=atoi(val);
    if(strcmp(key,"semitones")==0) p->semitones=atoi(val);
}

/* Per-history entry parse state */
typedef struct { char *code; int success; int samples; char *error; float *audio; int audio_n; } HistParse;
static void hist_field_cb(const char *key, const char *val, void *ud){
    HistParse *h=(HistParse*)ud;
    if(strcmp(key,"code")==0){
        const char *p=val; h->code=read_str_alloc(&p);
    } else if(strcmp(key,"success")==0){
        h->success=atoi(val);
    } else if(strcmp(key,"samples")==0){
        h->samples=atoi(val);
    } else if(strcmp(key,"error")==0){
        const char *p=val; h->error=read_str_alloc(&p);
    } else if(strcmp(key,"audio")==0){
        const char *p=skip_ws(val);
        if(*p=='"'){p++;const char *e=p;while(*e&&*e!='"')e++;
            h->audio=b64_decode_f32(p,(size_t)(e-p),&h->audio_n);}
    }
}

/* Top-level magic check */
typedef struct { char magic[64]; } MagicParse;
static void magic_field_cb(const char *key, const char *val, void *ud){
    MagicParse *m=(MagicParse*)ud;
    if(strcmp(key,"magic")==0){
        const char *p=val; char *s=read_str_alloc(&p);
        if(s){strncpy(m->magic,s,63);m->magic[63]='\0';free(s);}
    }
}

int session_load(const char *path, SessionLoadResult *out,
                 char *errmsg, int errmsg_len) {
    memset(out,0,sizeof(*out));
    for(int i=0;i<16;i++){out->pad_slots[i]=i;out->pad_semitones[i]=0;}

    FILE *f=fopen(path,"r");
    if(!f){snprintf(errmsg,errmsg_len,"Cannot open: %s",path);return -1;}
    fseek(f,0,SEEK_END);long sz=ftell(f);rewind(f);
    char *json=malloc((size_t)sz+1);
    if(!json){fclose(f);snprintf(errmsg,errmsg_len,"out of memory");return -1;}
    fread(json,1,(size_t)sz,f);json[sz]='\0';fclose(f);

    /* We need to parse the top-level object key by key.
       For large values (slots/history arrays) we handle them inline. */
    const char *p=skip_ws(json);
    if(*p=='{')p++;

    while(*p) {
        p=skip_ws(p);
        if(*p=='}'||*p=='\0')break;
        if(*p==','){p++;continue;}
        if(*p!='"'){p++;continue;}

        char *key=read_key(&p);
        p=skip_ws(p); if(*p==':')p++; p=skip_ws(p);
        if(!key){p=skip_value(p);continue;}

        if(strcmp(key,"magic")==0){
            char *val=read_str_alloc(&p);
            if(!val||strcmp(val,MAGIC)!=0){
                free(val);free(key);free(json);
                snprintf(errmsg,errmsg_len,"not a ksynth session file");return -1;
            }
            free(val);
        }
        else if(strcmp(key,"editor")==0){
            out->editor_text=read_str_alloc(&p);
        }
        else if(strcmp(key,"slots")==0){
            /* Parse array of slot objects */
            p=skip_ws(p); if(*p=='[')p++;
            int idx=0;
            while(*p&&idx<ENGINE_NUM_SLOTS){
                p=skip_ws(p); if(*p==']')break;
                if(*p==','){p++;continue;}
                if(*p=='{'){
                    SlotParse sp; memset(&sp,0,sizeof(sp)); sp.base_rate=1.0f;
                    p=parse_object(p,slot_field_cb,&sp);
                    engine_clear_slot(idx);
                    if(sp.audio&&sp.audio_n>0){
                        engine_bank_slot(idx,sp.audio,sp.audio_n,sp.label);
                        engine_set_slot_base_rate(idx,sp.base_rate);
                        free(sp.audio);
                    }
                    idx++;
                } else { p=skip_value(p); }
            }
            while(*p&&*p!=']')p++;
            if(*p)p++;
        }
        else if(strcmp(key,"pads")==0){
            p=skip_ws(p); if(*p=='[')p++;
            int idx=0;
            while(*p&&idx<16){
                p=skip_ws(p); if(*p==']')break;
                if(*p==','){p++;continue;}
                if(*p=='{'){
                    PadParse pp2; pp2.slot=idx; pp2.semitones=0;
                    p=parse_object(p,pad_field_cb,&pp2);
                    out->pad_slots[idx]=pp2.slot;
                    out->pad_semitones[idx]=pp2.semitones;
                    idx++;
                } else { p=skip_value(p); }
            }
            while(*p&&*p!=']')p++;
            if(*p)p++;
        }
        else if(strcmp(key,"history")==0){
            p=skip_ws(p); if(*p=='[')p++;
            /* Count entries */
            int count=0;
            const char *scan=p;
            while(*scan){
                scan=skip_ws(scan); if(*scan==']')break;
                if(*scan=='{'){count++;scan=skip_value(scan);}
                else{scan++;} 
            }
            if(count>0){
                out->history=(SessionEntry*)calloc((size_t)count,sizeof(SessionEntry));
                out->history_count=count;
                int idx=0;
                while(*p&&idx<count){
                    p=skip_ws(p); if(*p==']')break;
                    if(*p==','){p++;continue;}
                    if(*p=='{'){
                        HistParse hp; memset(&hp,0,sizeof(hp));
                        p=parse_object(p,hist_field_cb,&hp);
                        SessionEntry *e=&out->history[idx++];
                        e->code=(const char*)hp.code;
                        e->success=hp.success;
                        e->samples=hp.samples;
                        e->error=(const char*)(hp.error?hp.error:strdup(""));
                        e->audio=(const float*)hp.audio;
                        e->audio_len=hp.audio_n;
                    } else { p=skip_value(p); }
                }
            }
            while(*p&&*p!=']')p++;
            if(*p)p++;
        }
        else {
            p=skip_value(p);
        }

        free(key);
        p=skip_ws(p); if(*p==',')p++;
    }

    free(json);
    return 0;
}

void session_load_free(SessionLoadResult *r){
    free(r->editor_text);
    for(int i=0;i<r->history_count;i++){
        free((void*)r->history[i].code);
        free((void*)r->history[i].error);
        free((void*)r->history[i].audio);
    }
    free(r->history);
    memset(r,0,sizeof(*r));
}
