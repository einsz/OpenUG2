/* Pause SPEC 2-5: runtime native art over the host's paused driving scene. */
#include "pause_menu.h"
#include "pause_menu_layout.h"
#include "pause_menu_binding.h"
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
    int loaded, compatible, closing;
    FeAction closing_action;
    float ui_ms, close_ms, marker_x, marker_from, marker_to, marker_ms;
    int rail_drag, marker_native, nav_direction, capture_actions, capture_last_action;
    int slot, caption, marker, left, right;
    int roles[PAUSE_ROLE_COUNT], bound;
    int pool[PAUSE_POOL_COUNT], npool;
    int pill_hover, pill_initialized;
    UgAnim pill_pulse,pill_glow;
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
    ug_cv_init(&p->canvas);ug_ctx_init(&p->ctx);
    p->assets.main_face=-1;
    if(ug_assets_open_ingame(&p->assets,dataroot,NULL,p->status,sizeof p->status)) {
        pause_resolve_art(p);ug_skin_resolve(&p->skin,&p->assets);
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
    ug_fng_free(&p->package);ug_gl_destroy(p->gl);ug_cv_free(&p->canvas);
    if(p->pill_initialized){ug_anim_free(&p->pill_pulse);ug_anim_free(&p->pill_glow);}
    ug_assets_close(&p->assets);free(p);
}
void pause_menu_open(PauseMenu *p) {
    if(!p)return;
    fe_pause_init(&p->state);p->open=1;p->closing=0;p->closing_action=FE_ACTION_NONE;
    p->ui_ms=p->close_ms=p->marker_ms=0;
    if(p->pill_initialized){ug_anim_free(&p->pill_pulse);ug_anim_free(&p->pill_glow);}
    ug_anim_init(&p->pill_pulse,UG_WHITE,0,0);ug_anim_init(&p->pill_glow,UG_WHITE,0,0);
    p->pill_initialized=1;p->pill_hover=-1;
    p->marker_x=p->marker_from=p->marker_to=50;p->rail_drag=p->marker_native=p->nav_direction=0;
    p->original=p->compatible && p->gl && pause_scene_open(p);
    if(p->compatible && !p->original)fprintf(stderr,"pause: %s\n",p->status);
}
int pause_menu_is_open(const PauseMenu *p){return p && p->open;}
int pause_menu_visual_available(const PauseMenu *p){return p && p->original;}
const char *pause_menu_status(const PauseMenu *p){return p?p->status:"pause menu allocation unavailable";}
static float pause_marker_target(const PauseMenu *p) {
    int rank=0,n=0;
    for(int i=0;i<p->state.entry_count;i++)if(p->state.entries[i].enabled) {
        if(i<p->state.selected)rank++;
        n++;
    }
    return n>1?50+546.0f*rank/(n-1):50;
}
static int pause_can_step(const PauseMenu *p,int dir) {
    Fe copy=p->state;
    if(copy.screen!=FE_SCREEN_PAUSE)return 0;
    fe_input(&copy,dir<0?FE_INPUT_UP:FE_INPUT_DOWN);
    return copy.selected!=p->state.selected;
}
static void pause_selection_changed(PauseMenu *p,int before) {
    int direction=p->nav_direction?p->nav_direction:(p->state.selected<before?-1:1);
    p->nav_direction=0;
    if(p->state.selected==before)return;
    p->marker_native=0;
    p->marker_from=p->marker_x;p->marker_to=pause_marker_target(p);p->marker_ms=0;
    if(p->loaded) {
        /* Package directional events explicitly target the arrow, BINDING 5. */
        ug_scene_post(&p->scene,direction<0?0x9120409Eu:0xB5971BF1u,
                      direction<0?0x000DE06Du:0x000DE087u);
        pause_bind_apply(p);
    }
}
static void pause_dispatch(PauseMenu *p,FeInput in) {
    if(p->closing || p->state.pending!=FE_ACTION_NONE)return;
    int before=p->state.selected;
    p->nav_direction=in==FE_INPUT_UP?-1:in==FE_INPUT_DOWN?1:0;
    fe_input(&p->state,in);
    if(p->loaded && (p->state.pending==FE_ACTION_RESUME || p->state.pending==FE_ACTION_QUIT))
        ug_scene_post(&p->scene,in==FE_INPUT_BACK?UG_MSG_REQ_BACK:UG_MSG_REQ_FORWARD,0);
    pause_selection_changed(p,before);
}
static void pause_rail_select(PauseMenu *p,float x) {
    int n=0,ids[FE_MENU_MAX];
    for(int i=0;i<p->state.entry_count;i++)if(p->state.entries[i].enabled)ids[n++]=i;
    if(!n)return;
    float u=fmaxf(0,fminf(1,(x-50)/546));
    int index=(int)floorf((n-1)*u+.5f),before=p->state.selected;
    p->state.selected=ids[index];pause_selection_changed(p,before);
}
const char *pause_menu_caption(const PauseMenu *p) {
    if(!p || p->state.selected<0 || p->state.selected>=p->state.entry_count)return "";
    const FeMenuEntry *e=&p->state.entries[p->state.selected];
    return e->action==FE_ACTION_RESUME?"Return to Explore Mode":e->label;
}

void pause_menu_pointer(PauseMenu *p,float px,float py,int w,int h) {
    if(!pause_menu_is_open(p) || w<=0 || h<=0 || p->state.pending!=FE_ACTION_NONE || p->closing)return;
    float x,y;
    pause_menu_to_virtual(pause_menu_transform(w,h),px,py,&x,&y);
    if(x<0 || x>=640 || y<0 || y>=480)return;
    FeInput in=FE_INPUT_NONE;
    if(p->state.screen==FE_SCREEN_QUIT_CONFIRM) {
        if(ug_rect_hit((UgRect){170,264,140,40},x,y))p->state.quit_sel=0;
        else if(ug_rect_hit((UgRect){330,264,140,40},x,y))p->state.quit_sel=1;
        else return;
        in=FE_INPUT_CONFIRM;
    } else if(ug_rect_hit(pause_menu_bounds(PAUSE_BACK),x,y))in=FE_INPUT_BACK;
    else if(ug_rect_hit(pause_menu_bounds(PAUSE_CONTINUE),x,y))in=FE_INPUT_CONFIRM;
    else {
        if(ug_rect_hit((UgRect){33.3f,184,579.1f,8.9f},x,y)) {
            pause_rail_select(p,x);return;
        }
        if(ug_rect_hit((UgRect){45,213,31,31},x,y)) {
            if(pause_can_step(p,-1))pause_dispatch(p,FE_INPUT_UP);
            return;
        }
        if(ug_rect_hit((UgRect){564.5f,213,31,31},x,y)) {
            if(pause_can_step(p,1))pause_dispatch(p,FE_INPUT_DOWN);
            return;
        }
        for(int i=0;i<p->state.entry_count;i++) {
            if(ug_rect_hit(pause_menu_item_bounds(i-p->state.selected),x,y)) {
                if(p->state.entries[i].enabled) { int before=p->state.selected;p->state.selected=i;pause_selection_changed(p,before); }
                return; /* selection alone; never dispatch a host action */
            }
        }
    }
    pause_dispatch(p,in);
}

/* PC pills use the existing shared focus/glow animations (base 5/6.8, pause 3). */
static void pause_pill_hover(PauseMenu *p,float x,float y) {
    int hover=-1;
    if(p->state.screen==FE_SCREEN_PAUSE && !p->closing) {
        if(ug_rect_hit(pause_menu_bounds(PAUSE_BACK),x,y))hover=0;
        else if(ug_rect_hit(pause_menu_bounds(PAUSE_CONTINUE),x,y))hover=1;
    }
    if(hover==p->pill_hover)return;
    p->pill_hover=hover;
    if(hover>=0) {ug_anim_play(&p->pill_pulse,UG_ANIM_PILL_FOCUS,0);ug_anim_play(&p->pill_glow,UG_ANIM_PILL_GLOW,0);}
    else {ug_anim_stop(&p->pill_pulse);ug_anim_stop(&p->pill_glow);}
}

void pause_menu_event(PauseMenu *p,const SDL_Event *e,SDL_Window *win) {
    if(!pause_menu_is_open(p) || p->state.pending!=FE_ACTION_NONE || p->closing)return;
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
    } else if(e->type==SDL_MOUSEBUTTONDOWN && e->button.button==SDL_BUTTON_LEFT && win) {
        int w,h;SDL_GetWindowSize(win,&w,&h);
        if(w<=0 || h<=0)return;
        float x,y;pause_menu_to_virtual(pause_menu_transform(w,h),e->button.x,e->button.y,&x,&y);
        p->rail_drag=p->state.screen==FE_SCREEN_PAUSE && ug_rect_hit((UgRect){33.3f,184,579.1f,8.9f},x,y);
        pause_menu_pointer(p,e->button.x,e->button.y,w,h);
        return;
    } else if(e->type==SDL_MOUSEBUTTONUP && e->button.button==SDL_BUTTON_LEFT) p->rail_drag=0;
    else if(e->type==SDL_MOUSEMOTION && win) {
        int w,h;SDL_GetWindowSize(win,&w,&h);
        if(w>0 && h>0) {
            float x,y;pause_menu_to_virtual(pause_menu_transform(w,h),e->motion.x,e->motion.y,&x,&y);
            pause_pill_hover(p,x,y);
            if(p->rail_drag && p->state.screen==FE_SCREEN_PAUSE)pause_rail_select(p,x);
            if(p->loaded) {UgPtr ptr={.type=UG_PT_MOVE,.x=x-320,.y=y-240};ug_scene_pointer(&p->scene,&ptr);}
        }
    }
    pause_dispatch(p,in);
}
FeAction pause_menu_update(PauseMenu *p,float seconds) {
    if(!pause_menu_is_open(p))return FE_ACTION_NONE;
    float ms=fmaxf(0,seconds*1000);
    p->ui_ms+=ms;p->ctx.time_ms=p->ui_ms;
    ug_anim_update(&p->pill_pulse,ms);ug_anim_update(&p->pill_glow,ms);
    p->marker_ms=fminf(160,p->marker_ms+ms);
    if(!p->marker_native)p->marker_x=p->marker_from+(p->marker_to-p->marker_from)*p->marker_ms/160;
    int exit_done=0;
    if(p->loaded) {
        ug_scene_update(&p->scene,ms);
        uint32_t msg;
        while(ug_scene_poll_app(&p->scene,&msg)) {
            if(msg==UG_MSG_ENTER_DONE)ug_scene_refocus(&p->scene);
            if(msg==UG_MSG_EXIT_DONE)exit_done=1;
            /* Script events are presentation events. Fe alone dispatches actions. */
        }
        pause_bind_apply(p);
    }
    fe_update(&p->state,seconds);
    FeAction a=fe_poll_action(&p->state);
    if(a==FE_ACTION_RESUME || a==FE_ACTION_QUIT) {
        if(!p->loaded){p->open=0;return a;}
        p->closing=1;p->closing_action=a;p->close_ms=0;p->rail_drag=0;
    } else if(a!=FE_ACTION_NONE)return a;
    if(p->closing) {
        p->close_ms+=ms;
        /* Same supported-scene exit timeout as the independent screen viewer. */
        if(exit_done || p->close_ms>=UG_TUNE_EXIT_TIMEOUT_MS) {
            a=p->closing_action;p->closing_action=FE_ACTION_NONE;p->closing=0;p->open=0;
            return a;
        }
    }
    return FE_ACTION_NONE;
}

/* Nine-slice the documented native 128x64 focus artwork. Genuine corner texels
 * retain their size while edge texels stretch; no generated border/glow. */
static void native_frame(UgCanvas *cv,UgRect r,int tex,UgColor tint) {
    const float u[]={0,.125f,.875f,1},v[]={0,.25f,.75f,1};
    float xs[]={r.x,r.x+16,r.x+r.w-16,r.x+r.w};
    float ys[]={r.y,r.y+16,r.y+r.h-16,r.y+r.h};
    for(int row=0;row<3;row++)for(int col=0;col<3;col++)
        ug_cv_image(cv,xs[col],ys[row],xs[col+1],ys[row+1],u[col],v[row],u[col+1],v[row+1],tex,tint);
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
static int pause_position_track(const UgNode *n) {
    const UgScript *script=n->pl.running?n->pl.cur:NULL;
    if(!script)return 0;
    for(int i=0;i<script->ntracks;i++) {
        const UgTrack *t=&script->tracks[i];
        int end=(int)t->prop+ug_type_ncomp(t->type);
        if(t->nkeys>1 && t->prop<=UG_P_POS+1 && end>UG_P_POS)return 1;
    }
    return 0;
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
        int offset=k-p->state.selected;
        n->app_hidden=!valid || k>=p->state.entry_count || abs(offset)>1 || k>=FE_MENU_MAX || !p->item_tex[k];
        n->tex_override=k<p->state.entry_count && k<FE_MENU_MAX?p->item_tex[k]:0;
        if(!n->app_hidden) {
            ug_scene_set_world_pos(s,n->index,slot->pos[0]+158.0f*offset,slot->pos[1]);
            n->props.w[UG_P_SIZE]=offset?96:118;n->props.w[UG_P_SIZE+1]=offset?48:59;
            /* Preserve evaluated color/fade/highlight; assignment controls only art/position/size/visibility. */
        }
    }
    s->nodes[p->roles[PAUSE_ROLE_BACKING]].app_hidden=!valid;
    s->nodes[p->roles[PAUSE_ROLE_GLOW]].app_hidden=!valid;
    if(valid && p->state.selected<p->npool && s->focus!=p->pool[p->state.selected])
        ug_scene_set_focus(s,p->pool[p->state.selected]);
    UgNode *marker=&s->nodes[p->marker];
    float dx=pause_marker_offset_x(p,marker);
    if(pause_position_track(marker))p->marker_native=1;
    if(p->marker_native)p->marker_x=marker->we+dx+320;
    else ug_scene_set_world_pos(s,p->marker,p->marker_x-320-dx,marker->wf);
    s->nodes[p->left].app_hidden=!pause_can_step(p,-1);
    s->nodes[p->right].app_hidden=!pause_can_step(p,1);
    ug_scene_relayout(s);
}
static void pause_draw_pills(PauseMenu *p) {
    if(!p->bound || p->state.screen!=FE_SCREEN_PAUSE)return;
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
static void pause_draw_quit(PauseMenu *p) {
    UgCanvas *cv=&p->canvas;
    if(p->state.screen==FE_SCREEN_QUIT_CONFIRM) {
        /* Preserve the supported subview and its established Back semantics. */
        ug_kit_panel(cv,(UgRect){135,165,370,150},0,1);
        native_frame(cv,(UgRect){135,165,370,150},p->skin.icon_hl,UG_ACCENT);
        native_text(p,"Quit to desktop?",(UgRect){150,190,340,26},.8f,UG_WHITE,UG_J_CENTER);
        for(int i=0;i<2;i++) {
            UgRect r={i?330:170,264,140,40};
            ug_kit_pill_pc(cv,r,UG_WHITE);
            if(p->state.quit_sel==i)ug_kit_pill_glow(cv,r,UG_ACCENT);
            native_text(p,i?"Yes":"No",r,.7f,p->state.quit_sel==i?UG_ACCENT:UG_WHITE,UG_J_CENTER);
        }
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
    ug_cv_translate(cv,-320,-240);
    pause_draw_pills(p);
    pause_draw_quit(p);
}
void pause_menu_draw(PauseMenu *p,int width,int height) {
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
 * pointer/hover/rail_down/rail_move take top-left virtual x:y; rail_up releases.
 * The normal gameplay adapter never calls these entry points. */
static void pause_capture_tick(PauseMenu *p,float seconds) {
    FeAction a=pause_menu_update(p,seconds);
    if(a!=FE_ACTION_NONE){p->capture_actions++;p->capture_last_action=a;}
}
static void pause_capture_advance(PauseMenu *p,float ms) {
    while(ms>0) {float step=fminf(ms,10);pause_capture_tick(p,step*.001f);ms-=step;}
}
int pause_menu_capture_replay(PauseMenu *p,const char *events,float elapsed_ms,int w,int h) {
    if(!p || elapsed_ms<0 || w<=0 || h<=0)return 0;
    float previous=0;p->capture_actions=0;p->capture_last_action=FE_ACTION_NONE;
    char *copy=events?strdup(events):NULL,*save=NULL;
    if(events && !copy)return 0;
    for(char *item=copy?strtok_r(copy,",",&save):NULL;item;item=strtok_r(NULL,",",&save)) {
        float at,x=0,y=0;char name[32]={0};
        int fields=sscanf(item,"%f:%31[^:]:%f:%f",&at,name,&x,&y);
        if(fields<2 || !isfinite(at) || at<previous || at>elapsed_ms) {free(copy);return 0;}
        pause_capture_advance(p,at-previous);previous=at;
        if(!strcmp(name,"open"))pause_menu_open(p);
        else if(!strcmp(name,"Right"))pause_dispatch(p,FE_INPUT_DOWN);
        else if(!strcmp(name,"Left"))pause_dispatch(p,FE_INPUT_UP);
        else if(!strcmp(name,"Enter") || !strcmp(name,"Continue"))pause_dispatch(p,FE_INPUT_CONFIRM);
        else if(!strcmp(name,"Back") || !strcmp(name,"Escape"))pause_dispatch(p,FE_INPUT_BACK);
        else if(!strcmp(name,"rail_up"))p->rail_drag=0;
        else if(fields>=3 && (!strcmp(name,"rail_down") || !strcmp(name,"rail_move"))) {
            if(!strcmp(name,"rail_down")) {
                p->rail_drag=p->state.screen==FE_SCREEN_PAUSE && ug_rect_hit((UgRect){33.3f,184,579.1f,8.9f},x,fields==4?y:188.4f);
            }
            if(p->rail_drag && !p->closing && p->open)pause_rail_select(p,x);
        } else if(fields==4 && !strcmp(name,"pointer")) {
            PauseTransform t=pause_menu_transform(w,h);
            pause_menu_pointer(p,(float)(t.ox+x*t.scale),(float)(t.oy+y*t.scale),w,h);
        } else if(fields==4 && !strcmp(name,"hover") && p->loaded) {
            pause_pill_hover(p,x,y);UgPtr ptr={.type=UG_PT_MOVE,.x=x-320,.y=y-240};ug_scene_pointer(&p->scene,&ptr);
        } else {free(copy);return 0;}
        pause_capture_tick(p,0);
    }
    free(copy);pause_capture_advance(p,elapsed_ms-previous);
    fprintf(stderr,"pause capture: elapsed=%.3f ui=%.3f selected=%d marker_x=%.3f open=%d closing=%d actions=%d last_action=%d caption=%s status=%s\n",
            elapsed_ms,p->ui_ms,p->state.selected,p->marker_x,p->open,p->closing,p->capture_actions,p->capture_last_action,pause_menu_caption(p),p->status);
    return 1;
}
/* Separate reference evaluator. It shares only reviewed data identities, Fe's
 * existing supported action semantics, UgScene and native drawing primitives.
 * It never calls production detection/binding/update/input/capture functions. */
static void pause_reference_apply(PauseMenu *r) {
    UgScene *s=&r->scene;
    int selected=r->state.selected,valid=selected>=0 && selected<r->state.entry_count;
    s->nodes[r->slot].app_hidden=1;
    s->nodes[r->roles[PAUSE_ROLE_HEADING]].text_override="Pause";
    s->nodes[r->caption].text_override=valid?(r->state.entries[selected].action==FE_ACTION_RESUME?
        "Return to Explore Mode":r->state.entries[selected].label):"";
    const UgFngObject *anchor=s->nodes[r->slot].obj;
    for(int i=0;i<PAUSE_POOL_COUNT;i++) {
        UgNode *n=&s->nodes[r->pool[i]];
        int distance=i-selected;
        n->app_hidden=!valid || i>=r->state.entry_count || abs(distance)>1 || i>=FE_MENU_MAX || !r->item_tex[i];
        n->tex_override=i<r->state.entry_count && i<FE_MENU_MAX?r->item_tex[i]:0;
        if(!n->app_hidden) {
            ug_scene_set_world_pos(s,n->index,anchor->pos[0]+distance*158,anchor->pos[1]);
            n->props.w[UG_P_SIZE]=distance?96:118;n->props.w[UG_P_SIZE+1]=distance?48:59;
        }
    }
    if(valid && s->focus!=r->pool[selected])ug_scene_set_focus(s,r->pool[selected]);
    s->nodes[r->roles[PAUSE_ROLE_BACKING]].app_hidden=!valid;
    s->nodes[r->roles[PAUSE_ROLE_GLOW]].app_hidden=!valid;
    UgNode *marker=&s->nodes[r->marker];
    float offset=pause_marker_offset_x(r,marker);
    /* BINDING 5: marker's authored tracks are color-only; reviewed D position is separate. */
    ug_scene_set_world_pos(s,r->marker,r->marker_x-320-offset,marker->wf);
    for(int direction=-1;direction<=1;direction+=2) {
        Fe next=r->state;fe_input(&next,direction<0?FE_INPUT_UP:FE_INPUT_DOWN);
        s->nodes[direction<0?r->left:r->right].app_hidden=next.selected==selected || r->state.screen!=FE_SCREEN_PAUSE;
    }
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
    r->ui_ms=r->close_ms=r->marker_ms=0;r->marker_x=r->marker_from=r->marker_to=50;
    r->open=1;r->closing=0;r->closing_action=FE_ACTION_NONE;r->rail_drag=0;
    ug_scene_update(&r->scene,0);pause_reference_apply(r);return 1;
}
static void pause_reference_advance(PauseMenu *r,float milliseconds) {
    int first=1;
    while((first || milliseconds>0) && r->open) {
        first=0;
        float dt=fminf(milliseconds,10);milliseconds-=dt;r->ui_ms+=dt;r->ctx.time_ms=r->ui_ms;
        r->marker_ms=fminf(r->marker_ms+dt,160);
        r->marker_x=r->marker_from+(r->marker_to-r->marker_from)*(r->marker_ms/160);
        ug_anim_update(&r->pill_pulse,dt);ug_anim_update(&r->pill_glow,dt);
        ug_scene_update(&r->scene,dt);
        int completed=0;uint32_t msg;
        while(ug_scene_poll_app(&r->scene,&msg)) {
            if(msg==UG_MSG_ENTER_DONE)ug_scene_refocus(&r->scene);
            if(msg==UG_MSG_EXIT_DONE)completed=1;
        }
        pause_reference_apply(r);
        if(r->closing) {
            r->close_ms+=dt;
            if(completed || r->close_ms>=UG_TUNE_EXIT_TIMEOUT_MS) {
                r->open=0;r->closing=0;r->capture_actions++;r->capture_last_action=r->closing_action;
                r->closing_action=FE_ACTION_NONE;
            }
        }
    }
}
static int pause_reference_event(PauseMenu *r,const char *event,float x,float y,int coordinates) {
    if(!strcmp(event,"open"))return pause_reference_open(r);
    FeInput input=FE_INPUT_NONE;int before=r->state.selected,explicit_index=-1,direction=0;
    int rail=!strcmp(event,"rail_down") || !strcmp(event,"rail_move");
    if(!strcmp(event,"Right")){input=FE_INPUT_DOWN;direction=1;}
    else if(!strcmp(event,"Left")){input=FE_INPUT_UP;direction=-1;}
    else if(!strcmp(event,"Enter") || !strcmp(event,"Continue"))input=FE_INPUT_CONFIRM;
    else if(!strcmp(event,"Back") || !strcmp(event,"Escape"))input=FE_INPUT_BACK;
    else if(!strcmp(event,"rail_up")){r->rail_drag=0;return 1;}
    else if(!strcmp(event,"hover") && coordinates==2) {
        int hover=-1;
        if(r->state.screen==FE_SCREEN_PAUSE && !r->closing) {
            for(int k=0;k<2;k++)if(ug_rect_hit(pause_menu_bounds(k?PAUSE_CONTINUE:PAUSE_BACK),x,y))hover=k;
        }
        if(hover!=r->pill_hover) {
            r->pill_hover=hover;
            if(hover>=0){ug_anim_play(&r->pill_pulse,UG_ANIM_PILL_FOCUS,0);ug_anim_play(&r->pill_glow,UG_ANIM_PILL_GLOW,0);}
            else{ug_anim_stop(&r->pill_pulse);ug_anim_stop(&r->pill_glow);}
        }
        UgPtr ptr={.type=UG_PT_MOVE,.x=x-320,.y=y-240};ug_scene_pointer(&r->scene,&ptr);return 1;
    } else if((!strcmp(event,"pointer") && coordinates==2) || (rail && coordinates>=1)) {
        if(rail && coordinates==1)y=188.4f;
        if(!r->open || r->closing)return 1;
        if(r->state.screen==FE_SCREEN_QUIT_CONFIRM) {
            if(ug_rect_hit((UgRect){170,264,140,40},x,y)){r->state.quit_sel=0;input=FE_INPUT_CONFIRM;}
            else if(ug_rect_hit((UgRect){330,264,140,40},x,y)){r->state.quit_sel=1;input=FE_INPUT_CONFIRM;}
        } else if(ug_rect_hit(pause_menu_bounds(PAUSE_BACK),x,y))input=FE_INPUT_BACK;
        else if(ug_rect_hit(pause_menu_bounds(PAUSE_CONTINUE),x,y))input=FE_INPUT_CONFIRM;
        else if(rail || ug_rect_hit((UgRect){33.3f,184,579.1f,8.9f},x,y)) {
            if(!strcmp(event,"rail_down"))r->rail_drag=ug_rect_hit((UgRect){33.3f,184,579.1f,8.9f},x,y);
            if(!rail || r->rail_drag) {
                int enabled[FE_MENU_MAX],n=0;
                for(int i=0;i<r->state.entry_count;i++)if(r->state.entries[i].enabled)enabled[n++]=i;
                if(n)explicit_index=enabled[(int)floorf(fmaxf(0,fminf(1,(x-50)/546))*(n-1)+.5f)];
            }
        } else if(ug_rect_hit((UgRect){45,213,31,31},x,y)){input=FE_INPUT_UP;direction=-1;}
        else if(ug_rect_hit((UgRect){564.5f,213,31,31},x,y)){input=FE_INPUT_DOWN;direction=1;}
        else for(int i=0;i<r->state.entry_count;i++)
            if(r->state.entries[i].enabled && ug_rect_hit(pause_menu_item_bounds(i-before),x,y)){explicit_index=i;break;}
    } else return 0;
    if(!r->open || r->closing)return 1;
    if(explicit_index>=0)r->state.selected=explicit_index;
    else fe_input(&r->state,input);
    if(r->state.selected!=before) {
        if(!direction)direction=r->state.selected>before?1:-1;
        ug_scene_post(&r->scene,direction<0?0x9120409Eu:0xB5971BF1u,direction<0?0xDE06Du:0xDE087u);
        int enabled=0,rank=0;
        for(int i=0;i<r->state.entry_count;i++)if(r->state.entries[i].enabled){enabled++;if(i<r->state.selected)rank++;}
        r->marker_from=r->marker_x;r->marker_to=enabled>1?50+546.0f*rank/(enabled-1):50;r->marker_ms=0;
    }
    FeAction action=fe_poll_action(&r->state);
    if(action==FE_ACTION_RESUME || action==FE_ACTION_QUIT) {
        ug_scene_post(&r->scene,input==FE_INPUT_BACK?UG_MSG_REQ_BACK:UG_MSG_REQ_FORWARD,0);
        r->closing=1;r->closing_action=action;r->close_ms=0;
    } else if(action!=FE_ACTION_NONE){r->capture_actions++;r->capture_last_action=action;}
    pause_reference_apply(r);pause_reference_advance(r,0);return 1;
}
int pause_menu_draw_reference(PauseMenu *p,int width,int height,float elapsed_ms,const char *events) {
    if(!p || !p->gl || !p->package.nobj || !isfinite(elapsed_ms) || elapsed_ms<0 || width<=0 || height<=0)return 0;
    PauseMenu r={0};r.package=p->package;r.assets=p->assets;r.skin=p->skin;
    memcpy(r.item_tex,p->item_tex,sizeof r.item_tex);ug_ctx_init(&r.ctx);ug_cv_init(&r.canvas);
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
            ug_scene_draw(&r.scene,&r.ctx,&r.canvas);ug_cv_translate(&r.canvas,-320,-240);
            pause_draw_pills(&r);pause_draw_quit(&r);
            ok=ug_cv_visual_status(&r.canvas)==UG_VIS_OK;
            if(ok)ug_gl_render_overlay(p->gl,&r.canvas,width,height);
        }
        ug_text_set_face(oldface);ug_kit_set_skin(oldskin?&saved:NULL);
        fprintf(stderr,"pause reference: populated resume/quit elapsed=%.3f ui=%.3f selected=%d marker_x=%.3f open=%d actions=%d caption=%s\n",
                elapsed_ms,r.ui_ms,r.state.selected,r.marker_x,r.open,r.capture_actions,
                r.scene.nodes[r.caption].text_override?r.scene.nodes[r.caption].text_override:"");
    }
    if(r.loaded)ug_scene_free(&r.scene);
    ug_anim_free(&r.pill_pulse);ug_anim_free(&r.pill_glow);ug_cv_free(&r.canvas);
    return ok;
}
