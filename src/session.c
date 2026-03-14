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

/* Returns heap-allocated base64 string. Caller must free(). */
static char *b64_encode(const unsigned char *src, size_t len) {
    size_t out_len = 4 * ((len + 2) / 3) + 1;
    char *out = malloc(out_len);
    if (!out) return NULL;
    size_t i = 0, j = 0;
    while (i + 2 < len) {
        out[j++] = B64[(src[i]   >> 2)];
        out[j++] = B64[((src[i]   & 3) << 4) | (src[i+1] >> 4)];
        out[j++] = B64[((src[i+1] & 0xf) << 2) | (src[i+2] >> 6)];
        out[j++] = B64[(src[i+2] & 0x3f)];
        i += 3;
    }
    if (i < len) {
        out[j++] = B64[(src[i] >> 2)];
        if (i + 1 < len) {
            out[j++] = B64[((src[i] & 3) << 4) | (src[i+1] >> 4)];
            out[j++] = B64[((src[i+1] & 0xf) << 2)];
        } else {
            out[j++] = B64[((src[i] & 3) << 4)];
            out[j++] = '=';
        }
        out[j++] = '=';
    }
    out[j] = '\0';
    return out;
}

static int b64_val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

/* Decode base64 into a newly malloc'd buffer. Returns byte count or -1. */
static long b64_decode(const char *src, size_t src_len,
                       unsigned char **out_buf) {
    size_t out_len = (src_len / 4) * 3 + 4;
    unsigned char *out = malloc(out_len);
    if (!out) return -1;
    size_t i = 0, j = 0;
    while (i + 3 < src_len) {
        int a = b64_val(src[i]);
        int b = b64_val(src[i+1]);
        int c = b64_val(src[i+2]);
        int d = b64_val(src[i+3]);
        if (a<0||b<0) break;
        out[j++] = (unsigned char)((a<<2)|(b>>4));
        if (src[i+2]!='=' && c>=0) out[j++] = (unsigned char)((b<<4)|(c>>2));
        if (src[i+3]!='=' && d>=0) out[j++] = (unsigned char)((c<<6)|d);
        i += 4;
    }
    *out_buf = out;
    return (long)j;
}

/* ── simple JSON string escape ────────────────────────────────────── */
static void write_json_str(FILE *f, const char *s) {
    fputc('"', f);
    for (; *s; s++) {
        if      (*s == '"')  fputs("\\\"", f);
        else if (*s == '\\') fputs("\\\\", f);
        else if (*s == '\n') fputs("\\n",  f);
        else if (*s == '\r') fputs("\\r",  f);
        else if (*s == '\t') fputs("\\t",  f);
        else                 fputc(*s, f);
    }
    fputc('"', f);
}

/* ── save ─────────────────────────────────────────────────────────── */

int session_save(const char *path,
                 const int *pad_slots,
                 const int *pad_semitones,
                 int        num_pads) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;

    const Slot *slots = engine_slots();

    fprintf(f, "{\n");
    fprintf(f, "  \"magic\": \"%s\",\n", MAGIC);
    fprintf(f, "  \"saved\": \"desktop\",\n");

    /* slots */
    fprintf(f, "  \"slots\": [\n");
    for (int i = 0; i < ENGINE_NUM_SLOTS; i++) {
        const Slot *s = slots + i;
        fprintf(f, "    {");
        fprintf(f, "\"label\": "); write_json_str(f, s->label);
        fprintf(f, ", \"baseRate\": %g", (double)s->base_rate);
        if (s->samples && s->len > 0) {
            char *b64 = b64_encode((const unsigned char*)s->samples,
                                   (size_t)s->len * sizeof(float));
            if (b64) {
                fprintf(f, ", \"audio\": \"%s\"", b64);
                free(b64);
            }
        } else {
            fprintf(f, ", \"audio\": null");
        }
        fprintf(f, "}%s\n", i < ENGINE_NUM_SLOTS - 1 ? "," : "");
    }
    fprintf(f, "  ],\n");

    /* pads */
    fprintf(f, "  \"pads\": [\n");
    for (int i = 0; i < num_pads; i++) {
        fprintf(f, "    {\"slot\": %d, \"semitones\": %d}%s\n",
                pad_slots[i], pad_semitones[i],
                i < num_pads - 1 ? "," : "");
    }
    fprintf(f, "  ],\n");

    /* history: empty — the desktop doesn't persist notebook cells in sessions */
    fprintf(f, "  \"history\": []\n");
    fprintf(f, "}\n");

    fclose(f);
    return 0;
}

/* ── minimal JSON parser (just enough for our format) ─────────────── */

static const char *skip_ws(const char *p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return p;
}

/* Read a quoted JSON string into buf (max buf_len-1 chars). Returns ptr after closing quote. */
static const char *read_str(const char *p, char *buf, int buf_len) {
    p = skip_ws(p);
    if (*p != '"') return p;
    p++;
    int i = 0;
    while (*p && *p != '"') {
        if (*p == '\\') {
            p++;
            switch (*p) {
            case '"':  if (i<buf_len-1) buf[i++]='"';  break;
            case '\\': if (i<buf_len-1) buf[i++]='\\'; break;
            case 'n':  if (i<buf_len-1) buf[i++]='\n'; break;
            case 'r':  if (i<buf_len-1) buf[i++]='\r'; break;
            case 't':  if (i<buf_len-1) buf[i++]='\t'; break;
            default:   if (i<buf_len-1) buf[i++]=*p;   break;
            }
        } else {
            if (i < buf_len-1) buf[i++] = *p;
        }
        p++;
    }
    buf[i] = '\0';
    if (*p == '"') p++;
    return p;
}

/* Find the value after a key in a JSON object. p points into the object body. */
static const char *find_key(const char *obj, const char *key) {
    const char *p = obj;
    while (*p) {
        p = skip_ws(p);
        if (*p != '"') { p++; continue; }
        char found_key[128];
        p = read_str(p, found_key, sizeof(found_key));
        p = skip_ws(p);
        if (*p == ':') p++;
        p = skip_ws(p);
        if (strcmp(found_key, key) == 0) return p;
        /* skip value */
        if (*p == '"') {
            p++;
            while (*p && *p != '"') { if (*p=='\\') p++; p++; }
            if (*p) p++;
        } else if (*p == '{' || *p == '[') {
            int depth = 1; char open=*p, close=(*p=='{')?'}':']';
            p++;
            while (*p && depth) {
                if (*p==open) depth++;
                else if (*p==close) depth--;
                else if (*p=='"') { p++; while(*p&&*p!='"'){if(*p=='\\')p++;p++;} }
                p++;
            }
        } else {
            while (*p && *p!=',' && *p!='}' && *p!=']') p++;
        }
        p = skip_ws(p);
        if (*p == ',') p++;
    }
    return NULL;
}

/* ── load ─────────────────────────────────────────────────────────── */

int session_load(const char *path,
                 int *pad_slots,
                 int *pad_semitones,
                 int  num_pads,
                 char *errmsg,
                 int   errmsg_len) {
    FILE *f = fopen(path, "r");
    if (!f) { snprintf(errmsg, errmsg_len, "Cannot open: %s", path); return -1; }

    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char *json = malloc((size_t)sz + 1);
    if (!json) { fclose(f); snprintf(errmsg, errmsg_len, "out of memory"); return -1; }
    fread(json, 1, (size_t)sz, f);
    json[sz] = '\0';
    fclose(f);

    /* Check magic */
    const char *magic_p = find_key(json, "magic");
    if (!magic_p) { free(json); snprintf(errmsg, errmsg_len, "missing magic"); return -1; }
    char magic_val[64];
    read_str(magic_p, magic_val, sizeof(magic_val));
    if (strcmp(magic_val, MAGIC) != 0) {
        free(json);
        snprintf(errmsg, errmsg_len, "not a ksynth session file");
        return -1;
    }

    /* Find slots array */
    const char *slots_p = find_key(json, "slots");
    if (slots_p) {
        slots_p = skip_ws(slots_p);
        if (*slots_p == '[') {
            slots_p++;
            for (int i = 0; i < ENGINE_NUM_SLOTS; i++) {
                slots_p = skip_ws(slots_p);
                if (*slots_p != '{') break;
                /* Find end of this object */
                const char *obj_start = slots_p;
                int depth = 1; slots_p++;
                while (*slots_p && depth) {
                    if (*slots_p == '{') depth++;
                    else if (*slots_p == '}') depth--;
                    else if (*slots_p == '"') {
                        slots_p++;
                        while (*slots_p && *slots_p != '"')
                            { if (*slots_p=='\\') slots_p++; slots_p++; }
                    }
                    if (depth > 0) slots_p++;
                }
                /* obj_start..slots_p is the object */
                char obj_buf[1024*1024];  /* 1MB per slot should be enough for label+baseRate */
                size_t obj_len = (size_t)(slots_p - obj_start + 1);
                /* audio can be large — work with pointers into json directly */

                /* label */
                char label[64] = "";
                const char *lp = find_key(obj_start, "label");
                if (lp) read_str(lp, label, sizeof(label));

                /* baseRate */
                float base_rate = 1.0f;
                const char *rp = find_key(obj_start, "baseRate");
                if (rp) base_rate = (float)atof(rp);

                /* audio */
                const char *ap = find_key(obj_start, "audio");
                if (ap) {
                    ap = skip_ws(ap);
                    if (*ap == '"') {
                        ap++;
                        /* find end of b64 string */
                        const char *ae = ap;
                        while (*ae && *ae != '"') ae++;
                        size_t b64_len = (size_t)(ae - ap);
                        unsigned char *bytes = NULL;
                        long byte_count = b64_decode(ap, b64_len, &bytes);
                        if (byte_count > 0 && (byte_count % sizeof(float)) == 0) {
                            int n_samples = (int)(byte_count / sizeof(float));
                            float *samples = malloc((size_t)byte_count);
                            if (samples) {
                                memcpy(samples, bytes, (size_t)byte_count);
                                engine_bank_slot(i, samples, n_samples, label);
                                engine_set_slot_base_rate(i, base_rate);
                                free(samples);
                            }
                        }
                        free(bytes);
                    } else {
                        /* null — clear the slot */
                        engine_clear_slot(i);
                    }
                }
                (void)obj_buf; (void)obj_len;

                slots_p = skip_ws(slots_p);
                if (*slots_p == ',') slots_p++;
            }
        }
    }

    /* Find pads array */
    const char *pads_p = find_key(json, "pads");
    if (pads_p) {
        pads_p = skip_ws(pads_p);
        if (*pads_p == '[') {
            pads_p++;
            for (int i = 0; i < num_pads; i++) {
                pads_p = skip_ws(pads_p);
                if (*pads_p != '{') break;
                const char *obj_s = pads_p;
                int depth = 1; pads_p++;
                while (*pads_p && depth) {
                    if (*pads_p=='{') depth++;
                    else if (*pads_p=='}') depth--;
                    else if (*pads_p=='"') {
                        pads_p++;
                        while (*pads_p&&*pads_p!='"'){if(*pads_p=='\\')pads_p++;pads_p++;}
                    }
                    if (depth>0) pads_p++;
                }
                const char *sp = find_key(obj_s, "slot");
                if (sp) pad_slots[i] = atoi(sp);
                const char *stp = find_key(obj_s, "semitones");
                if (stp) pad_semitones[i] = atoi(stp);
                pads_p = skip_ws(pads_p);
                if (*pads_p == ',') pads_p++;
            }
        }
    }

    free(json);
    return 0;
}
