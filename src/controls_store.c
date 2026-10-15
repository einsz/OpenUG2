/* controls_store.c -- see controls_store.h. CONTROLS_SPEC 6.6, decisions D3/D4. */
#include "controls_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL_scancode.h>

#define KB_VERSION "1:"

int cs_kb_save(StRegStore *s, const CbKeyTable *t) {
    char buf[CB_KB_ENTRIES * 5 + 8], *w = buf;
    w += snprintf(w, sizeof buf, KB_VERSION);
    for (int i = 0; i < CB_KB_ENTRIES; i++)
        w += snprintf(w, (size_t)(buf + sizeof buf - w), i ? ",%d" : "%d", t->e[i].key);
    return st_reg_set_string(s, CS_KB_VALUE, buf);
}

int cs_kb_load(StRegStore *s, CbKeyTable *t) {
    char buf[CB_KB_ENTRIES * 5 + 8];
    if (!st_reg_get_string(s, CS_KB_VALUE, buf, sizeof buf)) return 0;
    if (strncmp(buf, KB_VERSION, strlen(KB_VERSION))) return 0;
    int keys[CB_KB_ENTRIES];
    const char *r = buf + strlen(KB_VERSION);
    for (int i = 0; i < CB_KB_ENTRIES; i++) {
        char *end;
        long v = strtol(r, &end, 10);
        if (end == r || (i < CB_KB_ENTRIES - 1 ? *end != ',' : *end != 0)) return 0;
        keys[i] = v > 0 && v < SDL_NUM_SCANCODES ? (int)v : 0;
        r = end + 1;
    }
    for (int i = 0; i < CB_KB_ENTRIES; i++)
        for (int j = 0; j < i; j++) if (keys[i] && keys[i] == keys[j]) keys[i] = 0;
    for (int i = 0; i < CB_KB_ENTRIES; i++) t->e[i].key = (int16_t)keys[i];
    return 1;
}

static int guid_zero(const uint8_t g[16]) {
    for (int i = 0; i < 16; i++) if (g[i]) return 0;
    return 1;
}

/* Index of the n-th record (from slot 1) holding `guid`, or -1. */
static int nth_record(const StProfile *p, const uint8_t guid[16], int n) {
    for (int i = 1; i < ST_INPUT_SLOTS; i++)
        if (!memcmp(p->input[i].guid, guid, 16) && n-- == 0) return i;
    return -1;
}
static int dup_index(const CbBindings *b, int pad) {
    int n = 0;
    for (int i = 0; i < pad; i++)
        if (b->pad[i].present && !memcmp(b->pad[i].guid, b->pad[pad].guid, 16)) n++;
    return n;
}

void cs_pads_store(StProfile *p, const CbBindings *b) {
    for (int i = 0; i < CB_MAX_PADS; i++) {
        const CbPad *pad = &b->pad[i];
        if (!pad->present || guid_zero(pad->guid)) continue;
        int slot = nth_record(p, pad->guid, dup_index(b, i));
        for (int k = 1; slot < 0 && k < ST_INPUT_SLOTS; k++) if (guid_zero(p->input[k].guid)) slot = k;
        if (slot < 0) continue;                                    /* no free record: not persisted */
        p->input[slot] = pad->dev;
        memcpy(p->input[slot].guid, pad->guid, 16);
    }
}

static int record_valid(const StInputDevice *d) {
    for (int r = 0; r < 4; r++) {
        if (d->axis[r] < 0 || d->axis[r] > 4 * (CB_AXIS_COUNT - 1) || d->axis[r] % 4) return 0;
        if (d->dir[r] != 1 && d->dir[r] != -1) return 0;
        if (d->assigned[r] > 1) return 0;
    }
    if (d->combined_mode > 1) return 0;
    for (int j = 0; j < 32; j++) if (d->binding[j] < 0 || d->binding[j] > 62) return 0;
    return 1;
}

int cs_pads_load(const StProfile *p, CbBindings *b) {
    int taken = 0;
    for (int i = 0; i < CB_MAX_PADS; i++) {
        CbPad *pad = &b->pad[i];
        if (!pad->present || guid_zero(pad->guid)) continue;
        int slot = nth_record(p, pad->guid, dup_index(b, i));
        if (slot < 0 || !record_valid(&p->input[slot])) continue;
        pad->dev = p->input[slot];
        memcpy(pad->dev.guid, pad->guid, 16);
        taken++;
    }
    return taken;
}
