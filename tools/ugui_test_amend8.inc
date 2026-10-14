/* GUI_CORRECTIONS_ACCEPTANCE H-1..H-5/G-1..G-7, independent synthetic inputs.
 * Final framebuffer acceptance is a separate --gui-check runtime operation. */
static void a8_idle(UgWidget *w, UgCtx *c) {
    for(int i=0;i<100;i++) w->vt->update(w,c,16);
}
static void a8_move(UgWidget *w,UgCtx *c,float x,float y) {
    w->vt->pointer(w,c,&(UgPtr){.type=UG_PT_MOVE,.x=x,.y=y});
    a8_idle(w,c);
}
static void test_a8_horizontal_hover(void) {
    /* All three profiles, both acceptance viewport mappings. No art needed. */
    for(int size=0;size<2;size++)for(int family=0;family<3;family++) {
        UgController p;ug_controller_init(&p);ug_controller_select(&p,(UgControllerFamily)family);ug_glyph_set_controller(&p);
        UgView view=ug_view_fit(1280,size?720:960);CHECK(view.scale>0);
        UgCtx c;ug_ctx_init(&c);UgIconScroller s;ug_icons_init(&s,9,"Synthetic categories");
        for(int k=0;k<5;k++)ug_icons_add(&s,"Synthetic item",k,0,k);
        ug_icons_select(&s,2);a8_idle(&s.w,&c);c.qh=c.qt=0;
        const float targets[][2]={{-158,-134},{0,-134},{158,-134},{-270,-132},{270,-132},{0,-172},{0,-171},{310,50}};
        for(int endpoint=0;endpoint<3;endpoint++) {
            int selected=endpoint==0?2:endpoint==1?0:4;
            ug_icons_select(&s,selected);a8_idle(&s.w,&c);c.qh=c.qt=0;
            for(size_t k=0;k<sizeof targets/sizeof targets[0];k++) {
                a8_move(&s.w,&c,targets[k][0],targets[k][1]);
                CHECK(s.sel==selected);CHECK_NEAR(s.pos,selected,0.001);CHECK(s.sb.index==selected);CHECK(c.qh==c.qt);
            }
            UgIconLayout L;ug_icons_layout(s.n,s.sel,&L);CHECK(L.arrow_l==(selected>0));CHECK(L.arrow_r==(selected<4));
        }
        ug_icons_select(&s,2);a8_idle(&s.w,&c);c.qh=c.qt=0;
        a8_move(&s.w,&c,158,-134);s.w.vt->action(&s.w,&c,UG_A_RIGHT);a8_idle(&s.w,&c);
        a8_move(&s.w,&c,158,-134);CHECK(s.sel==3);
        s.w.vt->action(&s.w,&c,UG_A_LEFT);a8_idle(&s.w,&c);a8_move(&s.w,&c,158,-134);CHECK(s.sel==2);
        s.w.vt->pointer(&s.w,&c,&(UgPtr){.type=UG_PT_WHEEL,.x=158,.y=-134,.wheel=-1});
        a8_move(&s.w,&c,158,-134);CHECK(s.sel==3);
        s.w.vt->pointer(&s.w,&c,&(UgPtr){.type=UG_PT_DOWN,.button=1,.x=-270,.y=-132});
        s.w.vt->pointer(&s.w,&c,&(UgPtr){.type=UG_PT_UP,.button=1,.x=-270,.y=-132});
        a8_move(&s.w,&c,-270,-132);CHECK(s.sel==2);
        c.qh=c.qt=0;
        s.w.vt->pointer(&s.w,&c,&(UgPtr){.type=UG_PT_DOWN,.button=1,.x=158,.y=-134});
        s.w.vt->pointer(&s.w,&c,&(UgPtr){.type=UG_PT_UP,.button=1,.x=158,.y=-134});
        a8_move(&s.w,&c,158,-134);CHECK(s.sel==3);
        UgUiEvent ev;int activations=0;while(ug_poll(&c,&ev))if(ev.kind==UG_EV_ACTIVATE)activations++;
        CHECK(activations==1);s.w.vt->destroy(&s.w);
        const UgCar cars[3]={{"A","Alpha",{.1f,.2f,.3f},0,UG_WHITE},{"B","Beta",{.4f,.5f,.6f},0,UG_WHITE},{"C","Gamma",{.7f,.8f,.9f},0,UG_WHITE}};
        const char *cats[]={"All"};UgCarSelect car;ug_carselect_init(&car,10,cars,3,cats,1);
        for(int sel=0;sel<3;sel++) {
            if(sel)car.w.vt->action(&car.w,&c,UG_A_RIGHT);
            a8_idle(&car.w,&c);c.qh=c.qt=0;
            for(size_t k=0;k<sizeof targets/sizeof targets[0];k++) {
                a8_move(&car.w,&c,targets[k][0],targets[k][1]);CHECK(car.cur==sel);CHECK(car.sb.index==sel);CHECK(c.qh==c.qt);
                for(int j=0;j<3;j++)CHECK_NEAR(car.bars.target[j],cars[sel].stats[j],0.001);
            }
        }
        car.w.vt->destroy(&car.w);
    }
    ug_glyph_set_controller(NULL);
}
static void test_a8_bind_aliases_and_profiles(void) {
    static DemoApp app;DemoApp *a=&app;demo_init(a,g_vec_dir);
    CHECK(a->controller.family==UG_CONTROLLER_PC);
    a->controller.available=a->controller.xbox_available=1;
    a->controller.icons[11]=101;a->controller.icons[10]=102;
    a->controller.xbox_icons[10]=201;a->controller.xbox_icons[11]=202;
    for(int f=0;f<2;f++) {
        UgControllerFamily family=f?UG_CONTROLLER_XBOX:UG_CONTROLLER_PS2;
        CHECK(demo_select_controller(a,family)==UG_VIS_OK);
        CHECK(ug_controller_bind(&a->controller,"SELECT",f?0x8522e0a0:ug_hash_upper("PS2_CROSS"))==UG_VIS_OK);
        CHECK(ug_controller_bind(&a->controller,"CANCEL",f?0x8522e0a1:ug_hash_upper("PS2_CIRCLE"))==UG_VIS_OK);
        UgGlyph g,other;ug_hint_glyphs(UG_A_ACCEPT,&g,&other);CHECK((int)g.id-UG_G_COUNT==(f?201:101));
        int tex=0;CHECK(ug_controller_token(&a->controller,"ACCEPT",&tex)==UG_VIS_OK&&tex==(f?201:101));
        CHECK(ug_controller_token(&a->controller,"HELP",&tex)==UG_VIS_UNBOUND&&tex==0);
        UgCanvas cv;ug_cv_init(&cv);a->glyphs.scr.draw_bg(&a->glyphs.scr,&a->ui.ctx,&cv);
        int quads=0;for(int k=0;k<cv.n;k++)if(cv.v[k].tex==(f?201:101)||cv.v[k].tex==(f?202:102)){CHECK(cv.v[k].a==255);quads++;}
        CHECK(quads==12);ug_cv_free(&cv);
        UgHintSet h;ug_hints_clear(&h);ug_hints_add(&h,UG_A_ACCEPT,"Accept",90);ug_hints_add(&h,UG_A_BACK,"Back",85);ug_hints_add(&h,UG_A_HELP,"Help",10);
        ug_hintbar_set(&a->ui.hints,&h);ug_hintbar_layout(&a->ui.hints,&a->ui.ctx);a->ui.hints.alpha=1;
        ug_cv_init(&cv);ug_hintbar_draw(&a->ui.hints,&a->ui.ctx,&cv);quads=0;
        for(int k=0;k<cv.n;k++)if(cv.v[k].tex==(f?201:101)||cv.v[k].tex==(f?202:102))quads++;
        CHECK(quads==12);ug_cv_free(&cv);
    }
    demo_select_controller(a,UG_CONTROLLER_PC);CHECK(a->ui.hints.mode==UG_HINTS_PC);
    demo_free(a);
}
static void test_a8_focus_and_bound_carousel(void) {
    static DemoApp app;demo_init(&app,g_vec_dir);
    UgCtx c;ug_ctx_init(&c);UgScreen screen;ug_screen_init(&screen,"Synthetic",0);
    UgButton button;ug_button_init(&button,1,UG_BTN_HINT,UG_RECT_C(100,50,60,20),"Standalone");
    UgIconScroller icons;ug_icons_init(&icons,2,"Synthetic");
    for(int i=0;i<3;i++)ug_icons_add(&icons,"Item",i,0,i);
    ug_screen_add(&screen,&button.w);ug_screen_add(&screen,&icons.w);ug_screen_set_focus(&screen,&c,0);
    ug_screen_pointer(&screen,&c,&(UgPtr){.type=UG_PT_MOVE,.x=158,.y=-134});CHECK(screen.focus==0);CHECK(icons.sel==0);
    ug_screen_set_focus(&screen,&c,1);
    ug_screen_pointer(&screen,&c,&(UgPtr){.type=UG_PT_MOVE,.x=100,.y=50});CHECK(screen.focus==0);
    ug_screen_free(&screen);
    SynMain m;DemoViewer v;syn_main_build(&m,&v);v.w.vt=app.viewer.vw.w.vt;
    CHECK(orig_bind_detect(&v));int textures[5]={1,2,3,4,5};char labels[5][40]={{"Alpha"},{"Beta"},{"Gamma"},{"Delta"},{"Epsilon"}};
    orig_bind_set_items(&v,5,textures,(const char (*)[40])labels);orig_bind_step(&v,&c,1);orig_bind_step(&v,&c,1);syn_main_settle(&v,100);
    int focus=v.sc.focus;float pos=v.bind.pos;const float xs[]={-158,0,158,-270,270,0};
    for(int k=0;k<6;k++) {
        v.w.vt->pointer(&v.w,&c,&(UgPtr){.type=UG_PT_MOVE,.x=xs[k],.y=k==5?-171:-134});
        syn_main_settle(&v,100);CHECK(v.bind.sel==2);CHECK_NEAR(v.bind.pos,pos,.001);CHECK(v.sc.focus==focus);
    }
    v.w.vt->action(&v.w,&c,UG_A_RIGHT);syn_main_settle(&v,100);
    v.w.vt->pointer(&v.w,&c,&(UgPtr){.type=UG_PT_MOVE,.x=158,.y=-134});syn_main_settle(&v,100);CHECK(v.bind.sel==3);
    ug_scene_free(&v.sc);free(m.obj);demo_free(&app);
}
static void run_amend8_tests(void){RUN(test_a8_horizontal_hover);RUN(test_a8_bind_aliases_and_profiles);RUN(test_a8_focus_and_bound_carousel);}
