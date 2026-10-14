/* Pause SPEC 2-5: runtime native art over the host's paused driving scene. */
#include "pause_menu.h"
#include "pause_menu_layout.h"
#include "pause_menu_binding.h"
#include "pause_menu_quit.h"
#include "options_menu.h"
#include "ugui/ugui_gl.h"
#include "ugui/ugui_assets.h"
#include "ugui/ugui_ui.h"
#include "ugui/ugui_scene.h"
#include "ugui/ugui_tuning.h"
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

struct PauseMenu {
    Fe state;
    int open, original;
    int item_tex[FE_MENU_MAX];
    int arrow_tex, marker_tex;
    UgAssets assets;
    UgSkin skin;
    UgGL *gl;
    UgCanvas canvas;
    UgFngPackage package;
    UgScene scene;
    UgCtx ctx;
    int loaded, compatible;
    float ui_ms, marker_x;
    float scroll;                /* continuous item-row position in item units, BEHAVIOR 1.5 */
    int press_center;            /* primary press began on the centred item, BEHAVIOR 4.1 */
    int blinks_posted;           /* arrow blink messages sent (verification counter) */
    int last_blink_dir;          /* direction of the latest blink message, -1/0/+1 (verification) */
    int capture_actions, capture_last_action;
    int slot, caption, marker, left, right;
    int roles[PAUSE_ROLE_COUNT], bound;
    int pool[PAUSE_POOL_COUNT], npool;
    int pill_hover, pill_initialized;
    UgAnim pill_pulse,pill_glow;
    PauseQuitDialog quit;        /* authored quit confirmation, QUIT_DIALOG.md */
    OptionsMenu options;         /* graphics Display/Customize screens, SPEC_GUI */
    StGraphics *opt_gfx; const StGfxCaps *opt_caps; StRegStore *opt_store;
    int options_enabled;         /* host lent the settings: the row gains an Options entry */
    char status[256];
};

/* Application action artwork, BINDING 3. Package request ids are not texture keys. */
static int pause_texture(UgAssets *a,uint32_t key) {
    int i=ug_texlib_find(&a->tex,key);
    return i>=0 && ug_texlib_pixels(&a->tex,i)?i+1:0;
}
static void pause_resolve_art(PauseMenu *p) {
    p->item_tex[0]=pause_texture(&p->assets,0x62AA0DF6u);
    p->item_tex[1]=pause_texture(&p->assets,0x4C9E34E6u);
    p->arrow_tex=pause_texture(&p->assets,0x9943D584u);
    p->marker_tex=pause_texture(&p->assets,0xDA10B4A1u);
}

/* Icon of entry k. Without the Options entry art follows the position (Resume, Quit); with it the art
 * follows the action. The Options entry has no authored icon yet, so its slot shows the caption only. */
static int pause_entry_art(const PauseMenu *p,int k) {
    if(p->options_enabled && k>=0 && k<p->state.entry_count) {
        FeAction a=p->state.entries[k].action;
        if(a==FE_ACTION_OPTIONS)return 0;
        if(a==FE_ACTION_QUIT)return p->item_tex[1];
        if(a==FE_ACTION_RESUME)return p->item_tex[0];
    }
    return k>=0 && k<FE_MENU_MAX?p->item_tex[k]:0;
}
static void pause_bind_apply(PauseMenu *p);
static int pause_bind_detect(PauseMenu *p);
static int pause_scene_open(PauseMenu *p) {
    if(p->loaded){ug_scene_free(&p->scene);p->loaded=0;}
    if(!p->package.nobj)return 0;
    if(!ug_scene_init(&p->scene,&p->package,&p->assets.strings))return 0;
    p->loaded=1;
    ug_scene_set_assets(&p->scene,&p->assets);
    p->scene.pc_build=1;p->scene.hide_hint_text=1;p->scene.console_strip=0;
    ug_scene_update(&p->scene,0);
    if(!pause_bind_detect(p)) {
        snprintf(p->status,sizeof p->status,"authored action binding unavailable: reviewed object hash/guid metadata not uniquely resolved");
        ug_scene_free(&p->scene);p->loaded=0;return 0;
    }
    pause_bind_apply(p);
    return 1;
}
PauseMenu *pause_menu_create(const char *dataroot) {
    PauseMenu *p=calloc(1,sizeof *p);
    if(!p)return NULL;
    ug_cv_init(&p->canvas);ug_ctx_init(&p->ctx);pqd_init(&p->quit);
    p->assets.main_face=-1;
    if(ug_assets_open_ingame(&p->assets,dataroot,NULL,p->status,sizeof p->status)) {
        pause_resolve_art(p);ug_skin_resolve(&p->skin,&p->assets);
        pqd_load(&p->quit,&p->assets);
        const UgAssetPackage *pk=ug_assets_package_by_key(&p->assets,0x32124083u);
        if(!pk)pk=ug_assets_package(&p->assets,"UI_pause.fng");
        if(pk && ug_fng_parse(pk->data,pk->len,&p->package,p->status,sizeof p->status)) {
            const UgFace *face=ug_assets_main_face(&p->assets);
            p->compatible=face && face->tex>0 && p->item_tex[0] && p->item_tex[1] && p->skin.pill && p->skin.pill_glow;
            if(!p->compatible)snprintf(p->status,sizeof p->status,"required Explore/Quit artwork, genuine PC pill/glow or font unavailable");
            if(p->compatible) { p->gl=ug_gl_create();if(p->gl)ug_gl_set_texlib(p->gl,&p->assets.tex); }
        } else if(!pk)snprintf(p->status,sizeof p->status,"UI_pause.fng package missing");
    }
    if(!p->compatible || !p->gl)
        fprintf(stderr,"pause: compatibility resources unavailable; authored presentation omitted (%s)\n",p->status);
    return p;
}
void pause_menu_destroy(PauseMenu *p) {
    if(!p)return;
    if(p->loaded)ug_scene_free(&p->scene);
    pqd_free(&p->quit);
    ug_fng_free(&p->package);ug_gl_destroy(p->gl);ug_cv_free(&p->canvas);
    if(p->pill_initialized){ug_anim_free(&p->pill_pulse);ug_anim_free(&p->pill_glow);}
    ug_assets_close(&p->assets);free(p);
}
void pause_menu_open(PauseMenu *p) {
    if(!p)return;
    fe_pause_init(&p->state);p->open=1;
    om_close(&p->options);
    if(p->options_enabled) {
        static const FeMenuEntry row[]={{"Resume",FE_ACTION_RESUME,1},{"Options",FE_ACTION_OPTIONS,1},{"Quit Game",FE_ACTION_QUIT,1}};
        fe_set_entries(&p->state,row,3);
    }
    if(p->quit.open)pqd_close(&p->quit,PQD_NONE);
    p->ui_ms=0;
    if(p->pill_initialized){ug_anim_free(&p->pill_pulse);ug_anim_free(&p->pill_glow);}
    ug_anim_init(&p->pill_pulse,UG_WHITE,0,0);ug_anim_init(&p->pill_glow,UG_WHITE,0,0);
    p->pill_initialized=1;p->pill_hover=-1;
    p->marker_x=50;p->press_center=0;p->blinks_posted=0;p->last_blink_dir=0;
    p->scroll=(float)p->state.selected;
    p->original=p->compatible && p->gl && pause_scene_open(p);
    if(p->compatible && !p->original)fprintf(stderr,"pause: %s\n",p->status);
    p->quit.caption_status[0]=0;
    if(p->assets.strings.n>0 && !ug_strtab_get(&p->assets.strings,PQD_KEY_CAPTION)) {
        /* Q4.3: report; the host's own label stays rather than an invented text. */
        snprintf(p->quit.caption_status,sizeof p->quit.caption_status,"quit caption unavailable: string table lacks the pause quit caption key");
        fprintf(stderr,"pause: %s\n",p->quit.caption_status);
    }
}
int pause_menu_is_open(const PauseMenu *p){return p && p->open;}
void pause_menu_set_options(PauseMenu *p,StGraphics *gfx,const StGfxCaps *caps,StRegStore *store) {
    if(!p)return;
    p->opt_gfx=gfx;p->opt_caps=caps;p->opt_store=store;
    p->options_enabled=gfx && caps;
}
int pause_menu_options_open(const PauseMenu *p){return p && om_is_open(&p->options);}
OptionsMenu *pause_menu_options(PauseMenu *p){return p?&p->options:NULL;}
int pause_menu_visual_available(const PauseMenu *p){return p && p->original;}
const char *pause_menu_status(const PauseMenu *p){return p?p->status:"pause menu allocation unavailable";}
int pause_menu_quit_dialog_open(const PauseMenu *p){return p && p->quit.open;}
int pause_menu_quit_dialog_visual_available(const PauseMenu *p){return p && p->quit.open && p->quit.visual;}
const char *pause_menu_quit_dialog_status(const PauseMenu *p){return p?p->quit.status:"pause menu allocation unavailable";}
const char *pause_menu_caption_status(const PauseMenu *p){return p?p->quit.caption_status:"pause menu allocation unavailable";}
static float pause_marker_target(const PauseMenu *p) {
    int rank=0,n=0;
    for(int i=0;i<p->state.entry_count;i++)if(p->state.entries[i].enabled) {
        if(i<p->state.selected)rank++;
        n++;
    }
    return n>1?50+546.0f*rank/(n-1):50;
}
/* BEHAVIOR 3.1: the row never wraps. Returns the enabled entry one step away in `dir`, or -1. */
static int pause_step_target(const PauseMenu *p,int dir) {
    if(p->state.screen!=FE_SCREEN_PAUSE)return -1;
    for(int k=p->state.selected+dir;k>=0 && k<p->state.entry_count;k+=dir)
        if(p->state.entries[k].enabled)return k;
    return -1;
}
static int pause_can_step(const PauseMenu *p,int dir) {return pause_step_target(p,dir)>=0;}
/* Selection is logically current at the input frame (BEHAVIOR 1.8): the marker snaps (2.1) and
 * the arrows switch (3.3) here; only the icon row keeps sliding in pause_menu_update. */
static void pause_selection_changed(PauseMenu *p,int before,int direction) {
    if(p->state.selected==before)return;
    p->marker_x=pause_marker_target(p);
    if(p->loaded) {
        /* Hidden wins over blink (3.6): an arrow with nothing beyond it never starts one. */
        if(pause_can_step(p,direction)) {
            /* Package directional events explicitly target the arrow, BINDING 5. */
            ug_scene_post(&p->scene,direction<0?0x9120409Eu:0xB5971BF1u,
                          direction<0?0x000DE06Du:0x000DE087u);
            p->blinks_posted++;p->last_blink_dir=direction;
        }
        pause_bind_apply(p);
    }
}
/* QUIT_DIALOG: the quit item opens the authored confirmation instead of Fe's application-made one.
 * The Back/Continue pills leave input and drawing for as long as it is open (Q7.3). */
static int pause_selected_is_quit(const Fe *f) {
    return f->screen==FE_SCREEN_PAUSE && f->selected>=0 && f->selected<f->entry_count &&
           f->entries[f->selected].enabled && f->entries[f->selected].action==FE_ACTION_QUIT;
}
static void pause_pill_clear(PauseMenu *p) {
    p->pill_hover=-1;
    if(p->pill_initialized){ug_anim_stop(&p->pill_pulse);ug_anim_stop(&p->pill_glow);}
}
static void pause_quit_begin(PauseMenu *p) {
    p->press_center=0;pause_pill_clear(p);
    pqd_open(&p->quit,&p->assets,p->original);
}
/* Confirm requests the host's existing quit action on the input frame, with no exit to wait for
 * (Q6.2a, Q7.5). Cancel only closes the dialog: the pause state is untouched and still paused. */
static void pause_quit_result(PauseMenu *p,PqdResult r) {
    if(r==PQD_CONFIRM)p->state.pending=FE_ACTION_QUIT;
}
static void pause_dispatch(PauseMenu *p,FeInput in) {
    if(p->state.pending!=FE_ACTION_NONE)return;
    if(p->quit.open) { /* Up/Down never reach the dialog (Q5.4) */
        if(in==FE_INPUT_CONFIRM)pause_quit_result(p,pqd_key(&p->quit,PQD_KEY_ACCEPT));
        else if(in==FE_INPUT_BACK)pause_quit_result(p,pqd_key(&p->quit,PQD_KEY_BACK));
        return;
    }
    if(in==FE_INPUT_CONFIRM && pause_selected_is_quit(&p->state)){pause_quit_begin(p);return;}
    if(p->state.screen==FE_SCREEN_PAUSE && (in==FE_INPUT_UP || in==FE_INPUT_DOWN)) {
        int direction=in==FE_INPUT_UP?-1:1,target=pause_step_target(p,direction);
        if(target<0)return; /* end of the row: no change of any kind */
        int before=p->state.selected;
        p->state.selected=target;pause_selection_changed(p,before,direction);
        return;
    }
    fe_input(&p->state,in);
}
/* Caption of an entry: resume is fixed; the quit item shows the string-table caption when it exists
 * (Q4.3); otherwise the host's own label stays. */
static const char *pause_entry_caption(const UgStrTab *strings,const FeMenuEntry *e) {
    if(e->action==FE_ACTION_RESUME)return "Return to Explore Mode";
    if(e->action==FE_ACTION_QUIT) {
        const char *t=strings?ug_strtab_get(strings,PQD_KEY_CAPTION):NULL;
        if(t)return t;
    }
    return e->label;
}
const char *pause_menu_caption(const PauseMenu *p) {
    if(!p || p->state.selected<0 || p->state.selected>=p->state.entry_count)return "";
    return pause_entry_caption(&p->assets.strings,&p->state.entries[p->state.selected]);
}

/* Pointer input in top-left virtual units, BEHAVIOR 4. The Back/Continue pills and the quit
 * confirmation keep their press-time behaviour. A press on a visible navigation arrow steps one
 * item at press-down exactly like Left/Right (R4.7-R4.10); the release has no effect and the press
 * never arms centred activation. The centred item activates on release, and only when the press
 * began on it. Neighbours, hidden arrows, rail, marker and empty space are inert (R4.4). */
static int pause_in_center(float x,float y) {return ug_rect_hit(pause_menu_center_hit(),x,y);}
static void pause_pointer_press(PauseMenu *p,float x,float y) {
    p->press_center=0;
    if(!pause_menu_is_open(p) || p->state.pending!=FE_ACTION_NONE)return;
    if(p->quit.open){pause_quit_result(p,pqd_primary_press(&p->quit,x,y));return;} /* modal (Q5.5) */
    if(x<0 || x>=640 || y<0 || y>=480)return;
    FeInput in=FE_INPUT_NONE;
    if(p->state.screen==FE_SCREEN_PAUSE && pause_menu_arrow_at(x,y)) {
        /* Visibility is judged on the logical selection at this frame (R4.9): pause_dispatch makes a
         * blocked step a no-op, which is exactly the hidden-arrow case. */
        pause_dispatch(p,pause_menu_arrow_at(x,y)<0?FE_INPUT_UP:FE_INPUT_DOWN);
        return;
    } else if(ug_rect_hit(pause_menu_bounds(PAUSE_BACK),x,y))in=FE_INPUT_BACK;
    else if(ug_rect_hit(pause_menu_bounds(PAUSE_CONTINUE),x,y))in=FE_INPUT_CONFIRM;
    else {p->press_center=p->state.screen==FE_SCREEN_PAUSE && pause_in_center(x,y);return;}
    pause_dispatch(p,in);
}
static void pause_pointer_release(PauseMenu *p,float x,float y) {
    int armed=p->press_center;p->press_center=0;
    if(!pause_menu_is_open(p) || p->state.pending!=FE_ACTION_NONE)return;
    if(p->quit.open){pause_quit_result(p,pqd_primary_release(&p->quit,x,y));return;}
    if(!armed)return;
    if(p->state.screen==FE_SCREEN_PAUSE && pause_in_center(x,y))pause_dispatch(p,FE_INPUT_CONFIRM);
}
/* Complete primary click at a window point (press and release together). */
void pause_menu_pointer(PauseMenu *p,float px,float py,int w,int h) {
    if(!pause_menu_is_open(p) || w<=0 || h<=0)return;
    float x,y;
    pause_menu_to_virtual(pause_menu_transform(w,h),px,py,&x,&y);
    if(om_is_open(&p->options)){om_pointer_click(&p->options,x-320,y-240);return;}
    pause_pointer_press(p,x,y);pause_pointer_release(p,x,y);
}

/* PC pills use the existing shared focus/glow animations (base 5/6.8, pause 3). */
static void pause_pill_hover(PauseMenu *p,float x,float y) {
    int hover=-1;
    if(p->quit.open){pause_quit_result(p,pqd_pointer_move(&p->quit,x,y));return;} /* pills are gone, Q7.3 */
    if(p->state.screen==FE_SCREEN_PAUSE) {
        if(ug_rect_hit(pause_menu_bounds(PAUSE_BACK),x,y))hover=0;
        else if(ug_rect_hit(pause_menu_bounds(PAUSE_CONTINUE),x,y))hover=1;
    }
    if(hover==p->pill_hover)return;
    p->pill_hover=hover;
    if(hover>=0) {ug_anim_play(&p->pill_pulse,UG_ANIM_PILL_FOCUS,0);ug_anim_play(&p->pill_glow,UG_ANIM_PILL_GLOW,0);}
    else {ug_anim_stop(&p->pill_pulse);ug_anim_stop(&p->pill_glow);}
}

/* Window events while the quit dialog is open (Q5.4-Q5.5). Everything is consumed: nothing reaches the
 * pause screen or the game. Only Left/Right, Accept, Back, hover and the two mouse buttons act; Up/Down,
 * wheel, Start and key auto-repeat do nothing. */
static void pause_quit_event(PauseMenu *p,const SDL_Event *e,SDL_Window *win) {
    PqdResult r=PQD_NONE;
    if(e->type==SDL_KEYDOWN && !e->key.repeat) {
        switch(e->key.keysym.sym) {
        case SDLK_LEFT: r=pqd_key(&p->quit,PQD_KEY_LEFT);break;
        case SDLK_RIGHT: r=pqd_key(&p->quit,PQD_KEY_RIGHT);break;
        case SDLK_RETURN: case SDLK_KP_ENTER: r=pqd_key(&p->quit,PQD_KEY_ACCEPT);break;
        case SDLK_ESCAPE: r=pqd_key(&p->quit,PQD_KEY_BACK);break;
        default:break;
        }
    } else if(e->type==SDL_CONTROLLERBUTTONDOWN) {
        switch(e->cbutton.button) {
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: r=pqd_key(&p->quit,PQD_KEY_LEFT);break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: r=pqd_key(&p->quit,PQD_KEY_RIGHT);break;
        case SDL_CONTROLLER_BUTTON_A: r=pqd_key(&p->quit,PQD_KEY_ACCEPT);break;
        case SDL_CONTROLLER_BUTTON_B: case SDL_CONTROLLER_BUTTON_BACK: r=pqd_key(&p->quit,PQD_KEY_BACK);break;
        default:break;
        }
    } else if(e->type==SDL_MOUSEBUTTONDOWN && e->button.button==SDL_BUTTON_RIGHT)r=pqd_secondary_press(&p->quit);
    else if((e->type==SDL_MOUSEBUTTONDOWN || e->type==SDL_MOUSEBUTTONUP) && e->button.button==SDL_BUTTON_LEFT && win) {
        int w,h;SDL_GetWindowSize(win,&w,&h);
        if(w<=0 || h<=0)return;
        float x,y;pause_menu_to_virtual(pause_menu_transform(w,h),e->button.x,e->button.y,&x,&y);
        r=e->type==SDL_MOUSEBUTTONDOWN?pqd_primary_press(&p->quit,x,y):pqd_primary_release(&p->quit,x,y);
    } else if(e->type==SDL_MOUSEMOTION && win) {
        int w,h;SDL_GetWindowSize(win,&w,&h);
        if(w<=0 || h<=0)return;
        float x,y;pause_menu_to_virtual(pause_menu_transform(w,h),e->motion.x,e->motion.y,&x,&y);
        r=pqd_pointer_move(&p->quit,x,y);
    }
    pause_quit_result(p,r);
}
/* Options screens (SPEC_GUI 2.7): Left/Right/Up/Down, Enter/A accept, Esc/B/right click Back, controller
 * Start = Start. A primary click goes to the list. Wheel and hover are unspecified (SPEC_GUI 11.2) and
 * ignored. Everything is consumed while the screens are open. */
static void pause_options_event(PauseMenu *p,const SDL_Event *e,SDL_Window *win) {
    OmInput in=OM_IN_NONE;
    if(e->type==SDL_KEYDOWN && !e->key.repeat) {
        switch(e->key.keysym.sym) {
        case SDLK_LEFT: in=OM_IN_LEFT;break;
        case SDLK_RIGHT: in=OM_IN_RIGHT;break;
        case SDLK_UP: in=OM_IN_UP;break;
        case SDLK_DOWN: in=OM_IN_DOWN;break;
        case SDLK_RETURN: case SDLK_KP_ENTER: in=OM_IN_ACCEPT;break;
        case SDLK_ESCAPE: in=OM_IN_BACK;break;
        default:break;
        }
    } else if(e->type==SDL_CONTROLLERBUTTONDOWN) {
        switch(e->cbutton.button) {
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: in=OM_IN_LEFT;break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: in=OM_IN_RIGHT;break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: in=OM_IN_UP;break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: in=OM_IN_DOWN;break;
        case SDL_CONTROLLER_BUTTON_A: in=OM_IN_ACCEPT;break;
        case SDL_CONTROLLER_BUTTON_B: case SDL_CONTROLLER_BUTTON_BACK: in=OM_IN_BACK;break;
        case SDL_CONTROLLER_BUTTON_START: in=OM_IN_START;break;
        default:break;
        }
    } else if(e->type==SDL_MOUSEBUTTONDOWN && e->button.button==SDL_BUTTON_RIGHT)in=OM_IN_BACK;
    else if(e->type==SDL_MOUSEBUTTONDOWN && e->button.button==SDL_BUTTON_LEFT && win) {
        int w,h;SDL_GetWindowSize(win,&w,&h);
        if(w<=0 || h<=0)return;
        float x,y;pause_menu_to_virtual(pause_menu_transform(w,h),e->button.x,e->button.y,&x,&y);
        om_pointer_click(&p->options,x-320,y-240);
        return;
    }
    om_input(&p->options,in);
}
void pause_menu_event(PauseMenu *p,const SDL_Event *e,SDL_Window *win) {
    if(!pause_menu_is_open(p) || p->state.pending!=FE_ACTION_NONE)return;
    if(p->quit.open){pause_quit_event(p,e,win);return;}
    if(om_is_open(&p->options)){pause_options_event(p,e,win);return;}
    FeInput in=FE_INPUT_NONE;
    if(e->type==SDL_KEYDOWN && !e->key.repeat) {
        switch(e->key.keysym.sym) {
        case SDLK_LEFT: case SDLK_UP: in=FE_INPUT_UP;break;
        case SDLK_RIGHT: case SDLK_DOWN: in=FE_INPUT_DOWN;break;
        case SDLK_RETURN: case SDLK_KP_ENTER: in=FE_INPUT_CONFIRM;break;
        case SDLK_ESCAPE: in=FE_INPUT_BACK;break;
        default:break;
        }
    } else if(e->type==SDL_CONTROLLERBUTTONDOWN) {
        switch(e->cbutton.button) {
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: case SDL_CONTROLLER_BUTTON_DPAD_UP: in=FE_INPUT_UP;break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: case SDL_CONTROLLER_BUTTON_DPAD_DOWN: in=FE_INPUT_DOWN;break;
        case SDL_CONTROLLER_BUTTON_A: in=FE_INPUT_CONFIRM;break;
        case SDL_CONTROLLER_BUTTON_B: case SDL_CONTROLLER_BUTTON_START: in=FE_INPUT_BACK;break;
        default:break;
        }
    } else if(e->type==SDL_MOUSEBUTTONDOWN && e->button.button==SDL_BUTTON_RIGHT)in=FE_INPUT_BACK;
    else if(e->type==SDL_MOUSEWHEEL && e->wheel.y) {
        int d=e->wheel.direction==SDL_MOUSEWHEEL_FLIPPED?-e->wheel.y:e->wheel.y;
        in=d>0?FE_INPUT_UP:FE_INPUT_DOWN;
    } else if((e->type==SDL_MOUSEBUTTONDOWN || e->type==SDL_MOUSEBUTTONUP) && e->button.button==SDL_BUTTON_LEFT && win) {
        int w,h;SDL_GetWindowSize(win,&w,&h);
        if(w<=0 || h<=0)return;
        float x,y;pause_menu_to_virtual(pause_menu_transform(w,h),e->button.x,e->button.y,&x,&y);
        if(e->type==SDL_MOUSEBUTTONDOWN)pause_pointer_press(p,x,y);else pause_pointer_release(p,x,y);
        return;
    } else if(e->type==SDL_MOUSEMOTION && win) {
        int w,h;SDL_GetWindowSize(win,&w,&h);
        if(w>0 && h>0) {
            float x,y;pause_menu_to_virtual(pause_menu_transform(w,h),e->motion.x,e->motion.y,&x,&y);
            pause_pill_hover(p,x,y); /* hover never changes selection, art or sound (BEHAVIOR 4.5) */
        }
        return;
    }
    pause_dispatch(p,in);
}
FeAction pause_menu_update(PauseMenu *p,float seconds) {
    if(!pause_menu_is_open(p))return FE_ACTION_NONE;
    float ms=fmaxf(0,seconds*1000);
    p->ui_ms+=ms;p->ctx.time_ms=p->ui_ms;
    ug_anim_update(&p->pill_pulse,ms);ug_anim_update(&p->pill_glow,ms);
    p->scroll=pause_scroll_advance(p->scroll,(float)p->state.selected,ms);
    pqd_update(&p->quit,ms); /* dialog animation runs on UI time while the game stays paused (Q6.5) */
    if(p->loaded) {
        ug_scene_update(&p->scene,ms);
        uint32_t msg;
        while(ug_scene_poll_app(&p->scene,&msg))
            if(msg==UG_MSG_ENTER_DONE)ug_scene_refocus(&p->scene);
        /* Script events are presentation events. Fe alone dispatches actions. */
        pause_bind_apply(p);
    }
    fe_update(&p->state,seconds);
    FeAction a=fe_poll_action(&p->state);
    if(a==FE_ACTION_OPTIONS) {
        /* Opens the graphics screens inside the pause; the game stays paused and nothing is reported. */
        a=FE_ACTION_NONE;
        if(p->options_enabled) {
            OmConfig cfg;om_config_default(&cfg,OM_CTX_INGAME);
            om_open(&p->options,&cfg,p->opt_gfx,p->opt_caps,p->opt_store,0);
        }
    }
    if(om_is_open(&p->options) || p->options.nevents) {
        om_update(&p->options,seconds);
        OmEvent oe;
        while((oe=om_poll_event(&p->options))!=OM_EV_NONE)
            if(oe==OM_EV_RESUME_GAME)a=FE_ACTION_RESUME; /* in-game Start: leave at once, nothing written (SPEC_GUI 5.2) */
    }
    /* BEHAVIOR 5.3 / QUIT_DIALOG Q7.5: resume and a confirmed quit both leave at the input frame. The
     * authored fade-outs are presentation only and nothing waits for them or for a timer. */
    if(a==FE_ACTION_RESUME || a==FE_ACTION_QUIT) {
        p->open=0;p->press_center=0;
        if(p->quit.open)pqd_close(&p->quit,PQD_NONE);
    }
    return a;
}

typedef struct { float x0,y0,x1,y1; int count; const UgFace *face; const uint8_t *pixels; int width,height; } PauseInk;
static void ink_extent(void *user,const UgQuad *q) {
    PauseInk *b=user;
    const UgFGlyph *g=ug_face_glyph(b->face,q->cp);
    if(!g || !b->pixels)return;
    int x0=g->w,y0=g->h,x1=0,y1=0;
    for(int y=0;y<g->h;y++)for(int x=0;x<g->w;x++) {
        int ax=g->x+x,ay=g->y+y;
        if(ax>=b->width || ay>=b->height || !b->pixels[((size_t)ay*b->width+ax)*4+3])continue;
        if(x<x0)x0=x;
        if(y<y0)y0=y;
        if(x+1>x1)x1=x+1;
        if(y+1>y1)y1=y+1;
    }
    if(x1<=x0 || y1<=y0)return;
    float left=q->x0+x0,top=q->y0+y0,right=q->x0+x1,bottom=q->y0+y1;
    if(!b->count++) { b->x0=left;b->y0=top;b->x1=right;b->y1=bottom;return; }
    b->x0=fminf(b->x0,left);b->y0=fminf(b->y0,top);
    b->x1=fmaxf(b->x1,right);b->y1=fmaxf(b->y1,bottom);
}
static void native_text(PauseMenu *p,const char *s,UgRect region,float max_scale,UgColor col,unsigned just) {
    UgCanvas *cv=&p->canvas;
    PauseInk b={0};
    const UgFace *face=ug_text_face();
    if(!face || face->tex<=0 || face->tex>p->assets.tex.n){cv->visual_status=UG_VIS_UNAVAILABLE;return;}
    const UgTexture *atlas=&p->assets.tex.t[face->tex-1];
    b.face=face;b.width=atlas->w;b.height=atlas->h;
    b.pixels=ug_texlib_pixels(&p->assets.tex,face->tex-1);
    ug_text_quads(face,s,1,ink_extent,&b,NULL);
    if(!b.count){cv->visual_status=UG_VIS_UNAVAILABLE;return;}
    float tw=b.x1-b.x0,th=b.y1-b.y0;
    float scale=max_scale;
    if(tw*scale>region.w && tw>0)scale=region.w/tw;
    if(th*scale>region.h && th>0)scale=region.h/th;
    float x=region.x-b.x0*scale;
    if(just&UG_J_CENTER)x+=(region.w-tw*scale)/2;
    float y=region.y+(region.h-th*scale)/2-b.y0*scale;
    ug_text_draw_ink(cv,s,x,y,scale,col,0,UG_GLYPHS_KEYS);
}
/* Fit visible alpha uniformly, emitting the entire native image including its
 * transparent margins. Section 2 measures ink, never the full image quad. */
static UgRect native_ink_rect(UgTexLib *lib,int tex,UgRect r) {
    if(tex<=0 || tex>lib->n)return (UgRect){0};
    const UgTexture *t=&lib->t[tex-1];
    const uint8_t *pixels=ug_texlib_pixels(lib,tex-1);
    if(!pixels || t->w<=0 || t->h<=0)return (UgRect){0};
    int x0=t->w,y0=t->h,x1=0,y1=0;
    for(int y=0;y<t->h;y++)for(int x=0;x<t->w;x++)if(pixels[((size_t)y*t->w+x)*4+3]) {
        if(x<x0)x0=x;
        if(y<y0)y0=y;
        if(x+1>x1)x1=x+1;
        if(y+1>y1)y1=y+1;
    }
    if(x1<=x0 || y1<=y0)return (UgRect){0};
    float s=fminf(r.w/(x1-x0),r.h/(y1-y0));
    return (UgRect){r.x+(r.w-(x1-x0)*s)/2-x0*s,r.y+(r.h-(y1-y0)*s)/2-y0*s,t->w*s,t->h*s};
}
static void native_icon(PauseMenu *p,UgRect ink,int tex,UgColor tint) {
    UgRect quad=native_ink_rect(&p->assets.tex,tex,ink);
    if(quad.w<=0 || quad.h<=0){p->canvas.visual_status=UG_VIS_UNAVAILABLE;return;}
    ug_cv_image_rect(&p->canvas,quad,tex,tint);
}
/* Genuine quarter-disc corners only at the outer edges of the stack. Native
 * radius 16 is uniformly reduced to 7, within SPEC 3's 4-10 transition range. */
static void native_stacked_surface(UgCanvas *cv,const UgSkin *skin,UgRect r,int top,UgColor tint) {
    float k=7,x1=r.x+r.w-k,y=top?r.y:r.y+r.h-k;
    ug_cv_image_rect(cv,(UgRect){r.x+k,r.y,r.w-2*k,r.h},skin->box_w,tint);
    ug_cv_image_rect(cv,(UgRect){r.x,top?r.y+k:r.y,k,r.h-k},skin->box_w,tint);
    ug_cv_image_rect(cv,(UgRect){x1,top?r.y+k:r.y,k,r.h-k},skin->box_w,tint);
    ug_cv_image(cv,r.x+k,top?y+k:y,r.x,top?y:y+k,0,0,1,1,skin->corner_w,tint);
    ug_cv_image(cv,x1,top?y+k:y,x1+k,top?y:y+k,0,0,1,1,skin->corner_w,tint);
}
/* Authored role binding uses raw hash/guid identities, never evaluated geometry. */
static int pause_bind_detect(PauseMenu *p) {
    p->bound=0;p->npool=0;
    for(int k=0;k<PAUSE_ROLE_COUNT;k++) {
        p->roles[k]=pause_identity_find(&p->scene,&pause_identities[k]);
        if(p->roles[k]<0)return 0;
    }
    for(int k=0;k<PAUSE_POOL_COUNT;k++) {
        p->pool[k]=pause_identity_find(&p->scene,&pause_pool_identities[k]);
        if(p->pool[k]<0)return 0;
        p->scene.nodes[p->pool[k]].app_hidden=1;p->npool++;
    }
    p->slot=p->roles[PAUSE_ROLE_SLOT];p->caption=p->roles[PAUSE_ROLE_CAPTION];
    p->marker=p->roles[PAUSE_ROLE_MARKER];p->left=p->roles[PAUSE_ROLE_LEFT];p->right=p->roles[PAUSE_ROLE_RIGHT];
    p->scene.nodes[p->slot].app_hidden=1;
    int hint=ug_scene_find_guid(&p->scene,0x000E24A1u);
    if(hint>=0)p->scene.nodes[hint].app_hidden=1;
    p->bound=p->state.entry_count<=p->npool;
    return p->bound;
}
/* Visible texel centre, including the native transparent-margin offset. */
static float pause_marker_offset_x(PauseMenu *p,const UgNode *n) {
    int tex=n->tex_override>0?n->tex_override:n->tex;
    if(tex<=0 || tex>p->assets.tex.n)return 0;
    UgTexture *t=&p->assets.tex.t[tex-1];const uint8_t *pixels=ug_texlib_pixels(&p->assets.tex,tex-1);
    if(!pixels || t->w<=0)return 0;
    int lo=t->w,hi=0;
    for(int y=0;y<t->h;y++)for(int x=0;x<t->w;x++)if(pixels[((size_t)y*t->w+x)*4+3]) {if(x<lo)lo=x;if(x+1>hi)hi=x+1;}
    if(hi<=lo)return 0;
    return n->wa*n->props.w[UG_P_SIZE]*((lo+hi)*.5f/t->w-.5f);
}
static void pause_bind_apply(PauseMenu *p) {
    UgScene *s=&p->scene;
    if(!p->loaded || !p->bound)return;
    const UgFngObject *slot=s->nodes[p->slot].obj;
    s->nodes[p->slot].app_hidden=1; /* hidden magenta authoring slot is never a displayed icon */
    s->nodes[p->caption].text_override=pause_menu_caption(p);
    s->nodes[p->roles[PAUSE_ROLE_HEADING]].text_override="Pause";
    int valid=p->state.selected>=0 && p->state.selected<p->state.entry_count;
    for(int k=0;k<p->npool;k++) {
        UgNode *n=&s->nodes[p->pool[k]];
        /* BEHAVIOR 1.5: every icon sits at the fractional scroll position; an icon is drawn
         * iff its opacity multiplier is positive (|k-s| < 1.8). Only the icons move. */
        PauseIconPlace ic=pause_icon_place(k,p->scroll);
        int listed=k<p->state.entry_count && k<FE_MENU_MAX;
        int art=listed?pause_entry_art(p,k):0;
        n->app_hidden=!valid || !listed || ic.alpha<=0 || !art;
        n->tex_override=art;
        n->app_opacity=ic.alpha;
        if(listed) {
            ug_scene_set_world_pos(s,n->index,slot->pos[0]+ic.dx,slot->pos[1]);
            n->props.w[UG_P_SIZE]=ic.w;n->props.w[UG_P_SIZE+1]=ic.h;
            /* Preserve evaluated color/fade/highlight; assignment controls only art/position/size/visibility. */
        }
    }
    s->nodes[p->roles[PAUSE_ROLE_BACKING]].app_hidden=!valid;
    s->nodes[p->roles[PAUSE_ROLE_GLOW]].app_hidden=!valid;
    if(valid && p->state.selected<p->npool && s->focus!=p->pool[p->state.selected])
        ug_scene_set_focus(s,p->pool[p->state.selected]);
    UgNode *marker=&s->nodes[p->marker];
    /* BEHAVIOR 2: the marker is never interpolated; it is always at the logical target. */
    ug_scene_set_world_pos(s,p->marker,p->marker_x-320-pause_marker_offset_x(p,marker),marker->wf);
    s->nodes[p->left].app_hidden=!pause_can_step(p,-1);
    s->nodes[p->right].app_hidden=!pause_can_step(p,1);
    ug_scene_relayout(s);
}
/* Q7.3: the Back/Continue pills are not drawn, hovered or clicked while the quit dialog is open. */
static int pause_pills_visible(const PauseMenu *p) {return p->state.screen==FE_SCREEN_PAUSE && !p->quit.open;}
static void pause_draw_pills(PauseMenu *p) {
    if(!p->bound || !pause_pills_visible(p))return;
    float alpha=p->scene.nodes[p->roles[PAUSE_ROLE_PRESENTATION]].wcolor.a/255.0f;
    for(int k=0;k<2;k++) {
        UgRect r=pause_menu_bounds(k?PAUSE_CONTINUE:PAUSE_BACK);
        UgColor tint=k==p->pill_hover?ug_anim_color(&p->pill_pulse):UG_RGBA(255,255,255,150);
        if(k==p->pill_hover)ug_kit_pill_glow(&p->canvas,r,ug_color_alpha(ug_anim_color(&p->pill_glow),alpha));
        ug_kit_pill_pc(&p->canvas,r,ug_color_alpha(tint,alpha));
        native_text(p,k?"Continue":"Back",pause_menu_label_ink(k),.45f,
                    ug_color_alpha(k==p->pill_hover?UG_ACCENT:UG_WHITE,alpha),UG_J_CENTER);
    }
}
static void pause_menu_compose(PauseMenu *p) {
    UgCanvas *cv=&p->canvas;ug_cv_clear(cv);
    if(!p->loaded){cv->visual_status=UG_VIS_UNAVAILABLE;return;}
    ug_scene_draw(&p->scene,&p->ctx,cv);
    if(ug_cv_visual_status(cv)!=UG_VIS_OK) {
        p->original=0;snprintf(p->status,sizeof p->status,"visible authored resources unavailable");
        ug_cv_clear(cv);cv->visual_status=UG_VIS_UNAVAILABLE;return;
    }
    pqd_draw(&p->quit,&p->ctx,cv); /* centred coordinates, above the darkened pause screen (Q7.1-Q7.2) */
    ug_cv_translate(cv,-320,-240);
    pause_draw_pills(p);
}
static void pause_options_draw(PauseMenu *p,int width,int height) {
    if(!p->gl || !p->compatible)return;
    const UgFace *face=ug_text_face();
    const UgSkin *old=ug_kit_skin();
    UgSkin saved;if(old)saved=*old;
    ug_text_set_face(ug_assets_main_face(&p->assets));ug_kit_set_skin(&p->skin);
    ug_cv_clear(&p->canvas);
    options_menu_draw(&p->options,&p->canvas);
    ug_gl_render_overlay(p->gl,&p->canvas,width,height);
    ug_text_set_face(face);ug_kit_set_skin(old?&saved:NULL);
}
void pause_menu_draw(PauseMenu *p,int width,int height) {
    if(pause_menu_is_open(p) && om_is_open(&p->options) && width>0 && height>0){pause_options_draw(p,width,height);return;}
    if(!pause_menu_is_open(p) || !p->original || width<=0 || height<=0)return;
    const UgFace *face=ug_text_face();
    const UgSkin *old=ug_kit_skin();
    UgSkin saved;if(old)saved=*old;
    ug_text_set_face(ug_assets_main_face(&p->assets));ug_kit_set_skin(&p->skin);
    pause_menu_compose(p);
    if(ug_cv_visual_status(&p->canvas)!=UG_VIS_OK){p->original=0;snprintf(p->status,sizeof p->status,"shared PC pill/font resources unavailable");}
    else ug_gl_render_overlay(p->gl,&p->canvas,width,height);
    ug_text_set_face(face);ug_kit_set_skin(old?&saved:NULL);
}

/* Verification adapter: advance only UI time, never host scene/camera state.
 * Replay syntax is comma-separated absolute times: 400:Right,480:Continue.
 * pointer (a complete click), press, release and hover take top-left virtual x:y; right_click is
 * the secondary button (Back). A press/pointer on a visible arrow steps (BEHAVIOR 4.7); rail_down/
 * rail_move/rail_up are accepted but inert (BEHAVIOR 4.4).
 * The normal gameplay adapter never calls these entry points. */
int pause_menu_capture_duration_valid(float elapsed_ms) {
    return isfinite(elapsed_ms) && elapsed_ms>=0 && elapsed_ms<=PAUSE_MENU_CAPTURE_MAX_MS;
}
static void pause_capture_tick(PauseMenu *p,float seconds) {
    FeAction a=pause_menu_update(p,seconds);
    if(a!=FE_ACTION_NONE){p->capture_actions++;p->capture_last_action=a;}
}
static void pause_capture_advance(PauseMenu *p,float ms) {
    while(ms>0) {float step=fminf(ms,10);pause_capture_tick(p,step*.001f);ms-=step;}
}
int pause_menu_capture_replay(PauseMenu *p,const char *events,float elapsed_ms,int w,int h) {
    if(!p || !pause_menu_capture_duration_valid(elapsed_ms) || w<=0 || h<=0)return 0;
    float previous=0;p->capture_actions=0;p->capture_last_action=FE_ACTION_NONE;
    char *copy=events?strdup(events):NULL,*save=NULL;
    if(events && !copy)return 0;
    for(char *item=copy?strtok_r(copy,",",&save):NULL;item;item=strtok_r(NULL,",",&save)) {
        float at,x=0,y=0;char name[32]={0};
        int fields=sscanf(item,"%f:%31[^:]:%f:%f",&at,name,&x,&y);
        if(fields<2 || !isfinite(at) || at<previous || at>elapsed_ms) {free(copy);return 0;}
        pause_capture_advance(p,at-previous);previous=at;
        if(!strcmp(name,"open"))pause_menu_open(p);
        else if(!strcmp(name,"Right")){if(p->quit.open)pause_quit_result(p,pqd_key(&p->quit,PQD_KEY_RIGHT));else pause_dispatch(p,FE_INPUT_DOWN);}
        else if(!strcmp(name,"Left")){if(p->quit.open)pause_quit_result(p,pqd_key(&p->quit,PQD_KEY_LEFT));else pause_dispatch(p,FE_INPUT_UP);}
        else if(!strcmp(name,"Enter") || !strcmp(name,"Continue"))pause_dispatch(p,FE_INPUT_CONFIRM);
        else if(!strcmp(name,"Back") || !strcmp(name,"Escape"))pause_dispatch(p,FE_INPUT_BACK);
        else if(!strcmp(name,"right_click"))pause_dispatch(p,FE_INPUT_BACK);
        else if(!strcmp(name,"rail_up") || (fields>=3 && (!strcmp(name,"rail_down") || !strcmp(name,"rail_move")))) {}
        else if(fields==4 && !strcmp(name,"pointer")) {
            PauseTransform t=pause_menu_transform(w,h);
            pause_menu_pointer(p,(float)(t.ox+x*t.scale),(float)(t.oy+y*t.scale),w,h);
        } else if(fields==4 && !strcmp(name,"press"))pause_pointer_press(p,x,y);
        else if(fields==4 && !strcmp(name,"release"))pause_pointer_release(p,x,y);
        else if(fields==4 && !strcmp(name,"hover"))pause_pill_hover(p,x,y);
        else {free(copy);return 0;}
        pause_capture_tick(p,0);
    }
    free(copy);pause_capture_advance(p,elapsed_ms-previous);
    fprintf(stderr,"pause capture: elapsed=%.3f ui=%.3f selected=%d marker_x=%.3f open=%d quit_dialog=%d actions=%d last_action=%d caption=%s status=%s\n",
            elapsed_ms,p->ui_ms,p->state.selected,p->marker_x,p->open,p->quit.open,p->capture_actions,p->capture_last_action,pause_menu_caption(p),p->status);
    return 1;
}
/* Separate reference evaluator. It shares only reviewed data identities, Fe's
 * existing supported action semantics, UgScene and native drawing primitives.
 * It never calls production detection/binding/update/input/capture functions. */
/* Independent no-wrap neighbour search for the reference evaluator. */
static int pause_reference_step(const PauseMenu *r,int direction) {
    if(r->state.screen!=FE_SCREEN_PAUSE)return -1;
    int k=r->state.selected+direction;
    while(k>=0 && k<r->state.entry_count && !r->state.entries[k].enabled)k+=direction;
    return k>=0 && k<r->state.entry_count?k:-1;
}
static void pause_reference_apply(PauseMenu *r) {
    UgScene *s=&r->scene;
    int selected=r->state.selected,valid=selected>=0 && selected<r->state.entry_count;
    s->nodes[r->slot].app_hidden=1;
    s->nodes[r->roles[PAUSE_ROLE_HEADING]].text_override="Pause";
    s->nodes[r->caption].text_override=valid?pause_entry_caption(&r->assets.strings,&r->state.entries[selected]):"";
    const UgFngObject *anchor=s->nodes[r->slot].obj;
    for(int i=0;i<PAUSE_POOL_COUNT;i++) {
        UgNode *n=&s->nodes[r->pool[i]];
        PauseIconPlace ic=pause_icon_place(i,r->scroll);
        int listed=i<r->state.entry_count && i<FE_MENU_MAX;
        n->app_hidden=!valid || !listed || ic.alpha<=0 || !r->item_tex[i];
        n->tex_override=listed?r->item_tex[i]:0;
        n->app_opacity=ic.alpha;
        if(listed) {
            ug_scene_set_world_pos(s,n->index,anchor->pos[0]+ic.dx,anchor->pos[1]);
            n->props.w[UG_P_SIZE]=ic.w;n->props.w[UG_P_SIZE+1]=ic.h;
        }
    }
    if(valid && s->focus!=r->pool[selected])ug_scene_set_focus(s,r->pool[selected]);
    s->nodes[r->roles[PAUSE_ROLE_BACKING]].app_hidden=!valid;
    s->nodes[r->roles[PAUSE_ROLE_GLOW]].app_hidden=!valid;
    UgNode *marker=&s->nodes[r->marker];
    float offset=pause_marker_offset_x(r,marker);
    /* BINDING 5: marker's authored tracks are color-only; reviewed D position is separate. */
    ug_scene_set_world_pos(s,r->marker,r->marker_x-320-offset,marker->wf);
    for(int direction=-1;direction<=1;direction+=2)
        s->nodes[direction<0?r->left:r->right].app_hidden=pause_reference_step(r,direction)<0;
    int hint=ug_scene_find_guid(s,0x000E24A1u);if(hint>=0)s->nodes[hint].app_hidden=1;
    ug_scene_relayout(s);
}
static int pause_reference_open(PauseMenu *r) {
    if(r->loaded){ug_scene_free(&r->scene);r->loaded=0;}
    if(r->pill_initialized){ug_anim_free(&r->pill_pulse);ug_anim_free(&r->pill_glow);}
    ug_anim_init(&r->pill_pulse,UG_WHITE,0,0);ug_anim_init(&r->pill_glow,UG_WHITE,0,0);
    r->pill_initialized=1;r->pill_hover=-1;
    if(!ug_scene_init(&r->scene,&r->package,&r->assets.strings))return 0;
    r->loaded=1;ug_scene_set_assets(&r->scene,&r->assets);
    r->scene.pc_build=1;r->scene.hide_hint_text=1;r->scene.console_strip=0;
    /* Direct raw identity lookup; no geometry classifier or production binder. */
    for(int k=0;k<PAUSE_ROLE_COUNT;k++) {
        r->roles[k]=pause_identity_find(&r->scene,&pause_identities[k]);
        if(r->roles[k]<0)return 0;
    }
    for(int k=0;k<PAUSE_POOL_COUNT;k++) {
        r->pool[k]=pause_identity_find(&r->scene,&pause_pool_identities[k]);
        if(r->pool[k]<0)return 0;
    }
    r->slot=r->roles[PAUSE_ROLE_SLOT];r->caption=r->roles[PAUSE_ROLE_CAPTION];
    r->marker=r->roles[PAUSE_ROLE_MARKER];r->left=r->roles[PAUSE_ROLE_LEFT];r->right=r->roles[PAUSE_ROLE_RIGHT];
    r->npool=PAUSE_POOL_COUNT;r->bound=1;fe_pause_init(&r->state);
    r->ui_ms=0;r->marker_x=50;r->scroll=0;r->press_center=0;
    pqd_drop_scene(&r->quit);r->quit.open=0;
    r->open=1;
    ug_scene_update(&r->scene,0);pause_reference_apply(r);return 1;
}
static void pause_reference_advance(PauseMenu *r,float milliseconds) {
    int first=1;
    while((first || milliseconds>0) && r->open) {
        first=0;
        float dt=fminf(milliseconds,10);milliseconds-=dt;r->ui_ms+=dt;r->ctx.time_ms=r->ui_ms;
        r->scroll=pause_scroll_advance(r->scroll,(float)r->state.selected,dt);
        ug_anim_update(&r->pill_pulse,dt);ug_anim_update(&r->pill_glow,dt);
        ug_scene_update(&r->scene,dt);
        uint32_t msg;
        while(ug_scene_poll_app(&r->scene,&msg))
            if(msg==UG_MSG_ENTER_DONE)ug_scene_refocus(&r->scene);
        pause_reference_apply(r);
        pqd_update(&r->quit,dt);
    }
}
/* The quit dialog is its own specified component (QUIT_DIALOG.md); the reference drives it through the
 * same state machine as production, while the pause screen underneath stays independently evaluated. */
static int pause_reference_quit_event(PauseMenu *r,const char *event,float x,float y,int coordinates) {
    PqdResult res=PQD_NONE;
    if(!strcmp(event,"Right"))res=pqd_key(&r->quit,PQD_KEY_RIGHT);
    else if(!strcmp(event,"Left"))res=pqd_key(&r->quit,PQD_KEY_LEFT);
    else if(!strcmp(event,"Enter") || !strcmp(event,"Continue"))res=pqd_key(&r->quit,PQD_KEY_ACCEPT);
    else if(!strcmp(event,"Back") || !strcmp(event,"Escape"))res=pqd_key(&r->quit,PQD_KEY_BACK);
    else if(!strcmp(event,"right_click"))res=pqd_secondary_press(&r->quit);
    else if(!strcmp(event,"rail_up") || ((!strcmp(event,"rail_down") || !strcmp(event,"rail_move")) && coordinates>=1))return 1;
    else if(coordinates==2 && !strcmp(event,"hover"))res=pqd_pointer_move(&r->quit,x,y);
    else if(coordinates==2 && !strcmp(event,"press"))res=pqd_primary_press(&r->quit,x,y);
    else if(coordinates==2 && !strcmp(event,"release"))res=pqd_primary_release(&r->quit,x,y);
    else if(coordinates==2 && !strcmp(event,"pointer")) {
        res=pqd_primary_press(&r->quit,x,y);
        if(res==PQD_NONE)res=pqd_primary_release(&r->quit,x,y);
    } else return 0;
    if(res==PQD_CONFIRM){r->open=0;r->capture_actions++;r->capture_last_action=FE_ACTION_QUIT;} /* immediate */
    return 1;
}
static int pause_reference_event(PauseMenu *r,const char *event,float x,float y,int coordinates) {
    if(!strcmp(event,"open"))return pause_reference_open(r);
    if(r->open && r->quit.open)return pause_reference_quit_event(r,event,x,y,coordinates);
    FeInput input=FE_INPUT_NONE;int before=r->state.selected,direction=0,click=0;
    if(!strcmp(event,"Right")){input=FE_INPUT_DOWN;direction=1;}
    else if(!strcmp(event,"Left")){input=FE_INPUT_UP;direction=-1;}
    else if(!strcmp(event,"Enter") || !strcmp(event,"Continue"))input=FE_INPUT_CONFIRM;
    else if(!strcmp(event,"Back") || !strcmp(event,"Escape") || !strcmp(event,"right_click"))input=FE_INPUT_BACK;
    else if(!strcmp(event,"rail_up") || ((!strcmp(event,"rail_down") || !strcmp(event,"rail_move")) && coordinates>=1))return 1;
    else if(!strcmp(event,"press") || !strcmp(event,"release") || !strcmp(event,"pointer")) {
        if(coordinates!=2)return 0;
        click=1;
    } else if(!strcmp(event,"hover") && coordinates==2) {
        int hover=-1;
        if(r->state.screen==FE_SCREEN_PAUSE) {
            for(int k=0;k<2;k++)if(ug_rect_hit(pause_menu_bounds(k?PAUSE_CONTINUE:PAUSE_BACK),x,y))hover=k;
        }
        if(hover!=r->pill_hover) {
            r->pill_hover=hover;
            if(hover>=0){ug_anim_play(&r->pill_pulse,UG_ANIM_PILL_FOCUS,0);ug_anim_play(&r->pill_glow,UG_ANIM_PILL_GLOW,0);}
            else{ug_anim_stop(&r->pill_pulse);ug_anim_stop(&r->pill_glow);}
        }
        return 1;
    } else return 0;
    if(!r->open)return 1;
    if(click) {
        int press=strcmp(event,"release")!=0,release=strcmp(event,"press")!=0,armed=r->press_center;
        r->press_center=0;
        if(press && ug_rect_hit(pause_menu_bounds(PAUSE_BACK),x,y))input=FE_INPUT_BACK;
        else if(press && ug_rect_hit(pause_menu_bounds(PAUSE_CONTINUE),x,y))input=FE_INPUT_CONFIRM;
        else if(press && r->state.screen==FE_SCREEN_PAUSE && pause_menu_arrow_at(x,y)) {
            /* Arrow press steps at press-down (BEHAVIOR 4.7); hidden arrows are inert (4.9). */
            int d=pause_menu_arrow_at(x,y);
            if(pause_reference_step(r,d)>=0){direction=d;input=d<0?FE_INPUT_UP:FE_INPUT_DOWN;}
        } else {
            if(press && r->state.screen==FE_SCREEN_PAUSE)armed=ug_rect_hit(pause_menu_center_hit(),x,y);
            if(release && armed && r->state.screen==FE_SCREEN_PAUSE && ug_rect_hit(pause_menu_center_hit(),x,y))input=FE_INPUT_CONFIRM;
            else if(!release)r->press_center=armed;
        }
        if(input==FE_INPUT_NONE)return 1;
    }
    if(direction) {
        int target=pause_reference_step(r,direction);
        if(target<0)return 1; /* no wrap */
        r->state.selected=target;
    } else if(input==FE_INPUT_CONFIRM && pause_selected_is_quit(&r->state)) {
        r->press_center=0;r->pill_hover=-1;ug_anim_stop(&r->pill_pulse);ug_anim_stop(&r->pill_glow);
        pqd_open(&r->quit,&r->assets,1);
    } else fe_input(&r->state,input);
    if(r->state.selected!=before) {
        int enabled=0,rank=0;
        for(int i=0;i<r->state.entry_count;i++)if(r->state.entries[i].enabled){enabled++;if(i<r->state.selected)rank++;}
        r->marker_x=enabled>1?50+546.0f*rank/(enabled-1):50; /* snaps */
        if(pause_reference_step(r,direction)>=0)
            ug_scene_post(&r->scene,direction<0?0x9120409Eu:0xB5971BF1u,direction<0?0xDE06Du:0xDE087u);
    }
    FeAction action=fe_poll_action(&r->state);
    if(action==FE_ACTION_RESUME) {
        r->open=0;r->capture_actions++;r->capture_last_action=action; /* immediate */
    } else if(action!=FE_ACTION_NONE){r->capture_actions++;r->capture_last_action=action;}
    if(r->open){pause_reference_apply(r);pause_reference_advance(r,0);}
    return 1;
}
int pause_menu_draw_reference(PauseMenu *p,int width,int height,float elapsed_ms,const char *events) {
    if(!p || !p->gl || !p->package.nobj || !pause_menu_capture_duration_valid(elapsed_ms) || width<=0 || height<=0)return 0;
    PauseMenu r={0};r.package=p->package;r.assets=p->assets;r.skin=p->skin;
    memcpy(r.item_tex,p->item_tex,sizeof r.item_tex);ug_ctx_init(&r.ctx);ug_cv_init(&r.canvas);
    pqd_init(&r.quit);r.quit.have_pkg=p->quit.have_pkg;r.quit.pkg=p->quit.pkg;
    snprintf(r.quit.load_status,sizeof r.quit.load_status,"%s",p->quit.load_status);
    int ok=pause_reference_open(&r);float last=0;
    char *copy=events?strdup(events):NULL,*save=NULL;
    if(events && !copy)ok=0;
    for(char *part=ok && copy?strtok_r(copy,",",&save):NULL;part && ok;part=strtok_r(NULL,",",&save)) {
        float at,x=0,y=0;char name[32]={0};int fields=sscanf(part,"%f:%31[^:]:%f:%f",&at,name,&x,&y);
        if(fields<2 || !isfinite(at) || at<last || at>elapsed_ms){ok=0;break;}
        pause_reference_advance(&r,at-last);last=at;ok=pause_reference_event(&r,name,x,y,fields-2);
    }
    free(copy);
    if(ok) {
        pause_reference_advance(&r,elapsed_ms-last);
        const UgFace *oldface=ug_text_face();const UgSkin *oldskin=ug_kit_skin();UgSkin saved;if(oldskin)saved=*oldskin;
        ug_text_set_face(ug_assets_main_face(&r.assets));ug_kit_set_skin(&r.skin);
        if(r.open) {
            ug_scene_draw(&r.scene,&r.ctx,&r.canvas);pqd_draw(&r.quit,&r.ctx,&r.canvas);
            ug_cv_translate(&r.canvas,-320,-240);
            pause_draw_pills(&r);
            ok=ug_cv_visual_status(&r.canvas)==UG_VIS_OK;
            if(ok && r.quit.open && !r.quit.visual) {
                fprintf(stderr,"pause reference: %s\n",r.quit.status);ok=0;
            }
            if(ok)ug_gl_render_overlay(p->gl,&r.canvas,width,height);
        }
        ug_text_set_face(oldface);ug_kit_set_skin(oldskin?&saved:NULL);
        fprintf(stderr,"pause reference: populated resume/quit elapsed=%.3f ui=%.3f selected=%d marker_x=%.3f open=%d actions=%d caption=%s\n",
                elapsed_ms,r.ui_ms,r.state.selected,r.marker_x,r.open,r.capture_actions,
                r.scene.nodes[r.caption].text_override?r.scene.nodes[r.caption].text_override:"");
    }
    pqd_drop_scene(&r.quit);   /* the package belongs to the production adapter */
    if(r.loaded)ug_scene_free(&r.scene);
    ug_anim_free(&r.pill_pulse);ug_anim_free(&r.pill_glow);ug_cv_free(&r.canvas);
    return ok;
}
