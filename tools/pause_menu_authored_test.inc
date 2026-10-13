/* Independent fixtures for AUTHORED 3 and MOTION 3/4; no runtime inputs. */
static void authored_checks(void) {
    binding_checks();binding_track_checks();binding_reference_checks();
    PauseMenu p={0}; pause_menu_open(&p);
    FeMenuEntry entries[]={{"A",FE_ACTION_RESUME,1},{"B",FE_ACTION_RACE_SELECT,1},{"C",FE_ACTION_FREE_ROAM,1}};
    fe_set_entries(&p.state,entries,3);
    assert(fabsf(pause_marker_target(&p)-50)<.001f);
    assert(pause_can_step(&p,-1) && pause_can_step(&p,1));
    pause_dispatch(&p,FE_INPUT_DOWN);
    assert(p.state.selected==1 && p.marker_x==50);
    pause_menu_update(&p,.08f);
    assert(fabsf(p.marker_x-186.5f)<.001f);
    pause_dispatch(&p,FE_INPUT_UP);
    pause_menu_update(&p,.08f);
    assert(fabsf(p.marker_x-118.25f)<.001f);
    pause_menu_update(&p,.08f);
    assert(fabsf(p.marker_x-50)<.001f);
    pause_menu_pointer(&p,323,188.4f,640,480);
    assert(p.state.selected==1 && p.state.pending==FE_ACTION_NONE);
    pause_menu_pointer(&p,60,228,640,480);
    assert(p.state.selected==0);
    pause_menu_pointer(&p,580,228,640,480);
    assert(p.state.selected==1);
    pause_menu_pointer(&p,540,302,640,480);
    assert(pause_menu_update(&p,0)==FE_ACTION_RACE_SELECT);
    assert(pause_menu_update(&p,0)==FE_ACTION_NONE);
    pause_menu_open(&p);
    assert(p.ui_ms==0 && p.marker_x==50 && !p.rail_drag);
    pause_menu_update(&p,.1f);
    pause_dispatch(&p,FE_INPUT_BACK);
    assert(pause_menu_update(&p,0)==FE_ACTION_RESUME);
    pause_menu_open(&p);
    assert(p.ui_ms==0 && p.state.pending==FE_ACTION_NONE);
    puts("authored integration/moving marker/arrow/rail synthetic checks: PASS");
}

static void authored_sample_fixture(void) {
    /* Values are approved vector inputs to independent synthetic tracks, never production constants. */
    const int times[]={0,100,200,300,600,700,800};
    const int alpha[4][7]={{0,78,209,235,235,235,235},{0,33,89,100,100,100,100},{0,22,58,65,65,65,65},{0,58,108,90,117,162,207}};
    UgFngObject objects[4]={0};UgScript scripts[4]={0};UgTrack tracks[4]={0};UgKey keys[4][7]={0};
    for(int j=0;j<4;j++) {
        for(int k=0;k<7;k++)keys[j][k]=(UgKey){.t=times[k],.v={20,30,40,alpha[j][k]}};
        tracks[j]=(UgTrack){.type=UG_VT_COLOR,.vsize=4,.length=800,.prop=UG_P_COLOR,.keys=keys[j],.nkeys=7};
        scripts[j]=(UgScript){.id=UG_SCRIPT_INITIAL,.length=800,.tracks=&tracks[j],.ntracks=1};
        objects[j]=(UgFngObject){.type=UG_OBJ_IMAGE,.guid=(uint32_t)(j+1),.color={20,30,40,255},.size={100,20},.pos={0,j*25},.scripts=&scripts[j],.nscripts=1};
    }
    UgFngPackage package={.obj=objects,.nobj=4};UgScene scene;
    assert(ug_scene_init(&scene,&package,NULL));ug_scene_set_assets(&scene,NULL);
    printf("{\"effective_object_alpha_before_texture\":{");
    const char *names[]={"outer","selector","caption","highlight"};
    int values[4][7];float previous=0;
    for(int k=0;k<7;k++) {
        ug_scene_update(&scene,times[k]-previous);previous=times[k];
        for(int j=0;j<4;j++) {
            values[j][k]=scene.nodes[j].wcolor.a;
            assert(abs(values[j][k]-alpha[j][k])<=1);
        }
    }
    for(int j=0;j<4;j++) {
        printf("%s\"%s\":[",j?",":"",names[j]);
        for(int k=0;k<7;k++)printf("%s%d",k?",":"",values[j][k]);
        printf("]");
    }
    printf("},\"byte_tolerance\":1}\n");ug_scene_free(&scene);
}

static void motion_vector(int argc,char **argv) {
    const char *id=argv[2];
    int n=argc>3?atoi(argv[3]):3,initial=argc>4?atoi(argv[4]):0;
    PauseMenu p={0};pause_menu_open(&p);
    FeMenuEntry entries[FE_MENU_MAX];
    assert(n>0 && n<=FE_MENU_MAX);
    for(int i=0;i<n;i++)entries[i]=(FeMenuEntry){"Synthetic action",FE_ACTION_RACE_SELECT,1};
    fe_set_entries(&p.state,entries,n);p.state.selected=initial;
    p.marker_x=p.marker_from=p.marker_to=pause_marker_target(&p);
    if(!strncmp(id,"clamped_",8) || !strcmp(id,"wrapping_first") || !strcmp(id,"singleton")) {
        /* Boundary fixture models both spec policies; host production keeps Fe's wrap. */
        int wrap=!strcmp(id,"wrapping_first");
        int left=wrap?pause_can_step(&p,-1):initial>0;
        int right=wrap?pause_can_step(&p,1):initial<n-1;
        printf("{\"marker_center\":[%.6f,188.4],\"left_arrow\":%s,\"right_arrow\":%s,\"Left_target_i\":%d}\n",p.marker_x,left?"true":"false",right?"true":"false",n-1);
    } else if(!strcmp(id,"right_keyboard_motion") || !strcmp(id,"left_arrow_motion")) {
        if(id[0]=='r')pause_dispatch(&p,FE_INPUT_DOWN);
        else pause_menu_pointer(&p,60,228,640,480);
        float x0=p.marker_x;pause_menu_update(&p,.08f);float x1=p.marker_x;pause_menu_update(&p,.08f);
        printf("{\"logical_i\":%d,\"glyph_and_caption_entry\":%d,\"marker_x\":[%.6f,%.6f,%.6f],\"simulation_paused\":%s,\"action_invocations\":0}\n",p.state.selected,p.state.selected,x0,x1,p.marker_x,p.open?"true":"false");
    } else if(!strcmp(id,"rail_click_middle") || !strcmp(id,"rail_drag_last")) {
        pause_menu_pointer(&p,id[5]=='c'?323:600,188.4f,640,480);
        printf("{\"logical_i\":%d,\"target_marker_center\":[%.6f,188.4],\"action_invocations\":%d}\n",p.state.selected,p.marker_to,p.state.pending!=FE_ACTION_NONE);
    } else if(!strcmp(id,"rapid_reversal")) {
        pause_dispatch(&p,FE_INPUT_DOWN);pause_menu_update(&p,.08f);float x0=p.marker_x;
        pause_dispatch(&p,FE_INPUT_UP);pause_menu_update(&p,.08f);float x1=p.marker_x;pause_menu_update(&p,.08f);
        printf("{\"logical_i_after_second_event\":%d,\"marker_x\":[%.6f,%.6f,%.6f],\"stale_action_invocations\":%d}\n",p.state.selected,x0,x1,p.marker_x,p.state.pending!=FE_ACTION_NONE);
    } else if(!strcmp(id,"continue_during_motion")) {
        pause_dispatch(&p,FE_INPUT_DOWN);pause_menu_update(&p,.08f);int selected=p.state.selected;
        pause_menu_pointer(&p,540,302,640,480);FeAction a=pause_menu_update(&p,0),b=pause_menu_update(&p,0);
        printf("{\"invoked_entry\":%d,\"action_invocations\":%d}\n",selected,(a!=FE_ACTION_NONE)+(b!=FE_ACTION_NONE));
    } else if(!strcmp(id,"resize_during_motion")) {
        pause_dispatch(&p,FE_INPUT_DOWN);pause_menu_update(&p,.08f);PauseTransform t=pause_menu_transform(1280,960);
        printf("{\"logical_i\":%d,\"marker_virtual_at_resize\":[%.6f,188.4],\"marker_pixel_at_resize\":[%.6f,376.8],\"target_marker_pixel\":[%.6f,376.8]}\n",p.state.selected,p.marker_x,t.ox+p.marker_x*t.scale,t.ox+p.marker_to*t.scale);
    } else if(!strcmp(id,"glow_cycle")) {
        UgAnim animation;ug_anim_init(&animation,UG_WHITE,0,0);ug_anim_play(&animation,UG_ANIM_BREATHE,0);
        float times[]={0,480,800,1200},values[4],last=0;
        for(int i=0;i<4;i++){ug_anim_update(&animation,times[i]-last);last=times[i];values[i]=ug_anim_alpha01(&animation);}
        printf("{\"normalized_glow_alpha\":[%.6f,%.6f,%.6f,%.6f]}\n",values[0],values[1],values[2],values[3]);ug_anim_free(&animation);
    } else return;
}
