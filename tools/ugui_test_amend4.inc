/* Tests from approved amendment 4 synthetic vectors. */
#include "ugui_controller.h"
static void test_corner_vectors(void) {
    char *txt = read_file("corner_transform_vectors.json", NULL); CHECK(txt != NULL); if (!txt) return;
    jp = txt; J *root = jparse(); const J *cases = jget(root, "cases");
    for (int i = 0; i < cases->n; i++) {
        const J *v = cases->kids[i], *in = jget(v, "input"), *q = jget(in, "quaternion"), *ex = jget(v, "expected_vertices");
        UgFngObject ob = {0}; ob.type = UG_OBJ_IMAGE; ob.guid = 1; ob.color = UG_WHITE;
        ob.size[0] = jnum(in, "width"); ob.size[1] = jnum(in, "height"); ob.size[2] = 1;
        for (int a = 0; a < 4; a++) ob.rot[a] = q->kids[a]->num;
        ob.uv[2] = ob.uv[3] = 1;
        UgFngPackage pk = {0}; pk.obj = &ob; pk.nobj = 1;
        UgScene sc; CHECK(ug_scene_init(&sc, &pk, NULL)); sc.nodes[0].tex_override = 1;
        UgCtx c; ug_ctx_init(&c); UgCanvas cv; ug_cv_init(&cv); ug_scene_draw(&sc, &c, &cv);
        CHECK(cv.n == 6); static const int idx[] = {0,1,2,5};
        if (cv.n == 6) for (int a = 0; a < 4; a++) {
            const J *xy = jget(ex->kids[a], "xy"), *uv = jget(ex->kids[a], "uv"); UgVertex *vv = &cv.v[idx[a]];
            CHECK(fabs(vv->x - xy->kids[0]->num) < 0.001f); CHECK(fabs(vv->y - xy->kids[1]->num) < 0.001f);
            CHECK(fabs(vv->lx - uv->kids[0]->num) < 0.001f); CHECK(fabs(vv->ly - uv->kids[1]->num) < 0.001f);
        }
        ug_cv_free(&cv); ug_scene_free(&sc);
    }
    jfree(root); free(txt);
}
static void test_ps2_codec_vectors(void) {
    char *txt = read_file("ps2_icon_codec_vectors.json", NULL); CHECK(txt != NULL); if (!txt) return;
    jp = txt; J *r = jparse(); size_t pn, cn, en; uint8_t *p = unhex(jstrv(r,"pixel_hex"), &pn);
    uint8_t *pal = unhex(jstrv(r,"palette_hex"), &cn), *indices = unhex(jstrv(r,"expected_index_hex"), &en);
    uint8_t rgba[4096]; CHECK(ug_ps2_icon_decode(p,pn,pal,cn,rgba)); CHECK(en == 1024);
    for (int i = 0; i < 1024; i++) {
        int k = indices[i]; for (int ch = 0; ch < 3; ch++) CHECK(rgba[4*i+ch] == pal[4*k+ch]);
        CHECK(rgba[4*i+3] == (pal[4*k+3] >= 128 ? 255 : 2*pal[4*k+3]));
    }
    const J *samples = jget(r,"samples"); for (int i = 0; i < samples->n; i++) {
        const J *s = samples->kids[i], *rgb = jget(s,"rgba"); int x = jnum(s,"x"), y = jnum(s,"y");
        CHECK(ug_ps2_icon_address(x,y) == (unsigned)jnum(s,"expanded_address"));
        for(int ch=0;ch<4;ch++) CHECK(rgba[(y*32+x)*4+ch] == rgb->kids[ch]->num);
    }
    CHECK(!ug_ps2_icon_decode(p,pn-1,pal,cn,rgba)); CHECK(!ug_ps2_icon_decode(p,pn,pal,cn-1,rgba));
    const char *outpath=getenv("UGUI_VECTOR_RGBA_FILE");
    if(outpath){FILE *out=fopen(outpath,"wb");CHECK(out!=NULL);if(out){CHECK(fwrite(rgba,1,sizeof rgba,out)==sizeof rgba);fclose(out);}}
    free(p); free(pal); free(indices); jfree(r); free(txt);
}
static void test_original_visual_policy(void) {
    char *txt = read_file("original_visual_policy_vectors.json",NULL); CHECK(txt != NULL); if(txt) { jp=txt; J *r=jparse(); CHECK(jget(r,"cases")->n == 7); jfree(r); free(txt); }
    UgController p; ug_controller_init(&p); CHECK(p.family == UG_CONTROLLER_PC);
    CHECK(ug_controller_select(&p,UG_CONTROLLER_XBOX) == UG_VIS_UNAVAILABLE); CHECK(ug_controller_token(&p,"ACCEPT",NULL) == UG_VIS_UNBOUND);
    UgCanvas cv; ug_cv_init(&cv); ug_kit_set_skin(NULL); ug_text_set_face(NULL);
    ug_kit_symbol(&cv,0,0,0,32,UG_WHITE); ug_kit_cursor(&cv,0,0); ug_kit_panel(&cv,(UgRect){-50,-50,100,100},1,1);
    ug_glyph_draw(&cv,(UgGlyph){UG_G_MOUSE_LEFT,0},0,0,32,UG_WHITE); ug_text_draw(&cv,"Missing font",0,0,1,UG_WHITE,0,0,0,UG_GLYPHS_MOUSE);
    CHECK(cv.n == 0); CHECK(ug_cv_visual_status(&cv)==UG_VIS_UNAVAILABLE);
    UgHintBar h; ug_hintbar_init(&h); UgCtx c; ug_ctx_init(&c); h.mode=UG_HINTS_STRIP; ug_hintbar_layout(&h,&c); int over;
    CHECK(ug_hintbar_pointer(&h,&c,&(UgPtr){.type=UG_PT_DOWN,.button=1,.x=223,.y=226},&over)==UG_A_NONE); CHECK(!over);
    ug_hintbar_free(&h); ug_cv_free(&cv);
    UgUi ui; ug_ui_init(&ui); UgScreen scr; ug_screen_init(&scr,"Synthetic",0); ug_ui_push(&ui,&scr); ug_ui_update(&ui,300);
    UgScreen child; ug_screen_init(&child,"Child",0); ug_ui_push(&ui,&child); ug_ui_update(&ui,300);
    ug_ui_pointer(&ui,&(UgPtr){.type=UG_PT_DOWN,.button=2,.x=0,.y=0}); ug_ui_update(&ui,300); ug_ui_update(&ui,300); CHECK(ui.sp==1);
    ug_screen_free(&child); ug_screen_free(&scr); ug_ui_free(&ui);
}
static void test_ps2_container_bounds(void);
static void test_v4_presentation(void);
static void test_v4_overlay_status(void);
static void run_amend4_tests(void) { RUN(test_ps2_container_bounds); RUN(test_corner_vectors); RUN(test_ps2_codec_vectors); RUN(test_original_visual_policy); RUN(test_v4_presentation); RUN(test_v4_overlay_status); }
/* Additional independently authored container tests, from 15 and ECMA-119. */
static void a4_u32(uint8_t *b,uint32_t v){for(int i=0;i<4;i++)b[i]=(uint8_t)(v>>(8*i));}
static void a4_chunk(uint8_t *b,uint32_t id,uint32_t n){a4_u32(b,id);a4_u32(b+4,n);}
static unsigned a4_dir_record(uint8_t *b,const char *name,uint32_t sector,uint32_t n,int dir){
    unsigned l=(unsigned)strlen(name),size=33+l+(!(l&1));memset(b,0,size);b[0]=(uint8_t)size;
    a4_u32(b+2,sector);a4_u32(b+10,n);b[25]=dir?2:0;b[28]=1;b[32]=(uint8_t)l;memcpy(b+33,name,l);return size;
}
static void test_ps2_container_bounds(void){
    char *txt=read_file("ps2_icon_codec_vectors.json",NULL);CHECK(txt!=NULL);if(!txt)return;
    jp=txt;J *r=jparse();size_t pn,cn;uint8_t *pixels=unhex(jstrv(r,"pixel_hex"),&pn),*pal=unhex(jstrv(r,"palette_hex"),&cn);
    size_t len=16512,rec=316;uint8_t *pack=calloc(1,len);CHECK(pack!=NULL);if(!pack)return;
    a4_chunk(pack,0xB3300000u,(uint32_t)len-8);a4_chunk(pack+8,0xB3310000u,4260);
    a4_chunk(pack+16,0x33310001u,124);a4_u32(pack+24,5);memcpy(pack+56,"Global\\GlobalTexturesPSX2.tpk",30);
    a4_chunk(pack+148,0x33310002u,152);a4_chunk(pack+308,0x33310004u,2356);
    a4_chunk(pack+2672,0x33310005u,1596);a4_chunk(pack+4276,0xB3320000u,(uint32_t)len-4284);
    a4_chunk(pack+4284,0x33320002u,(uint32_t)len-4292);
    for(int i=0;i<19;i++){
        a4_u32(pack+156+8*i,ug_controller_icon_key(i));uint8_t *b=pack+rec+124*i;
        memcpy(b+12,ug_controller_icon_name(i),strlen(ug_controller_icon_name(i)));a4_u32(b+36,ug_controller_icon_key(i));
        a4_u32(b+48,640*i);a4_u32(b+52,640*i+512);a4_u32(b+56,512);a4_u32(b+60,64);a4_u32(b+64,512);b[68]=b[70]=32;b[74]=4;b[77]=b[78]=1;
        memcpy(pack+4352+640*i,pixels,512);memcpy(pack+4352+640*i+512,pal,64);
    }
    UgController p;UgTexLib lib;ug_controller_init(&p);ug_texlib_init(&lib);
    CHECK(ug_controller_load_pack(&p,&lib,pack,len));CHECK(p.available&&lib.n==19);
    CHECK(ug_controller_select(&p,UG_CONTROLLER_PS2)==UG_VIS_OK);
    CHECK(ug_controller_token(&p,"ACCEPT",NULL)==UG_VIS_UNBOUND);
    CHECK(ug_controller_bind(&p,"ACCEPT",ug_controller_icon_key(11))==UG_VIS_OK);int tex=0;CHECK(ug_controller_token(&p,"ACCEPT",&tex)==UG_VIS_OK&&tex==p.icons[11]);
    CHECK(ug_controller_select(&p,UG_CONTROLLER_XBOX)==UG_VIS_UNAVAILABLE);CHECK(ug_controller_token(&p,"ACCEPT",&tex)==UG_VIS_UNAVAILABLE&&tex==0);ug_texlib_free(&lib);
    ug_controller_init(&p);ug_texlib_init(&lib);
    CHECK(!ug_controller_load_pack(&p,&lib,pack,len-1));CHECK(!lib.n&&!p.available);
    uint8_t old=pack[rec+74];pack[rec+74]=8;CHECK(!ug_controller_load_pack(&p,&lib,pack,len));pack[rec+74]=old;
    a4_u32(pack+rec+48,(uint32_t)len);CHECK(!ug_controller_load_pack(&p,&lib,pack,len));a4_u32(pack+rec+48,0);
    a4_u32(pack+2676,1595);CHECK(!ug_controller_load_pack(&p,&lib,pack,len));a4_u32(pack+2676,1596);
    CHECK(!lib.n);ug_texlib_free(&lib);
    /* ISO file at a deliberately chosen synthetic sector; no hardcoded retail sector. */
    size_t ilen=30*2048+len;uint8_t *iso=calloc(1,ilen);uint8_t *vd=iso+16*2048;
    vd[0]=1;memcpy(vd+1,"CD001",5);vd[6]=1;vd[129]=8;a4_u32(vd+158,20);a4_u32(vd+166,2048);
    a4_dir_record(iso+20*2048,"NFSUNDER",21,2048,1);a4_dir_record(iso+21*2048,"ZZDATA1.BIN;1",30,(uint32_t)len,0);
    memcpy(iso+30*2048,pack,len);char path[512];temp_path(path,"ugui_iso_test_XXXXXX");int fd=mkstemp(path);CHECK(fd>=0);
    if(fd>=0){FILE *file=fdopen(fd,"wb");CHECK(fwrite(iso,1,ilen,file)==ilen);fclose(file);
        ug_controller_init(&p);ug_texlib_init(&lib);CHECK(ug_controller_load_ps2(&p,&lib,path));CHECK(lib.n==19);ug_texlib_free(&lib);
        /* declared ISO file extent excludes the candidate's final byte */
        a4_u32(iso+21*2048+10,(uint32_t)len-1);file=fopen(path,"wb");CHECK(fwrite(iso,1,ilen,file)==ilen);fclose(file);
        ug_controller_init(&p);ug_texlib_init(&lib);CHECK(!ug_controller_load_ps2(&p,&lib,path));CHECK(lib.n==0);ug_texlib_free(&lib);remove(path);
    }
    free(iso);free(pack);free(pixels);free(pal);jfree(r);free(txt);
}

/* Reviewed SPEC 20.6 / V4-1..V4-3: configuration labels, original strip ownership,
 * and text ink bounds. Synthetic font metrics only; no artwork is generated. */
static void test_v4_presentation(void) {
    static DemoApp app; DemoApp *a=&app; demo_init(a,g_vec_dir);
    demo_select_controller(a,UG_CONTROLLER_PS2);
    CHECK(a->ui.hints.mode==UG_HINTS_STRIP);
    UgSkin skin={0};skin.cap=2;skin.box_b=3;ug_kit_set_skin(&skin);
    UgCanvas strip;ug_cv_init(&strip);ug_text_set_face(NULL);
    /* No controller source and no action map: four original strip pieces only. */
    ug_hintbar_draw(&a->ui.hints,&a->ui.ctx,&strip);CHECK(strip.n==24);
    for(int i=0;i<strip.n;i++)CHECK(strip.v[i].y>=190&&strip.v[i].y<=222);
    ug_cv_free(&strip);ug_kit_set_skin(NULL);
    CHECK(demo_select_controller(a,UG_CONTROLLER_XBOX)==UG_VIS_UNAVAILABLE);
    CHECK(strstr(a->root.hint_toggle.label,"Xbox")!=NULL);
    CHECK(strstr(a->glyphs.mode_btn.label,"Xbox")!=NULL);
    CHECK(strstr(a->controller.status,"Xbox")!=NULL);
    int tex=0;CHECK(ug_controller_token(&a->controller,"ACCEPT",&tex)!=UG_VIS_OK&&tex==0);
    a->controller.available=1;
    for(int i=0;i<19;i++)a->controller.icons[i]=100+i;
    CHECK(demo_select_controller(a,UG_CONTROLLER_PS2)==UG_VIS_OK);
    CHECK(strstr(a->root.hint_toggle.label,"PS2")!=NULL);
    CHECK(strstr(a->glyphs.mode_btn.label,"PS2")!=NULL);
    demo_cycle_hint_mode(a);
    CHECK(a->controller.family==UG_CONTROLLER_XBOX&&strstr(a->glyphs.mode_btn.label,"Xbox")!=NULL);
    demo_cycle_hint_mode(a);
    CHECK(a->controller.family==UG_CONTROLLER_PC&&strstr(a->root.hint_toggle.label,"PC")!=NULL);
    demo_select_controller(a,UG_CONTROLLER_PS2);
    UgFace f=*metric_face();UgFGlyph glyphs[95];memset(glyphs,0,sizeof glyphs);
    f.g=glyphs;f.ng=95;f.tex=1;
    for(int i=0;i<95;i++){glyphs[i].cp=32+i;glyphs[i].w=12;glyphs[i].h=26;glyphs[i].adv=13;glyphs[i].yoff=4;}
    UgCanvas cv;ug_cv_init(&cv);ug_text_set_face(&f);
    UgScreen *s=&a->glyphs.scr;s->draw_bg(s,&a->ui.ctx,&cv);
    int icons=0;float text_top=1e9f,text_bottom=-1e9f;
    for(int i=0;i<cv.n;i++) {
        const UgVertex *v=&cv.v[i];
        if(v->tex>=100&&v->tex<119)icons++;
        if(v->tex==f.tex&&v->y>120){if(v->y<text_top)text_top=v->y;if(v->y>text_bottom)text_bottom=v->y;}
    }
    CHECK(icons==19*6);CHECK(text_bottom>text_top);
    ug_cv_clear(&cv);a->glyphs.mode_btn.w.vt->draw(&a->glyphs.mode_btn.w,&a->ui.ctx,&cv);
    Ink label=ink_of(&cv,0);CHECK(label.n>0);
    /* The virtual 640x480 canvas scales by 2 at the required 1280x960 size. */
    CHECK(label.y1*2<=text_top*2 || text_bottom*2<=label.y0*2);
    demo_select_controller(a,UG_CONTROLLER_XBOX);ug_cv_clear(&cv);s->draw_bg(s,&a->ui.ctx,&cv);
    for(int i=0;i<cv.n;i++)CHECK(cv.v[i].tex<100||cv.v[i].tex>=119);
    ug_text_set_face(NULL);ug_cv_free(&cv);
    /* An authored strip owns the background; otherwise the library provides it. */
    UgFngObject ob={0};ob.type=UG_OBJ_IMAGE;ob.guid=1;ob.color=UG_WHITE;
    ob.rot[3]=1;ob.size[0]=385;ob.size[1]=32;ob.size[2]=1;ob.pos[1]=206;ob.flags=UG_OF_CONSOLE;
    UgFngPackage pk={0};pk.obj=&ob;pk.nobj=1;
    CHECK(ug_scene_init(&a->viewer.vw.sc,&pk,NULL));
    a->viewer.vw.loaded=1;
    snprintf(a->viewer.vw.name,sizeof a->viewer.vw.name,"Synthetic_Menu.fng");
    a->ui.stack[a->ui.sp++]=&a->viewer.scr;
    orig_post_frame(a);CHECK(a->ui.hints.mode==UG_HINTS_STRIP&&a->ui.hints.no_bg&&!a->ui.hide_hints);
    a->viewer.vw.sc.nodes[0].app_hidden=1;orig_post_frame(a);CHECK(!a->ui.hints.no_bg);
    ug_scene_free(&a->viewer.vw.sc);
    a->viewer.vw.loaded=0;a->ui.sp--;demo_free(a);
}

/* V4-4: suppression is distinct from malformed resources, and each open resets
 * status so a previous suppressed overlay cannot hide a later parse failure. */
static void test_v4_overlay_status(void) {
    static DemoApp app;DemoApp *a=&app;demo_init(a,g_vec_dir);
    demo_select_controller(a,UG_CONTROLLER_PS2);
    UgAssetPackage packs[2]={{0}};
    packs[0].name="UI_PC_Help_Bar.fng";packs[1].name="Synthetic_Broken.fng";
    a->have_assets=1;a->assets.pk=packs;a->assets.npk=2;
    CHECK(demo_open_original(a,0));
    CHECK(a->viewer.vw.suppressed&&!a->viewer.vw.loaded);
    CHECK(!a->viewer.vw.objects&&!a->viewer.vw.missing_tex);
    for(int i=0;i<80;i++)demo_update(a,16);
    ug_ui_action(&a->ui,UG_A_BACK);for(int i=0;i<80;i++)demo_update(a,16);
    CHECK(demo_open_original(a,1));
    CHECK(!a->viewer.vw.suppressed&&!a->viewer.vw.loaded&&a->viewer.vw.err[0]);
    a->have_assets=0;a->assets.pk=NULL;a->assets.npk=0;demo_free(a);
}
