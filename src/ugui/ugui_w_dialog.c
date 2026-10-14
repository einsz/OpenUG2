/* ugui_w_dialog.c -- modal dialog (SPEC 6.9) and help popup (SPEC 6.10). */
#include "ugui_ui.h"
#include "ugui_tuning.h"
#include <stdio.h>
#include <string.h>

/* SPEC 6.9 (amendment 3, R). Per variant: the dark panel (outer x -297..+297), the light inner panel
 * (x -290..+290), the line centres of the 2-unit border, the button row and the message block centre.
 * SMALL and LARGE were measured on captures; MED follows the same offsets from its authored extents. */
typedef struct {
    float top, bottom, light_top, light_bottom, btn_y, msg_y;
    float bl, br, bt, bb;                  /* border line centres: left, right, top, bottom */
} DlgGeom;
static const DlgGeom DLG_GEOM[3] = {
    {-91, 107, -49.4f, 98.6f, 73.7f, -3.0f, -297.0f, 298.0f, -91.6f, 109.0f},              /* SMALL (R) */
    {-144, 140, -105.4f, 132.6f, 107.0f, UG_TUNE_DLG_MSG_Y_MED, -297.5f, 297.8f, -144.4f, 141.2f},   /* MED (derived, G) */
    {-214, 199, -173.4f, 190.6f, 165.1f, -33.0f, -298.0f, 297.6f, -214.2f, 199.4f},        /* LARGE (R) */
};

void ug_dialog_geom(int size, int nbtn, int has_icon, UgDialogGeom *o) {
    if (size < 0) size = 0;
    if (size > 2) size = 2;
    if (nbtn < 1) nbtn = 1;
    if (nbtn > 3) nbtn = 3;
    const DlgGeom *g = &DLG_GEOM[size];
    memset(o, 0, sizeof *o);
    o->panel = (UgRect){-297, g->top, 594, g->bottom - g->top};
    o->light = (UgRect){-290, g->light_top, 580, g->light_bottom - g->light_top};
    o->border = (UgRect){g->bl, g->bt, g->br - g->bl, g->bb - g->bt};
    o->btn_cy = g->btn_y;
    o->nbtn = nbtn;
    float hw = UG_R_DLG_BTN_HW, cx[3] = {0, 0, 0};
    if (nbtn == 2) { cx[0] = -UG_R_DLG_BTN_DX; cx[1] = UG_R_DLG_BTN_DX; }
    else if (nbtn == 3) {                  /* the data has no three-button geometry in the captures (G) */
        hw = UG_TUNE_DLG3_BUTTON_W * 0.5f;
        cx[0] = -UG_TUNE_DLG3_BUTTON_PITCH; cx[1] = 0; cx[2] = UG_TUNE_DLG3_BUTTON_PITCH;
    }
    for (int i = 0; i < nbtn; i++) o->btn[i] = UG_RECT_C(cx[i], g->btn_y, 2 * hw, UG_R_DLG_BTN_H);
    o->msg_cx = 0; o->msg_cy = g->msg_y;
    /* title: accent 0.75, left aligned. Without an icon it starts at the light panel's left inner edge
     * (ink left -289); with one it sits 30 units further right next to the 32 x 32 icon (I) */
    o->title_x = has_icon ? UG_R_DLG_TITLE_INK_LEFT + 30.0f : UG_R_DLG_TITLE_INK_LEFT;
    o->title_cy = g->top + 23.5f;          /* cap top = top + 16 (LARGE: -198) */
    o->icon_cx = UG_R_DLG_TITLE_INK_LEFT + 16.0f; o->icon_cy = o->title_cy;
}

static void dialog_bg(UgScreen *s, UgCtx *c, UgCanvas *cv) {
    UgDialog *d = (UgDialog *)s->user;
    UgDialogGeom g;
    ug_dialog_geom(d->size, d->nbtn, d->icon >= 0, &g);
    /* full-screen darkening quad (about 1070 x 650, dark box at full alpha = 60% black): everything beneath,
     * including the 3D background, ends up at 0.40 of its brightness (SPEC 6.9, R) */
    ug_kit_flat(cv, UG_RECT_C(0, 0, 1070, 650), UG_RGBA(0, 0, 0, 153));
    /* the dark panel is nearly opaque inside the border (R): the data stacks several black 60% layers */
    for (int k = 0; k < 4; k++) ug_kit_panel(cv, g.panel, 0, 0.98f);
    ug_kit_panel(cv, g.light, 1, 0.75f);
    ug_cv_outline_centered(cv, g.border, UG_BORDER, 16.0f, 2.0f);
    if (d->title[0]) {
        if (d->icon >= 0) ug_kit_symbol(cv, d->icon, g.icon_cx, g.icon_cy, 28, UG_ACCENT);
        ug_kit_text(c, cv, d->title, g.title_x, g.title_cy, UG_R_DLG_TITLE_SCALE, UG_ACCENT, UG_J_VCENTER);
    }
    ug_text_draw(cv, d->message, g.msg_cx, g.msg_cy, 0.75f, UG_TEXT_LIGHT, UG_J_CENTER | UG_J_VCENTER,
                 720.0f, UG_R_DLG_LEADING, c->glyphs);
}

static int dialog_action(UgScreen *s, UgCtx *c, UgAction a) {
    UgDialog *d = (UgDialog *)s->user;
    if (a == UG_A_BACK) {
        ug_cue(c, UG_CUE_BACK);
        ug_post(c, UG_EV_DIALOG, d->id, d->back_result, 0);
        return 1;
    }
    if (a == UG_A_LEFT || a == UG_A_RIGHT) {   /* explicit links between buttons */
        int idx = s->focus;
        int to = idx + (a == UG_A_LEFT ? -1 : 1);
        if (idx >= 0 && to >= 0 && to < d->nbtn) ug_screen_set_focus(s, c, to);
        return 1;
    }
    return 0;
}

void ug_dialog_init(UgDialog *d, int id, int size, const char *title, const char *message,
                    const char *const *buttons, int nbtn) {
    memset(d, 0, sizeof *d);
    ug_screen_init(&d->scr, title, UG_SCR_MODAL);
    d->scr.user = d;
    d->scr.draw_bg = dialog_bg;
    d->scr.on_action = dialog_action;
    d->id = id;
    d->size = size < 0 ? 0 : (size > 2 ? 2 : size);
    d->nbtn = nbtn < 1 ? 1 : (nbtn > 3 ? 3 : nbtn);
    d->back_result = d->nbtn - 1;
    d->icon = -1;
    snprintf(d->title, sizeof d->title, "%s", title);
    snprintf(d->message, sizeof d->message, "%s", message);
    UgDialogGeom g;
    ug_dialog_geom(d->size, d->nbtn, 0, &g);
    d->panel = g.panel;
    for (int i = 0; i < d->nbtn; i++) {
        ug_button_init(&d->btn[i], i, UG_BTN_DIALOG, g.btn[i], buttons[i]);
        d->btn[i].post_kind = UG_EV_DIALOG;
        d->btn[i].post_source = id;
        ug_screen_add(&d->scr, &d->btn[i].w);
    }
    /* SPEC 6.9 (R): the right-hand, negative button has focus when the dialog opens */
    d->scr.default_focus = d->nbtn - 1;
}

/* ------------------------------------------------------------------ */
static void help_bg(UgScreen *s, UgCtx *c, UgCanvas *cv) {
    UgHelpPopup *h = (UgHelpPopup *)s->user;
    ug_kit_flat(cv, UG_RECT_C(0, 0, 1070, 650), UG_RGBA(0, 0, 0, 153));
    UgRect p = h->panel;
    ug_kit_panel(cv, p, 0, 0.98f);
    ug_cv_outline(cv, p, UG_BORDER, 16, 2.0f);
    ug_kit_text(c, cv, "Help", p.x + 24, p.y + 22, 0.85f, UG_ACCENT, UG_J_VCENTER);
    ug_kit_box(cv, (UgRect){p.x + 14, p.y + 44, p.w - 28, p.h - 44 - 46}, 1, 0.12f);
    for (int i = 0; i < h->nlines; i++)
        ug_kit_text(c, cv, h->lines[i], p.x + 34, p.y + 62 + (float)i * 25.0f, 0.7f, UG_TEXT_LIGHT, UG_J_VCENTER);
}

static int help_action(UgScreen *s, UgCtx *c, UgAction a) {
    UgHelpPopup *h = (UgHelpPopup *)s->user;
    if (a == UG_A_BACK || a == UG_A_ACCEPT) {
        ug_cue(c, UG_CUE_BACK);
        ug_post(c, UG_EV_DIALOG, h->back.post_source, 0, 0);
        return 1;
    }
    return 1;      /* the popup is modal: swallow everything else */
}

void ug_help_init(UgHelpPopup *h, int id, const char *const *lines, int n) {
    memset(h, 0, sizeof *h);
    ug_screen_init(&h->scr, "Help", UG_SCR_MODAL);
    h->scr.user = h;
    h->scr.draw_bg = help_bg;
    h->scr.on_action = help_action;
    h->nlines = n > 10 ? 10 : n;
    for (int i = 0; i < h->nlines; i++) h->lines[i] = lines[i];
    float ph = 44 + 46 + 25.0f * (float)h->nlines + 24;
    h->panel = UG_RECT_C(0, 0, 562, ph);
    ug_button_init(&h->back, 0, UG_BTN_HINT,
                   (UgRect){h->panel.x + h->panel.w - 120, h->panel.y + h->panel.h - 34, 100, 24},
                   "$JOY_EVENT_FENG_CANCEL$ Back");
    h->back.post_kind = UG_EV_DIALOG;
    h->back.post_source = id;
    ug_screen_add(&h->scr, &h->back.w);
}
