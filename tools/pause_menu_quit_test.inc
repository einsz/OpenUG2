/* QUIT_DIALOG.md acceptance harness. Everything here is synthetic: a hand-built package that carries
 * the reviewed object identities and focus responses (numbers taken from test-vectors/quit_dialog.json),
 * synthetic string tables and a fake font record. No runtime/retail input is read.
 * tools/pause_menu_quit_check.py drives these modes from tools/pause_menu_vectors/quit_dialog.json. */

/* ---- synthetic authored package ----------------------------------------------------------- */
enum { QS_HL_BTN=11,QS_UNHL_BTN,QS_PULSE=21,QS_UNHL_LBL };
typedef struct {
    UgFngPackage pk;
    UgFngObject obj[17];
    UgFngResource res[12];
    UgScript scripts[8];UgTrack tracks[8];UgKey keys[8][4];int nscripts;
    UgFngResponse resp[8];UgFngCmd cmd[16];int nresp,ncmd;
} QdPackage;

static UgScript *qs_script(QdPackage *q,uint32_t id,int loop,int length,const UgColor *at,const int *t,int n) {
    int k=q->nscripts++;
    for(int i=0;i<n;i++)q->keys[k][i]=(UgKey){.t=t[i],.v={at[i].r,at[i].g,at[i].b,at[i].a}};
    q->tracks[k]=(UgTrack){.type=UG_VT_COLOR,.vsize=4,.length=(uint32_t)length,.prop=UG_P_COLOR,.keys=q->keys[k],.nkeys=n};
    q->scripts[k]=(UgScript){.id=id,.length=(uint32_t)length,.flags=loop?UG_SCRIPT_LOOP:0,.tracks=&q->tracks[k],.ntracks=1};
    return &q->scripts[k];
}
/* Focus responses: start a script on the object itself; a button also forwards the focus message to
 * its label (guid `label`, 0 = none), as the authored data does. */
static void qs_focus_responses(QdPackage *q,UgFngObject *o,uint32_t gained,uint32_t lost,uint32_t label) {
    int k=q->nresp,c=q->ncmd;
    q->cmd[c]=(UgFngCmd){.id=UG_CMD_START_SCRIPT,.arg=gained};
    if(label)q->cmd[c+1]=(UgFngCmd){.id=UG_CMD_POST_GUI,.arg=UG_MSG_FOCUS_GAINED,.target=label};
    q->resp[k]=(UgFngResponse){UG_MSG_FOCUS_GAINED,&q->cmd[c],label?2:1};c+=label?2:1;
    q->cmd[c]=(UgFngCmd){.id=UG_CMD_START_SCRIPT,.arg=lost};
    if(label)q->cmd[c+1]=(UgFngCmd){.id=UG_CMD_POST_GUI,.arg=UG_MSG_FOCUS_LOST,.target=label};
    q->resp[k+1]=(UgFngResponse){UG_MSG_FOCUS_LOST,&q->cmd[c],label?2:1};c+=label?2:1;
    o->resp=&q->resp[k];o->nresp=2;q->nresp+=2;q->ncmd=c;
}
/* Order: parents first; the unused single-button centre set comes first in file order, as authored. */
static void qs_build(QdPackage *q) {
    memset(q,0,sizeof *q);
    for(int i=0;i<12;i++)q->res[i]=(UgFngResource){.id=(uint32_t)i,.kind=1,.name="synthetic.tga"};
    q->res[0]=(UgFngResource){.id=0,.kind=0,.name="(Untextured Simple Image)"};
    q->res[5]=(UgFngResource){.id=5,.kind=2,.name="ConduitMdITC_TT21i.ffn"};
    #define OBJ(i,H,G,PAR,TY,RS,FL,...) q->obj[i]=(UgFngObject){.type=TY,.guid=G,.name_hash=H,.parent_guid=PAR,.has_parent=(PAR)!=0,.res_index=RS,.flags=FL,__VA_ARGS__}
    OBJ(0,0x0DE769B5u,0x000E0B39u,0,5,0,0x40000000u);
    OBJ(1,0xFB98DA12u,0x000DEF6Eu,0x000E0B39u,5,0,0x40000000u);
    OBJ(2,0xB8A7C6CEu,0x000DE8ECu,0,5,0,0x50080000u);
    OBJ(3,0xDA473D31u,0x000DE993u,0,2,5,0x50080000u,.size={.75f,.75f,1},.justify=1,.text="SINGLE");
    OBJ(4,0xB8A7C6CCu,0x000DE8CEu,0,5,0,0x50080000u,.pos={-151,74,0},.color={255,255,255,80});
    OBJ(5,0xB8A7C6CDu,0x000DE86Au,0,5,0,0x50080000u,.pos={151,74,0});
    OBJ(6,0xDA473D2Fu,0x000DE6E8u,0,2,5,0x50080000u,.pos={-158,63,0},.size={.75f,.75f,1},.justify=1,.color={225,225,225,255},.text="PLACEHOLDER LEFT");
    OBJ(7,0xDA473D30u,0x000DE6F0u,0,2,5,0x40080000u,.pos={140,63,0},.size={.75f,.75f,1},.justify=1,.color={204,244,138,255},.text="PLACEHOLDER RIGHT");
    OBJ(8,0x1E2640FAu,0x000DE6D6u,0,2,5,0x40000000u,.pos={0,1,0},.size={.75f,.75f,1},.justify=5,.max_width=720,.text="PLACEHOLDER MESSAGE");
    OBJ(9,0x426C7B4Du,0x000DEF80u,0x000DEF6Eu,2,5,0x40000000u,.size={.75f,.75f,1},.text="TITLE");
    OBJ(10,0xC26A8D5Cu,0x000DEF81u,0x000DEF6Eu,1,6,0x40000000u);
    OBJ(11,0x47FF4E7Cu,0x000DE076u,0,2,5,0x40000000u,.pos={169,-366,0},.text="CTRL");
    OBJ(12,0xF7206332u,0x000DF018u,0,2,5,0x40000000u,.pos={-114,-331,0},.text="OFF");
    OBJ(13,0xB62648B1u,0x000DDFC1u,0,1,11,0x40000000u);
    OBJ(14,0xB62648B1u,0x000DDFC0u,0,1,11,0x40000000u);
    OBJ(15,0xB1B1EAB0u,0x000DCD32u,0,5,0,0x40000000u);
    OBJ(16,0x014245E2u,0x000E0ADFu,0x000DEF6Eu,1,0,0x40000000u,.size={1070,650,0},.color={0,0,0,153}); /* darkening quad */
    #undef OBJ
    for(int i=0;i<17;i++) {            /* defaults for the fields an object does not set */
        UgFngObject *o=&q->obj[i];
        if(!o->size[0] && !o->size[1] && !o->size[2])o->size[0]=o->size[1]=o->size[2]=1;
        if(!(o->color.r|o->color.g|o->color.b|o->color.a))o->color=(UgColor){255,255,255,255};
        o->uv[2]=o->uv[3]=1;
    }
    /* focus responses: button alpha 225 / 80, label pulse (1200 ms loop) / (192,192,192) rest (vectors) */
    const UgColor hl[]={{255,255,255,225}},unhl[]={{255,255,255,80}};const int t0[]={0};
    qs_script(q,QS_HL_BTN,0,1,hl,t0,1);qs_script(q,QS_UNHL_BTN,0,1,unhl,t0,1);
    const UgColor pulse[]={{204,244,138,0},{204,244,138,255},{204,244,138,255},{204,244,138,0}};const int tp[]={0,440,760,1200};
    const UgColor grey[]={{192,192,192,255}};
    qs_script(q,QS_PULSE,1,1200,pulse,tp,4);qs_script(q,QS_UNHL_LBL,0,1,grey,t0,1);
    for(int i=4;i<=5;i++){q->obj[i].scripts=&q->scripts[0];q->obj[i].nscripts=2;qs_focus_responses(q,&q->obj[i],QS_HL_BTN,QS_UNHL_BTN,q->obj[i+2].guid);}
    for(int i=6;i<=7;i++){q->obj[i].scripts=&q->scripts[2];q->obj[i].nscripts=2;qs_focus_responses(q,&q->obj[i],QS_PULSE,QS_UNHL_LBL,0);}
    q->pk=(UgFngPackage){.obj=q->obj,.nobj=17,.res=q->res,.nres=12};
}

/* ---- fixtures ------------------------------------------------------------------------------ */
typedef struct {
    PauseMenu p;QdPackage qp;UgTexture tex;uint8_t pixel[4];UgFace face;
    int frame,quit_actions,quit_frame,resume_actions,other_actions;
    const char *last_result;
} QdFix;

static void qd_strings(UgStrTab *t,int variant) {
    ug_strtab_init(t);
    if(variant==2)return;                           /* empty table */
    ug_strtab_add(t,"CONFIRM_QUIT_GAME","Quit to Main Menu?");
    ug_strtab_add(t,"CONFIRM_QUIT_TO_MAIN_LOST_PROGRESS","Quit to Main Menu?^All unsaved race progress will be lost.");
    ug_strtab_add(t,"PC_NAV_CONFIRM_QUIT","Are you sure you want to quit?");
    ug_strtab_add(t,"PC_NAV_QUIT","Quit Game");ug_strtab_add(t,"CANCEL","Cancel");
    if(variant==3) {                                 /* only the equivalent alternates for Yes/No */
        ug_strtab_add(t,"UI_COMMON_YES","Yes*");ug_strtab_add(t,"UI_COMMON_NO","No*");
    } else {
        ug_strtab_add(t,"YES","Yes");ug_strtab_add(t,"NO","No");
        ug_strtab_add(t,"UI_COMMON_YES","Yes*");ug_strtab_add(t,"UI_COMMON_NO","No*");
    }
    if(variant!=4)ug_strtab_add(t,"MU_PAUSE_QUIT_TO_FE","Quit to Main Menu");
}
/* mode 0: logic only (no package); 1: synthetic authored package; 2: package but empty string table;
 * 3: alternates only; 4: no caption key. */
static void qd_fixture(QdFix *f,int mode) {
    memset(f,0,sizeof *f);
    PauseMenu *p=&f->p;pqd_init(&p->quit);ug_ctx_init(&p->ctx);ug_cv_init(&p->canvas);
    qd_strings(&p->assets.strings,mode==2?2:mode==3?3:mode==4?4:1);
    f->pixel[0]=f->pixel[1]=f->pixel[2]=f->pixel[3]=255;
    f->tex=(UgTexture){.key=1,.w=1,.h=1,.rgba=f->pixel};
    f->face=(UgFace){.name="conduitmditc_tt21i",.line_height=24,.cell_height=24,.tex=1};
    p->assets.tex.t=&f->tex;p->assets.tex.n=1;p->assets.face=&f->face;p->assets.nface=1;p->assets.main_face=0;
    qs_build(&f->qp);
    if(mode!=0){p->quit.pkg=f->qp.pk;p->quit.have_pkg=1;}
    pause_menu_open(p);
    p->state.selected=1;p->scroll=1;p->marker_x=pause_marker_target(p);     /* the quit item */
    p->original=1;                                                       /* allow the picture */
    f->frame=0;f->quit_frame=-1;f->last_result="none";
}
static void qd_close(QdFix *f) {
    PauseMenu *p=&f->p;
    pqd_drop_scene(&p->quit);
    ug_cv_free(&p->canvas);ug_strtab_free(&p->assets.strings);
    if(p->pill_initialized){ug_anim_free(&p->pill_pulse);ug_anim_free(&p->pill_glow);}
}
static void qd_frame(QdFix *f,float ms) {
    FeAction a=pause_menu_update(&f->p,ms*.001f);
    if(a==FE_ACTION_QUIT){f->quit_actions++;if(f->quit_frame<0)f->quit_frame=f->frame;}
    else if(a==FE_ACTION_RESUME)f->resume_actions++;
    else if(a!=FE_ACTION_NONE)f->other_actions++;
    f->frame++;
}
static void qd_key(QdFix *f,SDL_Keycode k,int repeat) {
    SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=k;e.key.repeat=(Uint8)repeat;pause_menu_event(&f->p,&e,NULL);
}
static void qd_pad(QdFix *f,int button) {
    SDL_Event e={0};e.type=SDL_CONTROLLERBUTTONDOWN;e.cbutton.button=(Uint8)button;pause_menu_event(&f->p,&e,NULL);
}
static void qd_mouse(QdFix *f,int type,int button) {
    SDL_Event e={0};e.type=type;e.button.button=(Uint8)button;pause_menu_event(&f->p,&e,NULL);
}
static void qd_wheel(QdFix *f) {
    SDL_Event e={0};e.type=SDL_MOUSEWHEEL;e.wheel.y=1;pause_menu_event(&f->p,&e,NULL);
}
static void qd_open_by_enter(QdFix *f) {qd_key(f,SDLK_RETURN,0);}

/* scenario events: key_* , pad_* , mouse_right_down/up, press/release/move/click:x:y (top-left virtual),
 * wheel, update[:ms], open (the opening input: Enter on the quit item), repeat_enter */
static int qd_event(QdFix *f,const char *name,float x,float y,int fields) {
    PauseMenu *p=&f->p;
    if(!strcmp(name,"open"))qd_open_by_enter(f);
    else if(!strcmp(name,"open_pill")){f->p.state.selected=1;pause_pointer_press(p,540,302);}
    else if(!strcmp(name,"key_left"))qd_key(f,SDLK_LEFT,0);
    else if(!strcmp(name,"key_right"))qd_key(f,SDLK_RIGHT,0);
    else if(!strcmp(name,"key_up"))qd_key(f,SDLK_UP,0);
    else if(!strcmp(name,"key_down"))qd_key(f,SDLK_DOWN,0);
    else if(!strcmp(name,"key_enter"))qd_key(f,SDLK_RETURN,0);
    else if(!strcmp(name,"key_kpenter"))qd_key(f,SDLK_KP_ENTER,0);
    else if(!strcmp(name,"key_escape"))qd_key(f,SDLK_ESCAPE,0);
    else if(!strcmp(name,"repeat_enter"))qd_key(f,SDLK_RETURN,1);
    else if(!strcmp(name,"repeat_left"))qd_key(f,SDLK_LEFT,1);
    else if(!strcmp(name,"repeat_escape"))qd_key(f,SDLK_ESCAPE,1);
    else if(!strcmp(name,"pad_left"))qd_pad(f,SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    else if(!strcmp(name,"pad_right"))qd_pad(f,SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    else if(!strcmp(name,"pad_up"))qd_pad(f,SDL_CONTROLLER_BUTTON_DPAD_UP);
    else if(!strcmp(name,"pad_a"))qd_pad(f,SDL_CONTROLLER_BUTTON_A);
    else if(!strcmp(name,"pad_b"))qd_pad(f,SDL_CONTROLLER_BUTTON_B);
    else if(!strcmp(name,"pad_back"))qd_pad(f,SDL_CONTROLLER_BUTTON_BACK);
    else if(!strcmp(name,"pad_start"))qd_pad(f,SDL_CONTROLLER_BUTTON_START);
    else if(!strcmp(name,"mouse_right_down"))qd_mouse(f,SDL_MOUSEBUTTONDOWN,SDL_BUTTON_RIGHT);
    else if(!strcmp(name,"mouse_right_up"))qd_mouse(f,SDL_MOUSEBUTTONUP,SDL_BUTTON_RIGHT);
    else if(!strcmp(name,"wheel"))qd_wheel(f);
    else if(fields==3 && !strcmp(name,"press"))pause_pointer_press(p,x,y);
    else if(fields==3 && !strcmp(name,"release"))pause_pointer_release(p,x,y);
    else if(fields==3 && !strcmp(name,"move"))pause_pill_hover(p,x,y);
    else if(fields==3 && !strcmp(name,"click")){pause_pointer_press(p,x,y);pause_pointer_release(p,x,y);}
    else if(!strcmp(name,"update"))qd_frame(f,fields==3?x:16.667f);
    else return 0;
    return 1;
}
static int qd_run(QdFix *f,const char *events) {
    if(!strcmp(events,"-"))return 1;
    char *copy=strdup(events),*save=NULL;int ok=1;
    for(char *it=strtok_r(copy,",",&save);it && ok;it=strtok_r(NULL,",",&save)) {
        char name[32]={0};float x=0,y=0;
        int fields=sscanf(it,"%31[^:]:%f:%f",name,&x,&y);
        ok=qd_event(f,name,x,y,fields==2 && !strcmp(name,"update")?3:fields);
        assert(ok);
    }
    free(copy);return ok;
}

static void qd_rgba(const UgNode *n,const char *key) {
    printf("\"%s\":[%d,%d,%d,%d]",key,(int)(n->props.w[0]+.5f),(int)(n->props.w[1]+.5f),(int)(n->props.w[2]+.5f),(int)(n->props.w[3]+.5f));
}
/* quit-scenario <mode> <events> : prints the observable state as one JSON line. */
static void quit_scenario(int mode,const char *events) {
    QdFix *f=calloc(1,sizeof *f);assert(f);qd_fixture(f,mode);
    qd_run(f,events);
    PauseMenu *p=&f->p;PauseQuitDialog *q=&p->quit;
    const char *result=q->result==PQD_CONFIRM?"confirm":q->result==PQD_CANCEL?"cancel":NULL;
    /* the pause screen still has input iff a probe key moves its selection */
    printf("{\"dialog_open\":%s,\"focus\":\"%s\",\"result\":%s%s%s,\"quit_invoked\":%d,\"quit_frame\":%d,"
           "\"confirms\":%d,\"cancels\":%d,\"opens\":%d,\"resume_actions\":%d,\"other_actions\":%d,"
           "\"pause_still_open\":%s,\"game_paused\":%s,\"pause_resumed\":%s,\"selected\":%d,\"marker\":%.1f,"
           "\"pills_drawn\":%s,\"pill_hover\":%d,\"pending\":%d,\"press\":%d,\"picture\":%s",
           q->open?"true":"false",q->focus?"right_button_negative":"left_button_positive",
           result?"\"":"",result?result:"null",result?"\"":"",
           f->quit_actions+(p->state.pending==FE_ACTION_QUIT),f->quit_frame,q->confirms,q->cancels,q->opens,
           f->resume_actions,f->other_actions,p->open?"true":"false",p->open && !f->resume_actions && !f->quit_actions?"true":"false",
           f->resume_actions?"true":"false",p->state.selected,p->marker_x,pause_pills_visible(p)?"true":"false",
           p->pill_hover,(int)p->state.pending,q->press,q->visual?"true":"false");
    if(q->loaded) {
        const UgScene *s=&q->scene;
        const UgNode *lb=&s->nodes[q->node[PQD_ID_LEFT_BTN]],*rb=&s->nodes[q->node[PQD_ID_RIGHT_BTN]];
        printf(",\"left_button_alpha\":%d,\"right_button_alpha\":%d,",(int)(lb->props.w[3]+.5f),(int)(rb->props.w[3]+.5f));
        qd_rgba(&s->nodes[q->node[PQD_ID_LEFT_LBL]],"left_label_rgba");printf(",");
        qd_rgba(&s->nodes[q->node[PQD_ID_RIGHT_LBL]],"right_label_rgba");
    }
    printf("}\n");
    qd_close(f);free(f);
}

/* quit-presentation <events> : the synthetic authored scene after the events, with the node data a
 * checker needs (focus responses, hidden objects, text overrides). */
static void quit_presentation(const char *events) {
    QdFix *f=calloc(1,sizeof *f);assert(f);qd_fixture(f,1);
    qd_run(f,events);
    PauseQuitDialog *q=&f->p.quit;
    assert(q->open && q->loaded && q->visual);
    const UgScene *s=&q->scene;
    const UgNode *msg=&s->nodes[q->node[PQD_ID_MESSAGE]],*ll=&s->nodes[q->node[PQD_ID_LEFT_LBL]],*rl=&s->nodes[q->node[PQD_ID_RIGHT_LBL]];
    printf("{\"focus_node\":%d,\"right_node\":%d,\"left_node\":%d,\"message\":\"%s\",\"left_label\":\"%s\",\"right_label\":\"%s\",\"hidden\":[",
           s->focus,q->node[PQD_ID_RIGHT_BTN],q->node[PQD_ID_LEFT_BTN],msg->text_override,ll->text_override,rl->text_override);
    for(int k=PQD_ID_UNUSED_FIRST;k<PQD_ID_COUNT;k++)printf("%s%d",k>PQD_ID_UNUSED_FIRST?",":"",s->nodes[q->node[k]].visible);
    printf("],\"left_button_alpha\":%d,\"right_button_alpha\":%d,",(int)(s->nodes[q->node[PQD_ID_LEFT_BTN]].props.w[3]+.5f),
           (int)(s->nodes[q->node[PQD_ID_RIGHT_BTN]].props.w[3]+.5f));
    qd_rgba(ll,"left_label_rgba");printf(",");qd_rgba(rl,"right_label_rgba");
    printf(",\"left_label_script\":%u,\"right_label_script\":%u}\n",ug_scene_script_id(s,q->node[PQD_ID_LEFT_LBL]),ug_scene_script_id(s,q->node[PQD_ID_RIGHT_LBL]));
    qd_close(f);free(f);
}

/* quit-static : identities, keys, geometry and degradation facts that need no scenario. */
static void quit_static(void) {
    /* every string key of the vectors equals the table hash of its data label */
    assert(ug_hash_upper("CONFIRM_QUIT_GAME")==PQD_KEY_MESSAGE && ug_hash_upper("YES")==PQD_KEY_YES);
    assert(ug_hash_upper("NO")==PQD_KEY_NO && ug_hash_upper("MU_PAUSE_QUIT_TO_FE")==PQD_KEY_CAPTION);
    assert(ug_hash_upper("UI_COMMON_YES")==PQD_KEY_YES_ALT && ug_hash_upper("UI_COMMON_NO")==PQD_KEY_NO_ALT);
    assert(ug_hash_upper(PQD_PACKAGE_NAME)==PQD_PACKAGE_KEY);
    QdPackage *qp=malloc(sizeof *qp);assert(qp);qs_build(qp);
    int index[PQD_ID_COUNT];
    assert(pqd_resolve(&qp->pk,index));
    /* identity metadata mismatches and duplicated guids reject the package instead of picking one */
    qp->obj[5].name_hash^=1;assert(!pqd_resolve(&qp->pk,index));qp->obj[5].name_hash^=1;
    qp->obj[13].guid=qp->obj[14].guid;assert(!pqd_resolve(&qp->pk,index));qp->obj[13].guid=0x000DDFC1u;
    qp->obj[3].res_index=4;assert(!pqd_resolve(&qp->pk,index));qp->obj[3].res_index=5;
    UgFngObject saved=qp->obj[8];qp->obj[8].guid=0x12345u;assert(!pqd_resolve(&qp->pk,index));qp->obj[8]=saved;
    assert(pqd_resolve(&qp->pk,index));
    /* order independence: scene node k is package object k */
    UgFngObject tmp=qp->obj[4];qp->obj[4]=qp->obj[5];qp->obj[5]=tmp;
    assert(pqd_resolve(&qp->pk,index) && index[PQD_ID_LEFT_BTN]==5 && index[PQD_ID_RIGHT_BTN]==4);
    free(qp);
    /* button extents (inclusive), Q5.5 */
    float l[4],r[4];pause_quit_button_edges(0,l);pause_quit_button_edges(1,r);
    assert(fabsf(l[0]-34.3f)<1e-3 && fabsf(l[2]-303.7f)<1e-3 && fabsf(r[0]-336.3f)<1e-3 && fabsf(r[2]-605.7f)<1e-3);
    assert(fabsf(l[1]-293.0f)<1e-3 && fabsf(l[3]-334.4f)<1e-3 && l[1]==r[1] && l[3]==r[3]);
    assert(pqd_button_at(34.3f,293.0f)==0 && pqd_button_at(303.7f,334.4f)==0 && pqd_button_at(34.2f,300)==-1);
    assert(pqd_button_at(336.3f,293.0f)==1 && pqd_button_at(605.7f,334.4f)==1 && pqd_button_at(605.8f,300)==-1);
    assert(pqd_button_at(320,313)==-1 && pqd_button_at(100,292.9f)==-1 && pqd_button_at(100,334.5f)==-1);
    puts("{\"static\":true}");
}

/* quit-text <mode> : message/label text resolution and the pause caption. */
static void quit_text(int mode) {
    QdFix *f=calloc(1,sizeof *f);assert(f);qd_fixture(f,mode);
    qd_open_by_enter(f);
    PauseQuitDialog *q=&f->p.quit;
    printf("{\"dialog_open\":%s,\"picture\":%s,\"message\":%s%s%s,\"left\":%s%s%s,\"right\":%s%s%s,\"caption\":\"%s\",\"status\":\"%s\",\"caption_status\":\"%s\"}\n",
           q->open?"true":"false",q->visual?"true":"false",
           q->text[0]?"\"":"",q->text[0]?q->text[0]:"null",q->text[0]?"\"":"",
           q->text[1]?"\"":"",q->text[1]?q->text[1]:"null",q->text[1]?"\"":"",
           q->text[2]?"\"":"",q->text[2]?q->text[2]:"null",q->text[2]?"\"":"",
           pause_menu_caption(&f->p),pause_menu_quit_dialog_status(&f->p),pause_menu_caption_status(&f->p));
    qd_close(f);free(f);
}

/* quit-draw : drawing on a synthetic scene must never leave partial dialog geometry or an
 * application-made substitute; a missing resource reports unavailable. */
static void quit_draw(void) {
    QdFix *f=calloc(1,sizeof *f);assert(f);qd_fixture(f,1);
    PauseMenu *p=&f->p;
    qd_open_by_enter(f);
    assert(p->quit.open && p->quit.visual);
    ug_cv_clear(&p->canvas);
    pqd_draw(&p->quit,&p->ctx,&p->canvas);
    int with_dialog=p->canvas.n;
    /* the darkening quad is an authored untextured plain quad: drawn; texts with an empty atlas omit */
    printf("{\"drawn_vertices\":%d,\"visual_ok\":%s,\"picture_after\":%s}\n",with_dialog,
           ug_cv_visual_status(&p->canvas)==UG_VIS_OK?"true":"false",p->quit.visual?"true":"false");
    /* removing the dialog font makes the dialog unavailable at open, input still works */
    qd_close(f);
    qd_fixture(f,1);f->face.tex=0;
    qd_open_by_enter(f);
    assert(f->p.quit.open && !f->p.quit.visual && !f->p.quit.loaded);
    assert(strstr(pause_menu_quit_dialog_status(&f->p),"font"));
    qd_key(f,SDLK_LEFT,0);qd_key(f,SDLK_RETURN,0);
    assert(!f->p.quit.open && f->p.state.pending==FE_ACTION_QUIT);
    qd_close(f);free(f);
}

/* quit-identities / quit-constants : the identity table and keys the implementation binds, for comparison
 * with the vectors. */
static void quit_identities(void) {
    printf("[");
    for(int k=0;k<PQD_ID_COUNT;k++)
        printf("%s{\"hash\":\"0x%08X\",\"guid\":\"0x%08X\",\"parent\":\"0x%08X\",\"type\":%d,\"resource\":%d}",k?",":"",
               pqd_identities[k].hash,pqd_identities[k].guid,pqd_identities[k].parent,pqd_identities[k].type,pqd_identities[k].resource);
    printf("]\n");
}
static void quit_constants(void) {
    printf("{\"package_key\":\"0x%08X\",\"package_name\":\"%s\",\"message\":\"0x%08X\",\"yes\":\"0x%08X\",\"no\":\"0x%08X\","
           "\"yes_alt\":\"0x%08X\",\"no_alt\":\"0x%08X\",\"caption\":\"0x%08X\"}\n",
           PQD_PACKAGE_KEY,PQD_PACKAGE_NAME,PQD_KEY_MESSAGE,PQD_KEY_YES,PQD_KEY_NO,PQD_KEY_YES_ALT,PQD_KEY_NO_ALT,PQD_KEY_CAPTION);
}
/* quit-track <t:v,t:v,...> <times...> : alpha of a synthetic one-track script (base SPEC 3.5 semantics). */
static void quit_track(int argc,char **argv) {
    UgKey keys[8];int n=0;
    char *copy=strdup(argv[2]),*save=NULL;
    for(char *it=strtok_r(copy,",",&save);it && n<8;it=strtok_r(NULL,",",&save)) {
        int t;float v;assert(sscanf(it,"%d:%f",&t,&v)==2);
        keys[n++]=(UgKey){.t=t,.v={255,255,255,v}};
    }
    free(copy);
    UgTrack tr={.type=UG_VT_COLOR,.vsize=4,.length=(uint32_t)keys[n-1].t,.prop=UG_P_COLOR,.keys=keys,.nkeys=n};
    printf("[");
    for(int i=3;i<argc;i++){float out[4];ug_track_eval(&tr,(float)atof(argv[i]),out);printf("%s%.0f",i>3?",":"",out[3]);}
    printf("]\n");
}
