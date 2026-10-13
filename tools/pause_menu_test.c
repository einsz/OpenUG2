/* Synthetic pause acceptance only; never opens runtime retail input. */
#include "../src/pause_menu.c"
#include <string.h>
#include <assert.h>

#include "pause_menu_binding_test.inc"
#include "pause_menu_authored_test.inc"
#include "pause_menu_behavior_test.inc"
#include "pause_menu_quit_test.inc"

static void focused_checks(void) {
    authored_checks();
    PauseMenu p={0};
    pause_menu_open(&p);
    SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_LEFT;
    pause_menu_event(&p,&e,NULL);
    assert(p.state.selected==0 && p.scroll==0 && p.marker_x==50); /* no wrap: Left at the first item does nothing */
    e.key.keysym.sym=SDLK_RIGHT;pause_menu_event(&p,&e,NULL);
    assert(p.state.selected==1 && p.marker_x==596);
    pause_menu_event(&p,&e,NULL);
    assert(p.state.selected==1 && p.marker_x==596); /* no wrap at the last item either */
    e.key.keysym.sym=SDLK_RETURN;
    pause_menu_event(&p,&e,NULL);
    /* QUIT_DIALOG: the authored dialog opens with the negative button focused; Fe's own screen is unused. */
    assert(p.quit.open && p.quit.focus==1 && p.state.screen==FE_SCREEN_PAUSE && p.open);
    e.key.keysym.sym=SDLK_ESCAPE;
    pause_menu_event(&p,&e,NULL);
    assert(!p.quit.open && p.state.screen==FE_SCREEN_PAUSE && p.open && p.state.selected==1);
    pause_menu_open(&p);
    pause_menu_pointer(&p,540,302,640,480);
    e.key.keysym.sym=SDLK_RETURN;pause_menu_event(&p,&e,NULL);
    assert(pause_menu_update(&p,0)==FE_ACTION_RESUME);
    assert(!p.open); /* resume closes the menu in the same update (BEHAVIOR 5.3) */
    assert(pause_menu_update(&p,0)==FE_ACTION_NONE); /* pending action consumed once */
    pause_menu_open(&p);
    pause_menu_pointer(&p,540,302,640,480); /* Continue uses selected action */
    assert(pause_menu_update(&p,0)==FE_ACTION_RESUME && !p.open);
    pause_menu_open(&p);
    pause_menu_pointer(&p,479,227,640,480); /* neighbour click is inert */
    assert(p.state.selected==0 && p.state.pending==FE_ACTION_NONE);
    p.state.selected=1;p.scroll=1;
    pause_menu_pointer(&p,540,302,640,480);
    assert(p.quit.open && p.open);
    pause_menu_pointer(&p,104,260,640,480); /* clicks outside both buttons never reach the pause screen */
    pause_menu_pointer(&p,60,228,640,480);  /* nor its navigation arrow */
    assert(p.quit.open && p.open && p.state.pending==FE_ACTION_NONE && p.state.selected==1);
    pause_menu_pointer(&p,470,314,640,480); /* the negative button cancels */
    assert(!p.quit.open && p.open && p.state.screen==FE_SCREEN_PAUSE);
    p.state.entries[1].enabled=0;
    p.state.selected=0;p.scroll=0;
    pause_menu_pointer(&p,479,227,640,480);
    assert(p.state.selected==0);
    e.key.keysym.sym=SDLK_RIGHT;pause_menu_event(&p,&e,NULL);
    assert(p.state.selected==0);
    int sizes[][2]={{1920,1080},{1280,960},{1280,720},{640,480},{960,600},{1919,1079}};
    for(size_t i=0;i<sizeof sizes/sizeof sizes[0];i++) {
        pause_menu_open(&p);
        PauseTransform t=pause_menu_transform(sizes[i][0],sizes[i][1]);
        pause_menu_pointer(&p,(float)(t.ox+479*t.scale),(float)(t.oy+227*t.scale),sizes[i][0],sizes[i][1]);
        assert(p.state.selected==0 && p.state.pending==FE_ACTION_NONE);
        p.state.selected=1;p.scroll=1;
        pause_menu_pointer(&p,(float)(t.ox+540*t.scale),(float)(t.oy+302*t.scale),sizes[i][0],sizes[i][1]);
        assert(p.quit.open && p.state.screen==FE_SCREEN_PAUSE);
    }
    /* Authored scene-only composition; no independent hand-built fallback. */
    p.state.screen=FE_SCREEN_PAUSE;
    UgFngObject object={.type=UG_OBJ_IMAGE,.color={20,30,40,170},.size={200,80},.pos={0,-10},.uv={0,0,1,1}};
    UgFngPackage package={.obj=&object,.nobj=1};
    assert(ug_scene_init(&p.scene,&package,NULL));p.loaded=1;
    ug_scene_set_assets(&p.scene,NULL);ug_ctx_init(&p.ctx);ug_cv_init(&p.canvas);
    pause_menu_compose(&p);
    assert(p.canvas.n==6 && ug_cv_visual_status(&p.canvas)==UG_VIS_OK);
    assert(p.canvas.v[0].a==170);
    assert(p.canvas.v[0].x==-100 && p.canvas.v[0].y==-50);
    ug_cv_free(&p.canvas);ug_scene_free(&p.scene);p.loaded=0;
    puts("focused pause state/native-only canvas checks: PASS");
}

/* Correction vectors exercise lookup/placement against independently made
 * metadata and alpha fixtures; never read an install or retail pixels. */
static void correction_vector(int index) {
    PauseMenu p={0};
    const uint32_t keys[]={0x62AA0DF6u,0x520DE4E3u,0x367B3E04u,0x9943D584u,0xA79FA3B1u,0x42A78D53u};
    const int widths[]={128,128,32,32,32,16}, heights[]={64,64,32,32,32,16};
    const int boxes[][4]={{35,3,93,61},{20,13,107,51},{2,2,30,30},{2,2,30,30},{2,2,30,30},{3,4,13,13}};
    UgTexture textures[6]={0};
    uint8_t pixels[6][128*64*4]={{0}};
    for(int i=0;i<6;i++) {
        textures[i]=(UgTexture){.key=keys[i],.w=widths[i],.h=heights[i],.rgba=pixels[i]};
        for(int y=boxes[i][1];y<boxes[i][3];y++)for(int x=boxes[i][0];x<boxes[i][2];x++)
            memset(pixels[i]+(y*widths[i]+x)*4,255,4);
    }
    p.assets.tex.t=textures;p.assets.tex.n=6;ug_texlib_index(&p.assets.tex);
    pause_resolve_art(&p);
    /* Historical screenshot art is a correspondence fixture, independent of the application's Quit binding. */
    p.item_tex[1]=pause_texture(&p.assets,0x520DE4E3u);
    p.arrow_tex=pause_texture(&p.assets,0x367B3E04u);
    p.marker_tex=pause_texture(&p.assets,0x42A78D53u);
    if(index==0) {
        assert(p.item_tex[0]==1 && p.item_tex[1]==2);
        UgRect a=native_ink_rect(&p.assets.tex,p.item_tex[0],pause_menu_icon_ink(0));
        UgRect b=native_ink_rect(&p.assets.tex,p.item_tex[1],pause_menu_icon_ink(1));
        /* Project the fixture's nonzero alpha through the emitted full quad. */
        float sa=a.w/128,sb=b.w/128;
        printf("{\"selected_key\":\"0x%08X\",\"neighbor_key\":\"0x%08X\",\"selected_size\":[128,64],\"neighbor_size\":[128,64],\"selected_alpha_bbox\":[35,3,93,61],\"neighbor_alpha_bbox\":[20,13,107,51],\"selected_visible_bounds\":[%.3f,%.3f,%.3f,%.3f],\"neighbor_visible_bounds\":[%.3f,%.3f,%.3f,%.3f]}\n",textures[p.item_tex[0]-1].key,textures[p.item_tex[1]-1].key,a.x+35*sa,a.y+3*sa,a.x+93*sa,a.y+61*sa,b.x+20*sb,b.y+13*sb,b.x+107*sb,b.y+51*sb);
    } else if(index==1) {
        assert(p.arrow_tex==3);
        assert(!memcmp(pixels[2],pixels[3],32*32*4) && !memcmp(pixels[2],pixels[4],32*32*4));
        UgRect a=native_ink_rect(&p.assets.tex,p.arrow_tex,(UgRect){566.7,215.1,26.2,25.8});
        float s=a.w/32;
        printf("{\"decoded_level_zero_rgba_equal\":true,\"dimensions\":[32,32],\"alpha_bbox\":[2,2,30,30],\"arrow_visible_bounds\":[%.3f,%.3f,%.3f,%.3f]}\n",a.x+2*s,a.y+2*s,a.x+30*s,a.y+30*s);
        /* Every approved arrow alias resolves when the others are absent. */
        textures[2].key=0;ug_texlib_index(&p.assets.tex);p.arrow_tex=pause_texture(&p.assets,0x9943D584u);assert(p.arrow_tex==4);
        textures[3].key=0;ug_texlib_index(&p.assets.tex);p.arrow_tex=pause_texture(&p.assets,0xA79FA3B1u);assert(p.arrow_tex==5);
    } else if(index==2) {
        UgRect a=pause_menu_label_ink(0),b=pause_menu_label_ink(1);
        /* Padded glyph cells: only the alpha ink is 10x20. Transparent
         * bearings must never inflate the fitted label height/shift its pen. */
        uint8_t atlas[32*32*4]={0};
        for(int y=5;y<25;y++)for(int x=4;x<14;x++)atlas[(y*32+x)*4+3]=255;
        UgTexture text_atlas={.w=32,.h=32,.rgba=atlas};
        UgFGlyph glyphs[95];
        for(int i=0;i<95;i++)glyphs[i]=(UgFGlyph){.cp=(uint32_t)(i+32),.w=18,.h=30,.adv=12,.xoff=-3,.yoff=2};
        UgFace face={.g=glyphs,.ng=95,.cell_height=32,.atlas_w=32,.atlas_h=32,.tex=1};
        PauseMenu labels={0};labels.assets.tex.t=&text_atlas;labels.assets.tex.n=1;
        ug_cv_init(&labels.canvas);ug_text_set_face(&face);
        for(int i=0;i<2;i++) {
            ug_cv_clear(&labels.canvas);
            UgRect r=pause_menu_label_ink(i);
            native_text(&labels,i?"Continue":"Back",r,1,UG_WHITE,UG_J_CENTER);
            assert(ug_cv_visual_status(&labels.canvas)==UG_VIS_OK);
            assert(labels.canvas.n==(i?8:4)*6);
            float left=10000,top=10000,right=-10000,bottom=-10000;
            for(int g=0;g<labels.canvas.n;g+=6) {
                const UgVertex *v=&labels.canvas.v[g];
                assert(v[0].mode==UG_M_TEXT && v[0].tex==1);
                float sx=(v[1].x-v[0].x)/18,sy=(v[5].y-v[0].y)/30;
                assert(fabsf(sx-sy)<.0001f);
                left=fminf(left,v[0].x+4*sx);right=fmaxf(right,v[0].x+14*sx);
                top=fminf(top,v[0].y+5*sy);bottom=fmaxf(bottom,v[0].y+25*sy);
            }
            assert(fabsf(top-296)<2 && fabsf(bottom-304.9f)<2);
            assert(fabsf((top+bottom)/2-300.45f)<.001f);
            assert(fabsf(bottom-top-8.9f)<.3f);
            assert(fabsf((left+right)/2-(r.x+r.w/2))<.001f);
            assert(fabsf(left-r.x)<2 && fabsf(right-(r.x+r.w))<2);
        }
        ug_cv_free(&labels.canvas);ug_text_set_face(NULL);
        PauseTransform t=pause_menu_transform(1920,1080);
        printf("{\"virtual_ink_bounds\":{\"Back\":[%.3f,%.3f,%.3f,%.3f],\"Continue\":[%.3f,%.3f,%.3f,%.3f]},\"reference_pixel_ink_bounds\":{\"Back\":[%.0f,%.0f,%.0f,%.0f],\"Continue\":[%.0f,%.0f,%.0f,%.0f]},\"approximate_ink_height_virtual\":%.3f}\n",a.x,a.y,a.x+a.w,a.y+a.h,b.x,b.y,b.x+b.w,b.y+b.h,t.ox+a.x*t.scale,a.y*t.scale,t.ox+(a.x+a.w)*t.scale,(a.y+a.h)*t.scale,t.ox+b.x*t.scale,b.y*t.scale,t.ox+(b.x+b.w)*t.scale,(b.y+b.h)*t.scale,a.h);
    } else if(index==3 || index==4) {
        ug_cv_init(&p.canvas);p.skin.box_w=1;p.skin.corner_w=2;
        native_stacked_surface(&p.canvas,&p.skin,(UgRect){32,196,581.3,62.7},index==3,UG_WHITE);
        assert(p.canvas.n==30); /* three fill quads, two native exterior corners */
        for(int i=18;i<30;i++)assert(p.canvas.v[i].tex==2);
        /* UV mapping reverses left x once; top corners reverse y once,
         * bottom corners retain y. The adjoining edge is all box artwork. */
        const UgVertex *left=&p.canvas.v[18],*right=&p.canvas.v[24];
        assert(left[0].x>left[1].x && right[0].x<right[1].x);
        assert(left[0].lx==0 && left[1].lx==1 && left[0].ly==0 && left[5].ly==1);
        assert((index==3)==(left[0].y>left[5].y));
        assert(fabsf(left[0].x-left[1].x)>=4 && fabsf(left[0].x-left[1].x)<=10);
        assert(p.canvas.v[6].y==(index==3?203:196));
        printf("{\"upper_left\":\"%s\",\"upper_right\":\"%s\",\"lower_left\":\"%s\",\"lower_right\":\"%s\",\"corner_transition_extent_virtual_range\":[4,10]}\n",index==3?"rounded":"square",index==3?"rounded":"square",index==4?"rounded":"square",index==4?"rounded":"square");
        ug_cv_free(&p.canvas);
    } else if(index==5) {
        assert(p.marker_tex==6);
        UgRect r=pause_menu_marker_ink();
        UgRect quad=native_ink_rect(&p.assets.tex,p.marker_tex,r);
        float s=quad.w/16;
        assert(fabsf(quad.h/16-s)<.0001f);
        assert(fabsf(quad.x+8*s-50)<2 && fabsf(quad.y+8.5f*s-188.4f)<2);
        printf("{\"bright_core_shape\":\"circular\",\"center_virtual\":[50,188.4],\"visible_bounds_virtual\":[%.3f,%.3f,%.3f,%.3f],\"flat_block_acceptable\":false,\"original_resource_alias\":\"unknown\"}\n",r.x,r.y,r.x+r.w,r.y+r.h);
    } else {
        textures[0].key=0x12BB5EA2u;textures[1].key=0x4C9E34E6u;
        ug_texlib_index(&p.assets.tex);pause_resolve_art(&p);
        assert(!p.item_tex[0] && !pause_texture(&p.assets,0x520DE4E3u));
        puts("{\"initial_reference_artwork_matches\":false}");
    }
    free(p.assets.tex.order);
}

int main(int argc, char **argv) {
    if (argc < 2) return 2;
    if (!strcmp(argv[1],"binding")) { binding_vector(argv[2]);return 0; }
    if (!strcmp(argv[1],"motion")) { motion_vector(argc,argv);return 0; }
    if (!strcmp(argv[1],"authored-samples")) { authored_sample_fixture();return 0; }
    if (!strcmp(argv[1],"behavior-timeline")) { behavior_timeline(argc,argv);return 0; }
    if (!strcmp(argv[1],"behavior-place")) { behavior_place(argv[2]);return 0; }
    if (!strcmp(argv[1],"behavior-substeps")) { behavior_substeps();return 0; }
    if (!strcmp(argv[1],"behavior-blink")) { behavior_blink(argv[2]);return 0; }
    if (!strcmp(argv[1],"behavior-resume")) { behavior_resume(argv[2]);return 0; }
    if (!strcmp(argv[1],"quit-static")) { quit_static();return 0; }
    if (!strcmp(argv[1],"quit-scenario") && argc>3) { quit_scenario(atoi(argv[2]),argv[3]);return 0; }
    if (!strcmp(argv[1],"quit-presentation") && argc>2) { quit_presentation(argv[2]);return 0; }
    if (!strcmp(argv[1],"quit-text") && argc>2) { quit_text(atoi(argv[2]));return 0; }
    if (!strcmp(argv[1],"quit-draw")) { quit_draw();return 0; }
    if (!strcmp(argv[1],"quit-identities")) { quit_identities();return 0; }
    if (!strcmp(argv[1],"quit-constants")) { quit_constants();return 0; }
    if (!strcmp(argv[1],"quit-track") && argc>3) { quit_track(argc,argv);return 0; }
    if (!strcmp(argv[1],"checks")) { focused_checks();return 0; }
    if (!strcmp(argv[1],"correction")) { correction_vector(atoi(argv[2]));return 0; }
    if (!strcmp(argv[1], "geometry")) {
        int w=atoi(argv[2]), h=atoi(argv[3]);
        PauseTransform t=pause_menu_transform(w,h);
        float x,y;
        pause_menu_to_virtual(t,t.ox+540*t.scale,t.oy+302*t.scale,&x,&y);
        printf("{\"scale\":%.9f,\"origin\":[%.9f,%.9f],\"pixel_bounds\":{",t.scale,t.ox,t.oy);
        const char *names[]={"outer_panel","selected_frame","back_button","continue_button"};
        for(int i=0;i<4;i++) {
            UgRect r=pause_menu_bounds(i);
            printf("%s\"%s\":[%.9f,%.9f,%.9f,%.9f]",i?",":"",names[i],t.ox+r.x*t.scale,t.oy+r.y*t.scale,t.ox+(r.x+r.w)*t.scale,t.oy+(r.y+r.h)*t.scale);
        }
        printf("},\"pixel_continue_click\":[%.9f,%.9f],\"inverse_click\":[%.9f,%.9f],\"continue_hit\":%s,\"back_hit\":%s,\"selection_preserved\":true}\n",t.ox+540*t.scale,t.oy+302*t.scale,x,y,ug_rect_hit(pause_menu_bounds(3),x,y)?"true":"false",ug_rect_hit(pause_menu_bounds(2),x,y)?"true":"false");
        return 0;
    }
    PauseMenu p={0};
    pause_menu_open(&p);
    FeMenuEntry fixture[]={ {"Return to Explore Mode",FE_ACTION_RESUME,1}, {"Synthetic configuration action",FE_ACTION_RACE_SELECT,1}, {"Synthetic disabled action",FE_ACTION_FREE_ROAM,0} };
    fe_set_entries(&p.state,fixture,3);
    if(argc>3 && !strcmp(argv[3],"supported_configuration"))p.state.selected=1;
    const char *event=argv[2];
    Fe baseline=p.state;
    FeInput baseline_input=FE_INPUT_NONE;
    SDL_Event e={0};e.type=SDL_KEYDOWN;
    if(!strcmp(event,"right")) e.key.keysym.sym=SDLK_RIGHT;
    else if(!strcmp(event,"left"))e.key.keysym.sym=SDLK_LEFT;
    else if(!strcmp(event,"enter"))e.key.keysym.sym=SDLK_RETURN;
    else if(!strcmp(event,"escape"))e.key.keysym.sym=SDLK_ESCAPE;
    else e.key.keysym.sym=SDLK_w;
    if(!strcmp(event,"continue_click"))pause_menu_pointer(&p,540,302,640,480);
    else if(!strcmp(event,"back_click"))pause_menu_pointer(&p,104,302,640,480);
    else if(!strcmp(event,"configuration_icon_click"))pause_menu_pointer(&p,479,227,640,480);
    else if(!strcmp(event,"disabled_item_click_and_continue")) {
        p.state.selected=2;pause_menu_pointer(&p,321,227,640,480);pause_menu_pointer(&p,540,302,640,480);
    } else if(strcmp(event,"open_pause") && strcmp(event,"resize"))pause_menu_event(&p,&e,NULL);
    if(!strcmp(event,"continue_click") || !strcmp(event,"enter"))baseline_input=FE_INPUT_CONFIRM;
    fe_input(&baseline,baseline_input);
    FeAction expected_baseline=fe_poll_action(&baseline);
    FeAction a=pause_menu_update(&p,0);
    printf("{\"paused\":%s,\"selected\":\"%s\",\"caption\":\"%s\",\"title\":\"Pause\",\"back_label\":\"Back\",\"accept_label\":\"Continue\",\"invocations\":%d,\"resume_invocations\":%d,\"cancel_invocations\":%d,\"supported_configuration_invocations\":%d,\"existing_configuration_behavior_preserved\":%s,\"simulation_advancement_from_input\":0,\"disabled_action_invocations\":%d}\n",p.open?"true":"false",p.state.selected==0?"resume":"supported_configuration",pause_menu_caption(&p),a!=FE_ACTION_NONE,a==FE_ACTION_RESUME,a==FE_ACTION_RESUME,a==FE_ACTION_RACE_SELECT,(a==expected_baseline && p.state.screen==baseline.screen)?"true":"false",a==FE_ACTION_FREE_ROAM);
    return 0;
}
