/* Reviewed RESTORATION_PC_PILLS cases 1–7; synthetic handles/metrics only. */
static void test_a5_default_and_profiles(void) {
    static DemoApp app; DemoApp *a=&app; demo_init(a,g_vec_dir);
    CHECK(a->controller.family==UG_CONTROLLER_PC&&a->ui.hints.mode==UG_HINTS_PC);
    a->controller.available=1; /* Supplying PS2 resources must not change selection. */
    for(int i=0;i<19;i++)a->controller.icons[i]=100+i;
    orig_post_frame(a);CHECK(a->ui.hints.mode==UG_HINTS_PC);
    CHECK(demo_select_controller(a,UG_CONTROLLER_PS2)==UG_VIS_OK);
    CHECK(a->ui.hints.mode==UG_HINTS_STRIP&&strstr(a->glyphs.mode_btn.label,"PS2"));
    CHECK(demo_select_controller(a,UG_CONTROLLER_PC)==UG_VIS_OK);
    CHECK(a->ui.hints.mode==UG_HINTS_PC&&strstr(a->root.hint_toggle.label,"PC"));
    CHECK(strstr(a->glyphs.mode_btn.label,"PC"));
    CHECK(demo_select_controller(a,UG_CONTROLLER_XBOX)==UG_VIS_UNAVAILABLE);
    CHECK(strstr(a->root.hint_toggle.label,"Xbox")&&strstr(a->glyphs.mode_btn.label,"Xbox"));
    demo_cycle_hint_mode(a);CHECK(a->controller.family==UG_CONTROLLER_PC);
    demo_cycle_hint_mode(a);CHECK(a->controller.family==UG_CONTROLLER_PS2);
    demo_free(a);
}
static void test_a5_original_pill_art(void) {
    UgCtx c;ug_ctx_init(&c);UgHintBar h;ug_hintbar_init(&h);h.mode=UG_HINTS_PC;
    UgHintSet s;mkhints(&s,1,1,0,0);ug_hintbar_set(&h,&s);ug_hintbar_layout(&h,&c);
    UgSkin skin={0};skin.corner_b=21;skin.box_b=22;skin.pill=23;skin.pill_glow=24;
    ug_kit_set_skin(&skin);ug_text_set_face(NULL);UgCanvas cv;ug_cv_init(&cv);
    ug_hintbar_draw(&h,&c,&cv);int boxes=0,pills=0;
    for(int i=0;i<cv.n;i++) {
        CHECK(cv.v[i].tex>=21&&cv.v[i].tex<=23);CHECK(cv.v[i].y>=189.2f);
        if(cv.v[i].tex==22)boxes++;
        if(cv.v[i].tex==23)pills++;
    }
    CHECK(boxes==24&&pills==12); /* Four box pieces, two pill texture quads. */
    CHECK(ug_cv_visual_status(&cv)==UG_VIS_UNAVAILABLE); /* Missing font. */
    int over=0;ug_hintbar_pointer(&h,&c,&(UgPtr){.type=UG_PT_MOVE,.x=223,.y=203},&over);
    CHECK(over&&h.hover==1&&ug_anim_running(&h.hover_pulse)&&ug_anim_running(&h.hover_glow));
    ug_hintbar_update(&h,&c,500);CHECK(ug_anim_color(&h.hover_pulse).a==255);
    ug_cv_clear(&cv);ug_hintbar_draw(&h,&c,&cv);int glow=0;
    for(int i=0;i<cv.n;i++)if(cv.v[i].tex==24)glow++;
    CHECK(glow==6);
    CHECK(ug_hintbar_pointer(&h,&c,&(UgPtr){.type=UG_PT_DOWN,.button=1,.x=151.1f,.y=203},&over)==UG_A_ACCEPT&&over);
    CHECK(ug_hintbar_pointer(&h,&c,&(UgPtr){.type=UG_PT_UP,.button=1,.x=223,.y=203},&over)==UG_A_NONE);
    CHECK(ug_hintbar_pointer(&h,&c,&(UgPtr){.type=UG_PT_DOWN,.button=1,.x=76,.y=203},&over)==UG_A_NONE&&!over);
    CHECK(h.hover==-1&&!ug_anim_running(&h.hover_glow));
    ug_kit_set_skin(NULL);ug_cv_clear(&cv);ug_hintbar_draw(&h,&c,&cv);
    CHECK(cv.n==0&&ug_cv_visual_status(&cv)==UG_VIS_UNAVAILABLE);
    ug_cv_free(&cv);ug_hintbar_free(&h);
}
static void test_a5_click_once_and_modal(void) {
    UgUi u;ug_ui_init(&u);UgScreen s;ug_screen_init(&s,"Synthetic root",UG_SCR_ROOT);
    ug_hints_add(&s.hints,UG_A_ACCEPT,"Select",90);ug_ui_push(&u,&s);ug_ui_update(&u,300);
    CHECK(u.hints.mode==UG_HINTS_PC&&u.hints.pc.n==2);
    ug_ui_pointer(&u,&(UgPtr){.type=UG_PT_DOWN,.button=1,.x=223,.y=203});
    ug_ui_pointer(&u,&(UgPtr){.type=UG_PT_UP,.button=1,.x=223,.y=203});
    UgUiEvent e;CHECK(ug_ui_poll(&u,&e)&&e.kind==UG_EV_HINT&&e.a==UG_A_ACCEPT);CHECK(!ug_ui_poll(&u,&e));
    ug_ui_pointer(&u,&(UgPtr){.type=UG_PT_DOWN,.button=1,.x=223,.y=226});
    ug_ui_pointer(&u,&(UgPtr){.type=UG_PT_UP,.button=1,.x=223,.y=226});
    CHECK(ug_ui_poll(&u,&e)&&e.kind==UG_EV_QUIT);CHECK(!ug_ui_poll(&u,&e));
    UgScreen modal;ug_screen_init(&modal,"Synthetic dialog",UG_SCR_MODAL);ug_ui_push(&u,&modal);ug_ui_update(&u,300);
    CHECK(!ug_ui_hints_visible(&u));ug_ui_pointer(&u,&(UgPtr){.type=UG_PT_DOWN,.button=1,.x=223,.y=226});
    CHECK(!ug_ui_poll(&u,&e));ug_ui_pop(&u);ug_ui_update(&u,300);CHECK(ug_ui_hints_visible(&u));
    CHECK(u.hints.pc.n==2&&u.hints.pc.p[1].action==UG_A_ACCEPT);
    u.hide_hints=1;ug_ui_pointer(&u,&(UgPtr){.type=UG_PT_DOWN,.button=1,.x=223,.y=226});CHECK(!ug_ui_poll(&u,&e));
    ug_screen_free(&modal);ug_screen_free(&s);ug_ui_free(&u);
}
static void test_a5_overlay_eligibility(void) {
    size_t len=0;uint8_t *bin=read_hex("minimal_package.hex",&len);CHECK(bin!=NULL);if(!bin)return;
    static DemoApp app;DemoApp *a=&app;demo_init(a,g_vec_dir);
    UgAssetPackage p={0};p.name="UI_PC_Help_Bar.fng";p.data=bin;p.len=len;
    a->have_assets=1;a->assets.pk=&p;a->assets.npk=1;
    CHECK(demo_open_original(a,0));CHECK(a->viewer.vw.loaded&&!a->viewer.vw.suppressed);
    for(int i=0;i<40;i++)demo_update(a,16);
    CHECK(a->ui.hints.no_bg);
    demo_select_controller(a,UG_CONTROLLER_PS2);orig_post_frame(a);
    CHECK(a->viewer.vw.suppressed&&!a->viewer.vw.loaded);
    demo_select_controller(a,UG_CONTROLLER_PC);orig_post_frame(a);
    CHECK(a->viewer.vw.loaded&&!a->viewer.vw.suppressed&&a->ui.hints.no_bg);
    a->have_assets=0;a->assets.pk=NULL;a->assets.npk=0;demo_free(a);free(bin);
}
static void run_amend5_tests(void) {
    RUN(test_a5_default_and_profiles);RUN(test_a5_original_pill_art);
    RUN(test_a5_click_once_and_modal);RUN(test_a5_overlay_eligibility);
    RUN(test_a3_pc_bar_in_ui);RUN(test_a3_demo_quit_flow);
}
