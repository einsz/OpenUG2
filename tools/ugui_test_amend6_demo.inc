/* Reviewed Amendment 6 demo requirements, synthetic resources only. */
static void test_a6_demo_composition(void) {
    static DemoApp app; DemoApp *a=&app; demo_init(a,g_vec_dir);
    CHECK(!a->diagnostics);
    CHECK(a->package.example.w.id==DID_SCENE+1);
    const UgRect r=a->orig.br.sb.track;
    CHECK(r.x+r.w+20<=312); /* blink arrow silhouette + D margin */
    CHECK(a->controls.vscroll.w.bounds.x+a->controls.vscroll.w.bounds.w<=312);
    CHECK(!strcmp(a->pcctl.list.value_header,"Binding"));
    for(int p=0;p<=12;p++) CHECK(demo_page_screen(a,p)!=NULL);
    demo_open_page(a,DP_PACKAGE);for(int i=0;i<40;i++)demo_update(a,16);
    int n=a->package.sw.total;
    UgRect b=a->package.example.w.bounds;
    ug_ui_pointer(&a->ui,&(UgPtr){.type=UG_PT_DOWN,.button=1,.x=b.x+b.w/2,.y=b.y+b.h/2});
    demo_update(a,16);CHECK(a->package.sw.total==n+1);
    ug_ui_pointer(&a->ui,&(UgPtr){.type=UG_PT_UP,.button=1,.x=b.x+b.w/2,.y=b.y+b.h/2});
    demo_update(a,16);CHECK(a->package.sw.total==n+1);
    demo_free(a);
}
static void test_a6_dialog_filled_ink(void) {
    UgFace f=*metric_face();UgFGlyph glyphs[95]={0};f.g=glyphs;f.ng=95;f.tex=1;
    for(int i=0;i<95;i++){glyphs[i].cp=32+i;glyphs[i].w=12;glyphs[i].h=26;glyphs[i].adv=13;glyphs[i].yoff=4;}
    ug_text_set_face(&f);UgCtx c;ug_ctx_init(&c);UgCanvas cv;ug_cv_init(&cv);
    UgButton b;ug_button_init(&b,1,UG_BTN_DIALOG,UG_RECT_C(0,0,214,42),"Yes");
    b.w.vt->draw(&b.w,&c,&cv);int glyphs_n=0;
    for(int i=0;i<cv.n;i++)if(cv.v[i].tex==1){CHECK(cv.v[i].mode==UG_M_TEXT);CHECK(cv.v[i].a==255);CHECK(cv.v[i].r==150);glyphs_n++;}
    CHECK(glyphs_n==18);
    b.w.vt->destroy(&b.w);ug_cv_free(&cv);ug_text_set_face(NULL);
}
static void run_amend6_demo_tests(void){RUN(test_a6_demo_composition);RUN(test_a6_dialog_filled_ink);}
