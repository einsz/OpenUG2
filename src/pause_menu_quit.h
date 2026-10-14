/* Pause quit confirmation, QUIT_DIALOG.md. The dialog is the original authored generic dialog
 * package (IG_GenericDialog_SMALL, loaded at run time through the asset loader) driven by a small
 * state machine that owns focus, input and result. The state machine needs no resources, so input
 * and results keep working when the package or any of its art is unavailable; only the picture is
 * omitted then (never replaced by an application-made panel, Q1.3). */
#ifndef OPENUG2_PAUSE_MENU_QUIT_H
#define OPENUG2_PAUSE_MENU_QUIT_H
#include "pause_menu_layout.h"
#include "ugui/ugui_scene.h"
#include "ugui/ugui_assets.h"
#include <stdio.h>
#include <string.h>

/* String-table keys, Q4.1-Q4.3. The English text always comes from the loaded table. */
#define PQD_KEY_MESSAGE   0x30ED2368u   /* "Quit to Main Menu?" (single-line profile, Q4.2) */
#define PQD_KEY_YES       0x0000F770u
#define PQD_KEY_YES_ALT   0x88579775u   /* equivalent alternates, used only if the primary key is absent */
#define PQD_KEY_NO        0x0000061Cu
#define PQD_KEY_NO_ALT    0xBE503981u
#define PQD_KEY_CAPTION   0xC7E1420Cu   /* pause item caption, Q4.3 */
#define PQD_PACKAGE_KEY   0x191A579Cu   /* IG_GenericDialog_SMALL.fng, Q2.1 */
#define PQD_PACKAGE_NAME  "IG_GenericDialog_SMALL.fng"

typedef enum { PQD_NONE, PQD_CONFIRM, PQD_CANCEL } PqdResult;
typedef enum { PQD_KEY_LEFT, PQD_KEY_RIGHT, PQD_KEY_ACCEPT, PQD_KEY_BACK } PqdKey;

/* Package-local identities, Q3: guid is the unique key; hash/parent/type/resource must agree. */
typedef struct { uint32_t hash,guid,parent; int type,resource; } PqdIdentity;
enum { PQD_ID_LEFT_BTN,PQD_ID_RIGHT_BTN,PQD_ID_LEFT_LBL,PQD_ID_RIGHT_LBL,PQD_ID_MESSAGE,
       PQD_ID_UNUSED_FIRST,PQD_ID_COUNT=PQD_ID_UNUSED_FIRST+9 };
static const PqdIdentity pqd_identities[PQD_ID_COUNT]={
    {0xB8A7C6CCu,0x000DE8CEu,0,5,0},        /* left button (positive) */
    {0xB8A7C6CDu,0x000DE86Au,0,5,0},        /* right button (negative) */
    {0xDA473D2Fu,0x000DE6E8u,0,2,5},        /* left label */
    {0xDA473D30u,0x000DE6F0u,0,2,5},        /* right label */
    {0x1E2640FAu,0x000DE6D6u,0,2,5},        /* message text */
    /* never shown in the two-button quit dialog (Q3): title, icon, single-button set, off-screen objects */
    {0x426C7B4Du,0x000DEF80u,0x000DEF6Eu,2,5},
    {0xC26A8D5Cu,0x000DEF81u,0x000DEF6Eu,1,6},
    {0xDA473D31u,0x000DE993u,0,2,5},
    {0xB8A7C6CEu,0x000DE8ECu,0,5,0},
    {0x47FF4E7Cu,0x000DE076u,0,2,5},
    {0xF7206332u,0x000DF018u,0,2,5},
    {0xB62648B1u,0x000DDFC1u,0,1,11},
    {0xB62648B1u,0x000DDFC0u,0,1,11},
    {0xB1B1EAB0u,0x000DCD32u,0,5,0},
};

typedef struct {
    int open;                 /* logically open: owns all input */
    int focus;                /* 0 = left (positive, "Yes"), 1 = right (negative, "No") */
    int press;                /* primary press origin: -1 none, 0/1 button, 2 elsewhere */
    PqdResult result;         /* outcome of the most recent close */
    int opens,confirms,cancels;
    int have_pkg,loaded,visual;
    float ms;
    const char *text[3];      /* message, left label, right label (string-table entries) or NULL */
    char status[220],load_status[220],caption_status[160];
    UgFngPackage pkg;
    UgScene scene;
    int node[PQD_ID_COUNT];
} PauseQuitDialog;

/* Visible button extents, top-left virtual units, edges inclusive (Q5.5). */
static inline int pqd_button_at(float x,float y) {
    for(int b=0;b<2;b++) {
        float e[4];pause_quit_button_edges(b,e);
        if(x>=e[0] && x<=e[2] && y>=e[1] && y<=e[3])return b;
    }
    return -1;
}

static void pqd_set_status(PauseQuitDialog *q,const char *msg) {snprintf(q->status,sizeof q->status,"%s",msg);}
/* Resolves every identity by unique guid; scene node k corresponds to package object k. */
static int pqd_resolve(const UgFngPackage *pk,int *index) {
    for(int k=0;k<PQD_ID_COUNT;k++) {
        const PqdIdentity *id=&pqd_identities[k];
        index[k]=-1;
        for(int i=0;i<pk->nobj;i++) {
            const UgFngObject *o=&pk->obj[i];
            if(o->guid!=id->guid)continue;
            if(index[k]>=0 || o->name_hash!=id->hash || o->parent_guid!=id->parent ||
               o->type!=(uint32_t)id->type || o->res_index!=id->resource)return 0;
            index[k]=i;
        }
        if(index[k]<0)return 0;
    }
    return 1;
}
static void pqd_init(PauseQuitDialog *q) {memset(q,0,sizeof *q);q->press=-1;q->focus=1;}
static void pqd_drop_scene(PauseQuitDialog *q) {
    if(q->loaded)ug_scene_free(&q->scene);
    q->loaded=0;
}
static void pqd_free(PauseQuitDialog *q) {
    pqd_drop_scene(q);
    if(q->have_pkg)ug_fng_free(&q->pkg);
    q->have_pkg=0;
}
/* Parses the authored package from the loaded in-game resources. Returns 1 when usable. */
static int pqd_load(PauseQuitDialog *q,const UgAssets *assets) {
    const UgAssetPackage *pk=ug_assets_package_by_key(assets,PQD_PACKAGE_KEY);
    if(!pk)pk=ug_assets_package(assets,PQD_PACKAGE_NAME);
    q->load_status[0]=0;
    if(!pk){snprintf(q->load_status,sizeof q->load_status,"quit dialog unavailable: " PQD_PACKAGE_NAME " not found in the in-game resources");return 0;}
    char err[120]="";
    if(!ug_fng_parse(pk->data,pk->len,&q->pkg,err,sizeof err)) {
        snprintf(q->load_status,sizeof q->load_status,"quit dialog unavailable: package unreadable (%s)",err);return 0;
    }
    int index[PQD_ID_COUNT];
    if(!pqd_resolve(&q->pkg,index)) {
        ug_fng_free(&q->pkg);
        snprintf(q->load_status,sizeof q->load_status,"quit dialog unavailable: reviewed object identities not uniquely resolved");return 0;
    }
    q->have_pkg=1;return 1;
}

static const char *pqd_lookup(const UgStrTab *t,uint32_t key,uint32_t alt) {
    const char *s=t?ug_strtab_get(t,key):NULL;
    if(!s && alt)s=ug_strtab_get(t,alt);
    return s;
}
/* Q4.2: the three displayed texts. Missing entries stay NULL; placeholders are never shown. */
static int pqd_resolve_text(PauseQuitDialog *q,const UgStrTab *t) {
    q->text[0]=pqd_lookup(t,PQD_KEY_MESSAGE,0);
    q->text[1]=pqd_lookup(t,PQD_KEY_YES,PQD_KEY_YES_ALT);
    q->text[2]=pqd_lookup(t,PQD_KEY_NO,PQD_KEY_NO_ALT);
    return q->text[0] && q->text[1] && q->text[2];
}

/* The focused button is mirrored into the scene so the authored focus responses (button alpha,
 * label pulse and the authored guards) run as data (Q5.6). */
static void pqd_set_focus(PauseQuitDialog *q,int focus) {
    if(focus==q->focus)return;
    q->focus=focus;
    if(q->loaded)ug_scene_set_focus(&q->scene,q->node[focus?PQD_ID_RIGHT_BTN:PQD_ID_LEFT_BTN]);
}

/* Builds the authored presentation for a fresh open; returns 1 when it can be drawn. */
static int pqd_build_scene(PauseQuitDialog *q,const UgAssets *assets,const UgStrTab *strings) {
    if(!q->have_pkg)return 0;
    if(!pqd_resolve(&q->pkg,q->node)){pqd_set_status(q,"quit dialog unavailable: reviewed object identities not uniquely resolved");return 0;}
    if(!ug_scene_init(&q->scene,&q->pkg,strings)){pqd_set_status(q,"quit dialog unavailable: scene allocation failed");return 0;}
    q->loaded=1;
    UgScene *s=&q->scene;
    ug_scene_set_assets(s,assets);
    s->pc_build=1;s->hide_hint_text=0;s->console_strip=0;
    for(int k=PQD_ID_UNUSED_FIRST;k<PQD_ID_COUNT;k++)s->nodes[q->node[k]].app_hidden=1;
    /* Placeholder strings never display; the table text replaces them (Q3.1). */
    s->nodes[q->node[PQD_ID_MESSAGE]].text_override=q->text[0];
    s->nodes[q->node[PQD_ID_LEFT_LBL]].text_override=q->text[1];
    s->nodes[q->node[PQD_ID_RIGHT_LBL]].text_override=q->text[2];
    /* Q6.1 order: initial focus at creation (right button), then the enter broadcast on the first update. */
    ug_scene_set_focus(s,q->node[PQD_ID_RIGHT_BTN]);
    ug_scene_update(s,0);
    int fonts_ok=1;
    for(int k=PQD_ID_LEFT_LBL;k<=PQD_ID_MESSAGE;k++)if(!s->nodes[q->node[k]].face)fonts_ok=0;
    if(!fonts_ok){pqd_set_status(q,"quit dialog unavailable: authored dialog font missing");return 0;}
    int missing=ug_scene_missing_textures(s);
    if(missing>0) {
        snprintf(q->status,sizeof q->status,"quit dialog unavailable: %d authored dialog texture(s) missing",missing);return 0;
    }
    return 1;
}

/* Opens a fresh dialog (Q7.6): focus on the negative button, no stale press, restarted presentation. */
static void pqd_open(PauseQuitDialog *q,const UgAssets *assets,int allow_visual) {
    pqd_drop_scene(q);
    q->open=1;q->focus=1;q->press=-1;q->ms=0;q->result=PQD_NONE;q->opens++;q->visual=0;q->status[0]=0;
    const UgStrTab *strings=assets && assets->strings.n>0?&assets->strings:NULL;
    int text_ok=pqd_resolve_text(q,strings);
    if(!allow_visual)pqd_set_status(q,"quit dialog picture unavailable: pause resources unavailable");
    else if(!q->have_pkg)pqd_set_status(q,q->load_status[0]?q->load_status:"quit dialog unavailable: package not loaded");
    else if(!text_ok)pqd_set_status(q,"quit dialog unavailable: string-table text missing");
    else q->visual=pqd_build_scene(q,assets,strings);
    if(!q->visual){
        if(q->loaded)pqd_drop_scene(q);
        if(!q->status[0])pqd_set_status(q,"quit dialog unavailable");
        fprintf(stderr,"pause: %s; input handling continues without the picture\n",q->status);
    }
}
static PqdResult pqd_close(PauseQuitDialog *q,PqdResult r) {
    q->open=0;q->press=-1;q->result=r;
    q->confirms+=r==PQD_CONFIRM;q->cancels+=r==PQD_CANCEL;
    pqd_drop_scene(q);          /* instant removal: no exit presentation (Q6.2a) */
    return r;
}

/* Keys, controller buttons (already mapped by the caller). Up/Down, wheel and Start never get here. */
static PqdResult pqd_key(PauseQuitDialog *q,PqdKey key) {
    if(!q->open)return PQD_NONE;
    switch(key) {
    case PQD_KEY_LEFT: pqd_set_focus(q,0);return PQD_NONE;      /* no wrap: no-op at the left button */
    case PQD_KEY_RIGHT: pqd_set_focus(q,1);return PQD_NONE;
    case PQD_KEY_ACCEPT: return pqd_close(q,q->focus?PQD_CANCEL:PQD_CONFIRM);
    case PQD_KEY_BACK: return pqd_close(q,PQD_CANCEL);
    }
    return PQD_NONE;
}
/* Pointer movement (Q5.5): focus follows a hovered button; leaving keeps the focus. */
static PqdResult pqd_pointer_move(PauseQuitDialog *q,float x,float y) {
    if(!q->open)return PQD_NONE;
    int b=pqd_button_at(x,y);
    if(b>=0)pqd_set_focus(q,b);
    return PQD_NONE;
}
/* Primary button: a press never acts; only a release inside the button the press began on does. */
static PqdResult pqd_primary_press(PauseQuitDialog *q,float x,float y) {
    if(!q->open)return PQD_NONE;
    int b=pqd_button_at(x,y);
    q->press=b>=0?b:2;
    return PQD_NONE;
}
static PqdResult pqd_primary_release(PauseQuitDialog *q,float x,float y) {
    if(!q->open)return PQD_NONE;
    int began=q->press;q->press=-1;
    if(began<0 || began>1 || pqd_button_at(x,y)!=began)return PQD_NONE;
    return pqd_close(q,began?PQD_CANCEL:PQD_CONFIRM);
}
/* Secondary button anywhere cancels on the press; its release is ignored by the pause screen. */
static PqdResult pqd_secondary_press(PauseQuitDialog *q) {
    return q->open?pqd_close(q,PQD_CANCEL):PQD_NONE;
}

static void pqd_update(PauseQuitDialog *q,float ms) {
    if(!q->open)return;
    q->ms+=ms;
    if(q->loaded) {
        ug_scene_update(&q->scene,ms);
        uint32_t msg;
        while(ug_scene_poll_app(&q->scene,&msg)){} /* presentation events only; results never wait for them (Q6.2a) */
    }
}
/* Draws in centred virtual coordinates on top of what is already on the canvas. A failing draw
 * drops only the dialog's own geometry and reports the picture unavailable. */
static void pqd_draw(PauseQuitDialog *q,UgCtx *ctx,UgCanvas *cv) {
    if(!q->open || !q->loaded || !q->visual)return;
    int mark=cv->n;
    ug_scene_draw(&q->scene,ctx,cv);
    if(ug_cv_visual_status(cv)!=UG_VIS_OK) {
        cv->n=mark;cv->visual_status=UG_VIS_OK;q->visual=0;
        pqd_set_status(q,"quit dialog unavailable: authored resources could not be drawn");
        fprintf(stderr,"pause: %s\n",q->status);
    }
}
#endif
