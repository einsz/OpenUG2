/* Pause menu <-> graphics options screens (SPEC_GUI): the Options entry, routing of input while the
 * screens are open, Back/Start leaving, and the apply/re-init hook. Synthetic; no runtime resources. */
static SDL_Event options_key(int sym) {
    SDL_Event e;memset(&e,0,sizeof e);e.type=SDL_KEYDOWN;e.key.keysym.sym=sym;return e;
}
static SDL_Event options_pad(int button) {
    SDL_Event e;memset(&e,0,sizeof e);e.type=SDL_CONTROLLERBUTTONDOWN;e.cbutton.button=button;return e;
}
static void options_checks(void) {
    /* without pause_menu_set_options the row is the original two entries */
    PauseMenu plain={0};
    pause_menu_open(&plain);
    assert(plain.state.entry_count==2 && plain.state.entries[1].action==FE_ACTION_QUIT);
    assert(!pause_menu_options_open(&plain));

    StGraphics gfx;StGfxCaps caps;StRegStore store;
    st_gfx_defaults(&gfx);st_gfx_caps_full(&caps,5);assert(st_reg_open_memory(&store)==ST_OK);
    gfx.v[ST_GFX_RESOLUTION]=2;
    PauseMenu p={0};
    p.item_tex[0]=11;p.item_tex[1]=22;
    pause_menu_set_options(&p,&gfx,&caps,&store);
    pause_menu_open(&p);
    assert(p.state.entry_count==3 && p.state.entries[1].action==FE_ACTION_OPTIONS && p.state.entries[2].action==FE_ACTION_QUIT);
    assert(!strcmp(pause_menu_caption(&p),"Resume") || !strcmp(pause_menu_caption(&p),"Return to Explore Mode"));
    assert(pause_entry_art(&p,0)==11 && pause_entry_art(&p,1)==0 && pause_entry_art(&p,2)==22);
    SDL_Event e=options_key(SDLK_RIGHT);
    pause_menu_event(&p,&e,NULL);
    assert(p.state.selected==1 && p.marker_x==323 && !strcmp(pause_menu_caption(&p),"Options"));
    e=options_key(SDLK_RETURN);pause_menu_event(&p,&e,NULL);
    assert(p.state.pending==FE_ACTION_OPTIONS && !pause_menu_options_open(&p));
    assert(pause_menu_update(&p,0)==FE_ACTION_NONE);          /* the action stays inside the pause menu */
    assert(pause_menu_options_open(&p) && p.open && om_screen(pause_menu_options(&p))==OM_SCREEN_DISPLAY);
    assert(pause_menu_options(&p)->cfg.ctx==OM_CTX_INGAME);

    /* input goes to the options screens: Right changes the resolution, nothing reaches the pause row */
    OptionsMenu *om=pause_menu_options(&p);
    e=options_key(SDLK_RIGHT);pause_menu_event(&p,&e,NULL);
    assert(gfx.v[ST_GFX_RESOLUTION]==3 && p.state.selected==1 && om_reinit_pending(om));
    assert(om_take_reinit_request(om)==1 && om_take_reinit_request(om)==0);   /* hook: consumed once */
    e=options_key(SDLK_DOWN);pause_menu_event(&p,&e,NULL);
    assert(om_focus(om)==1);
    pause_menu_pointer(&p,320+246.2f,240+om_row_center_y(0),640,480);        /* complete click on the right arrow */
    assert(gfx.v[ST_GFX_RESOLUTION]==3);                                       /* row 1 is not focused: inert */

    /* Escape leaves Display (writing the values), back to the pause row on the Options entry */
    e=options_key(SDLK_ESCAPE);pause_menu_event(&p,&e,NULL);
    assert(pause_menu_update(&p,0)==FE_ACTION_NONE && pause_menu_options_open(&p));   /* exit animation still running */
    assert(pause_menu_update(&p,1.0f)==FE_ACTION_NONE);
    assert(!pause_menu_options_open(&p) && p.open && p.state.selected==1);
    { uint32_t v=0;assert(st_reg_get_dword(&store,"g_RacingResolution",&v) && v==3); }
    assert(om_apply_pending(om));                                               /* flags survive closing until consumed */

    /* Customize and Back to Display */
    e=options_key(SDLK_RETURN);pause_menu_event(&p,&e,NULL);
    assert(pause_menu_update(&p,0)==FE_ACTION_NONE && pause_menu_options_open(&p));
    e=options_key(SDLK_DOWN);pause_menu_event(&p,&e,NULL);pause_menu_event(&p,&e,NULL);
    e=options_key(SDLK_RETURN);pause_menu_event(&p,&e,NULL);
    pause_menu_update(&p,1.0f);
    assert(om_screen(om)==OM_SCREEN_CUSTOMIZE);
    e=options_pad(SDL_CONTROLLER_BUTTON_B);pause_menu_event(&p,&e,NULL);
    pause_menu_update(&p,1.0f);
    assert(om_screen(om)==OM_SCREEN_DISPLAY);

    /* controller Start resumes the game at once and writes nothing */
    { uint32_t v;assert(st_reg_get_dword(&store,"g_VSyncOn",&v)); }
    gfx.v[ST_GFX_VSYNC]=1;
    int writes=om->registry_writes;
    e=options_pad(SDL_CONTROLLER_BUTTON_START);pause_menu_event(&p,&e,NULL);
    assert(pause_menu_update(&p,0)==FE_ACTION_RESUME && !p.open && !pause_menu_options_open(&p));
    assert(om->registry_writes==writes);
    { uint32_t v=9;assert(st_reg_get_dword(&store,"g_VSyncOn",&v) && v==0); }  /* old value, not the Start-time one */

    /* re-opening the pause menu drops stale options state; Quit is now entry 2 and opens its dialog */
    pause_menu_open(&p);
    assert(!pause_menu_options_open(&p) && p.state.entry_count==3);
    e=options_key(SDLK_RIGHT);pause_menu_event(&p,&e,NULL);pause_menu_event(&p,&e,NULL);
    e=options_key(SDLK_RETURN);pause_menu_event(&p,&e,NULL);
    assert(p.state.selected==2 && p.quit.open);
    st_reg_close(&store);
    puts("pause options checks: PASS");
}
