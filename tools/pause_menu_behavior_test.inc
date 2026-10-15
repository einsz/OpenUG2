/* Behaviour harness of the pause carousel through the real adapter entry points (events, update,
 * authored scene state), on the declarative binding fixture; no runtime input.
 * tools/pause_menu_behavior_check.py drives these modes from tools/pause_menu_vectors/behavior.json and
 * compares the printed JSON. Superseded cases (SPEC_GUI_ART 2.4, 3.2: exit-then-resume, Hermite scroll)
 * are skipped there and covered by the checks in this file instead. */
#define BEH_MAX_EV 48
typedef struct { int t; char name[24]; float x, y; int fields; } BehEvent;
typedef struct { float x, y, w, h, alpha; } BehItem;
/* The scene is dropped when the menu closes; samples taken afterwards report the last presented state. */
typedef struct { PauseMenu *p; int n; int left, right; BehItem item[PE_MAX]; unsigned long long sig; } BehFixture;

static int beh_parse_events(const char *spec, BehEvent *ev) {
    int n = 0;
    if (!strcmp(spec, "-")) return 0;                    /* no events */
    char *copy = strdup(spec), *save = NULL;
    for (char *it = strtok_r(copy, ",", &save); it && n < BEH_MAX_EV; it = strtok_r(NULL, ",", &save)) {
        BehEvent *e = &ev[n];
        memset(e, 0, sizeof *e);
        e->fields = sscanf(it, "%d:%23[^:]:%f:%f", &e->t, e->name, &e->x, &e->y);
        assert(e->fields >= 2);
        n++;
    }
    free(copy);
    return n;
}
static void beh_snap(BehFixture *f);
/* catalogue: a count of synthetic entries or comma-separated names (resume/quit/other). */
static void beh_fixture(BehFixture *f, const char *catalogue, int start) {
    f->p = (PauseMenu *)calloc(1, sizeof *f->p);
    binding_fixture_init(f->p);
    f->n = fx_catalogue(f->p, catalogue, start);
    pause_menu_update(f->p, 0);
    beh_snap(f);
}
static void beh_free(BehFixture *f) { fx_remove(f->p); free(f->p); f->p = NULL; }

static unsigned long long beh_signature(const PauseMenu *p) {
    unsigned long long h = 1469598103934665603ull;
    #define MIX(v) do { float f_ = (float)(v); const unsigned char *b_ = (const unsigned char *)&f_; for (size_t i_ = 0; i_ < sizeof f_; i_++) { h ^= b_[i_]; h *= 1099511628211ull; } } while (0)
    for (int i = 0; p->scr.loaded && i < p->scr.scene.n; i++) {
        const UgNode *n = &p->scr.scene.nodes[i];
        MIX(n->we); MIX(n->wf); MIX(n->props.w[UG_P_SIZE]); MIX(n->props.w[UG_P_SIZE + 1]);
        MIX(n->wcolor.a); MIX(n->visible); MIX(n->app_hidden); MIX(n->app_opacity); MIX(n->tex_override);
    }
    MIX(p->ps.car.scroll); MIX(p->ps.car.cur); MIX(p->open);
    #undef MIX
    return h;
}
static void beh_snap(BehFixture *f) {
    const PauseMenu *p = f->p;
    f->sig = beh_signature(p);
    if (!p->scr.loaded) return;                          /* closed: keep the last presented state */
    const UgScene *sc = &p->scr.scene;
    f->left = !sc->nodes[p->car_v.arrow_l].app_hidden;
    f->right = !sc->nodes[p->car_v.arrow_r].app_hidden;
    for (int k = 0; k < f->n; k++) {
        const UgNode *n = &sc->nodes[p->car_v.slot[k]];
        f->item[k] = (BehItem){ n->we + 320, n->wf + 240, n->props.w[UG_P_SIZE], n->props.w[UG_P_SIZE + 1], n->app_hidden ? 0.0f : 1.0f };
    }
}
static void beh_sample(const BehFixture *f) {
    const PauseMenu *p = f->p;
    printf("{\"i\":%d,\"s\":%.6f,\"marker\":[%.4f,188.4],\"left\":%s,\"right\":%s,\"caption\":\"%s\",\"paused\":%s,\"screen\":3,\"items\":[",
           p->ps.car.cur, -gc_scroll_value(&p->ps.car) / GC_PITCH, fx_marker_x(p),
           f->left ? "true" : "false", f->right ? "true" : "false", pause_menu_caption(p), p->open ? "true" : "false");
    for (int k = 0; k < f->n; k++)
        printf("%s{\"x\":%.4f,\"y\":%.4f,\"w\":%.4f,\"h\":%.4f,\"alpha\":%.4f}", k ? "," : "", f->item[k].x, f->item[k].y, f->item[k].w, f->item[k].h, f->item[k].alpha);
    printf("]}");
}
static int beh_back_type(const char *name) {
    return !strcmp(name, "Escape") || !strcmp(name, "Back") || !strcmp(name, "right_click") ||
           !strcmp(name, "controller_Back") || !strcmp(name, "controller_Start") || !strcmp(name, "back_pill");
}
/* Deliver one input through the adapter's public/real handlers. */
static void beh_apply(PauseMenu *p, const BehEvent *e) {
    SDL_Event k = {0};
    if (!strcmp(e->name, "Right") || !strcmp(e->name, "Left") || !strcmp(e->name, "Enter") || !strcmp(e->name, "Escape") || !strcmp(e->name, "Back")) {
        k.type = SDL_KEYDOWN;
        k.key.keysym.sym = !strcmp(e->name, "Right") ? SDLK_RIGHT : !strcmp(e->name, "Left") ? SDLK_LEFT : !strcmp(e->name, "Enter") ? SDLK_RETURN : SDLK_ESCAPE;
        pause_menu_event(p, &k, NULL);
    } else if (!strncmp(e->name, "controller_", 11)) {
        k.type = SDL_CONTROLLERBUTTONDOWN;
        k.cbutton.button = !strcmp(e->name, "controller_Back") ? SDL_CONTROLLER_BUTTON_B : !strcmp(e->name, "controller_Start") ? SDL_CONTROLLER_BUTTON_START : SDL_CONTROLLER_BUTTON_A;
        pause_menu_event(p, &k, NULL);
    } else if (!strcmp(e->name, "right_click")) {
        k.type = SDL_MOUSEBUTTONDOWN; k.button.button = SDL_BUTTON_RIGHT; pause_menu_event(p, &k, NULL);
    } else if (!strcmp(e->name, "Continue")) pause_menu_pointer(p, 540, 302, 640, 480);
    else if (!strcmp(e->name, "back_pill")) pause_menu_pointer(p, 104, 302, 640, 480);
    else if (!strcmp(e->name, "press")) pause_pointer_press(p, e->x, e->y);
    else if (!strcmp(e->name, "release")) pause_pointer_release(p, e->x, e->y);
    else if (!strcmp(e->name, "click")) { pause_pointer_press(p, e->x, e->y); pause_pointer_release(p, e->x, e->y); }
    else if (!strcmp(e->name, "hover")) pause_pill_hover(p, e->x, e->y);
    else assert(!"unknown behaviour event");
}
static const char *beh_action_name(FeAction a, int back) {
    return a == FE_ACTION_RESUME ? (back ? "back/resume" : "resume") : a == FE_ACTION_QUIT ? "quit" : a == FE_ACTION_REPLAY ? "options" : "other";
}
typedef struct { char invoked[512], by_event[1024]; int first; } BehLog;
static void beh_log(BehLog *l, FeAction a, int event, const BehEvent *ev) {
    char piece[64];
    if (a == FE_ACTION_NONE) return;
    snprintf(piece, sizeof piece, "%s\"%s\"", l->first ? "" : ",", beh_action_name(a, event >= 0 && beh_back_type(ev[event].name)));
    strcat(l->invoked, piece);
    l->first = 0;
    snprintf(piece, sizeof piece, "%s%d", *l->by_event ? "," : "", event);
    strcat(l->by_event, piece);
}
/* behavior timeline <catalogue> <start> <events> <sample times...>
 * Samples are the state after processing every event with t <= sample (the first presented frame
 * after an input); -1 is the state before anything. Time advances in 1 ms UI steps. After the last
 * sample the run continues for 450 ms so that actions that wait for the 320 ms exit are collected;
 * each is attributed to the most recent input event. */
static void behavior_timeline(int argc, char **argv) {
    BehFixture f;
    BehEvent ev[BEH_MAX_EV];
    BehLog log = { "", "", 1 };
    beh_fixture(&f, argv[2], atoi(argv[3]));
    int nev = beh_parse_events(argv[4], ev), nsamples = argc - 5, end = 0;
    int samples[64];
    assert(nsamples < 64);
    for (int i = 0; i < nsamples; i++) { samples[i] = atoi(argv[5 + i]); if (samples[i] > end) end = samples[i]; }
    for (int i = 0; i < nev; i++) if (ev[i].t > end) end = ev[i].t;
    int last_event = -1, blinks = 0, last_blink = 0, printed = 0;
    unsigned long long sig0 = beh_signature(f.p);
    printf("{\"samples\":{");
    for (int i = 0; i < nsamples; i++) if (samples[i] < 0) { printf("%s\"%d\":", printed++ ? "," : "", samples[i]); beh_sample(&f); }
    for (int ms = 0; ms <= end + 450; ms++) {
        if (ms > 0) { beh_log(&log, pause_menu_update(f.p, 0.001f), last_event, ev); beh_snap(&f); }   /* an action that waited for its exit */
        for (int i = 0; i < nev; i++) if (ev[i].t == ms) {
            unsigned before = f.p->ps.move_serial;
            beh_apply(f.p, &ev[i]);
            last_event = i;
            if (f.p->ps.move_serial != before && ((f.p->ps.move_dir > 0 && gc_right_arrow_visible(&f.p->ps.car)) ||
                                                  (f.p->ps.move_dir < 0 && gc_left_arrow_visible(&f.p->ps.car)))) {
                blinks++;
                last_blink = f.p->ps.move_dir;
            }
            beh_log(&log, pause_menu_update(f.p, 0), i, ev);
            beh_snap(&f);
        }
        for (int i = 0; i < nsamples; i++) if (samples[i] == ms) { printf("%s\"%d\":", printed++ ? "," : "", ms); beh_sample(&f); }
    }
    printf("},\"invoked\":[%s],\"invoked_event_indexes\":[%s],\"blinks\":%d,\"last_blink\":\"%s\",\"signature_changed\":%s,\"open\":%s,\"screen\":3,\"end_i\":%d}\n",
           log.invoked, log.by_event, blinks, last_blink < 0 ? "left" : last_blink > 0 ? "right" : "none", sig0 != beh_signature(f.p) ? "true" : "false",
           f.p->open ? "true" : "false", f.p->ps.car.cur);
    beh_free(&f);
}

/* Arrow blink data from the package (BEHAVIOR 3.5) installed on the arrow objects of the binding
 * fixture; the adapter must start Blink on the arrow of the move direction and never on a hidden one. */
static void behavior_blink(const char *side) {
    int right = !strcmp(side, "right");
    UgKey blink_color[3] = {{.t = 0, .v = {202, 244, 138, 150}}, {.t = 200, .v = {202, 244, 138, 255}}, {.t = 400, .v = {202, 244, 138, 150}}};
    UgKey blink_size[3] = {{.t = 0, .v = {26, 26, 0}}, {.t = 200, .v = {32, 32, 0}}, {.t = 400, .v = {26, 26, 0}}};
    UgKey idle_color[1] = {{.t = 0, .v = {255, 255, 255, 150}}}, idle_size[1] = {{.t = 0, .v = {26, 26, 0}}};
    UgTrack blink_tracks[2] = {
        {.type = UG_VT_COLOR, .vsize = 4, .interp = UG_INTERP_LINEAR, .length = 400, .prop = UG_P_COLOR, .keys = blink_color, .nkeys = 3},
        {.type = UG_VT_VEC3, .vsize = 3, .interp = UG_INTERP_LINEAR, .length = 400, .prop = UG_P_SIZE, .keys = blink_size, .nkeys = 3}};
    UgTrack idle_tracks[2] = {
        {.type = UG_VT_COLOR, .vsize = 4, .interp = UG_INTERP_LINEAR, .length = 0, .prop = UG_P_COLOR, .keys = idle_color, .nkeys = 1},
        {.type = UG_VT_VEC3, .vsize = 3, .interp = UG_INTERP_LINEAR, .length = 0, .prop = UG_P_SIZE, .keys = idle_size, .nkeys = 1}};
    UgScript scripts[2] = {
        {.id = 0x0280164Fu, .length = 400, .tracks = blink_tracks, .ntracks = 2, .has_chain = 1, .chain = 0x7AB70D67u},
        {.id = 0x7AB70D67u, .length = 0, .tracks = idle_tracks, .ntracks = 2}};
    /* binding_objects[0] is the right arrow, [1] the left arrow. */
    UgFngObject saved[2] = {binding_objects[0], binding_objects[1]};
    for (int i = 0; i < 2; i++) { binding_objects[i].scripts = scripts; binding_objects[i].nscripts = 2; }
    BehFixture f;
    beh_fixture(&f, "3", right ? 0 : 2);
    int arrow = right ? f.p->car_v.arrow_r : f.p->car_v.arrow_l;
    pause_key(f.p, right ? PMK_RIGHT : PMK_LEFT);
    pause_menu_update(f.p, 0);
    assert(ug_scene_script_id(&f.p->scr.scene, arrow) == 0x0280164Fu);
    printf("{\"samples\":[");
    const int times[] = {0, 100, 200, 300, 400};
    int now = 0;
    for (int i = 0; i < 5; i++) {
        while (now < times[i]) { pause_menu_update(f.p, 0.001f); now++; }
        const UgProps *pr = &f.p->scr.scene.nodes[arrow].props;
        printf("%s{\"t_ms\":%d,\"rgba\":[%.2f,%.2f,%.2f,%.2f],\"size\":[%.3f,%.3f]}", i ? "," : "", times[i],
               pr->w[0], pr->w[1], pr->w[2], pr->w[3], pr->w[UG_P_SIZE], pr->w[UG_P_SIZE + 1]);
    }
    while (now < 450) { pause_menu_update(f.p, 0.001f); now++; }
    const UgProps *pr = &f.p->scr.scene.nodes[arrow].props;
    printf("],\"after_blink_static\":{\"rgba\":[%.2f,%.2f,%.2f,%.2f],\"size\":[%.3f,%.3f]},\"hidden_arrow_blinks\":",
           pr->w[0], pr->w[1], pr->w[2], pr->w[3], pr->w[UG_P_SIZE], pr->w[UG_P_SIZE + 1]);
    beh_free(&f);
    /* Hidden wins (3.6): stepping onto the end item must not start a blink for that direction. */
    BehFixture g;
    beh_fixture(&g, "2", right ? 0 : 1);
    int arrow2 = right ? g.p->car_v.arrow_r : g.p->car_v.arrow_l;
    pause_key(g.p, right ? PMK_RIGHT : PMK_LEFT);
    pause_menu_update(g.p, 0);
    printf("%d}\n", ug_scene_script_id(&g.p->scr.scene, arrow2) == 0x0280164Fu);
    beh_free(&g);
    binding_objects[0] = saved[0];
    binding_objects[1] = saved[1];
}

/* behavior resume <trigger>: the host frame loop of src/main.c (events first, then pause_menu_update,
 * simulation gated by the frame-top pause flag) around the real adapter. SPEC_GUI_ART 2.4: the screen
 * runs its 320 ms exit script and resumes at exit-complete; the triggering input is never forwarded. */
static void behavior_resume(const char *trigger) {
    BehFixture f;
    beh_fixture(&f, "resume,options,quit", 0);
    PauseMenu *p = f.p;
    const double frame_ms = 16.667;
    const int input_frame = 100, frames = 140;
    int paused_after[140], sim_advanced[140], gameplay_input[140], action_frame = -1, trigger_to_gameplay = 0;
    for (int frame = 0; frame < frames; frame++) {
        int pause_frame = pause_menu_is_open(p);
        gameplay_input[frame] = 0;
        BehEvent events[3];
        int nevents = 0;
        memset(events, 0, sizeof events);
        if (frame == input_frame || (frame == input_frame - 1 && !strcmp(trigger, "Resume_item_click"))) {
            const char *name = NULL;
            if (!strcmp(trigger, "Escape")) name = "Escape";
            else if (!strcmp(trigger, "controller_Back")) name = "controller_Back";
            else if (!strcmp(trigger, "controller_Start")) name = "controller_Start";
            else if (!strcmp(trigger, "right_mouse_button")) name = "right_click";
            else if (!strcmp(trigger, "Back_pill_click")) name = "back_pill";
            else if (!strcmp(trigger, "Resume_item_Enter")) name = "Enter";
            else if (!strcmp(trigger, "Continue_pill_with_Resume_selected")) name = "Continue";
            else if (!strcmp(trigger, "Resume_item_click")) name = frame == input_frame - 1 ? "press" : "release";
            assert(name);
            snprintf(events[0].name, sizeof events[0].name, "%s", name);
            events[0].x = 320; events[0].y = 226;
            nevents = 1;
        }
        /* a gameplay probe key every frame: accepted by gameplay only while the menu is closed */
        for (int i = 0; i < nevents + 1; i++) {
            int probe = i == nevents;
            if (pause_menu_is_open(p)) {
                if (!probe) beh_apply(p, &events[i]);
            } else if (probe) gameplay_input[frame] = 1;
            else trigger_to_gameplay = 1;
        }
        FeAction a = pause_menu_update(p, (float)(frame_ms / 1000));
        if (a == FE_ACTION_RESUME && action_frame < 0) action_frame = frame;
        paused_after[frame] = pause_menu_is_open(p);
        sim_advanced[frame] = !pause_frame;
    }
    int sim_from = -1, input_from = -1;
    for (int frame = input_frame; frame < frames; frame++) {
        if (sim_from < 0 && sim_advanced[frame]) sim_from = frame;
        if (input_from < 0 && gameplay_input[frame]) input_from = frame;
    }
    /* the closed menu accepts nothing */
    int sel = p->ps.car.cur;
    SDL_Event k = {0};
    k.type = SDL_KEYDOWN; k.key.keysym.sym = SDLK_RIGHT;
    pause_menu_event(p, &k, NULL);
    k.key.keysym.sym = SDLK_RETURN;
    pause_menu_event(p, &k, NULL);
    pause_menu_pointer(p, 320, 226, 640, 480);
    int accepts = p->ps.car.cur != sel || p->ps.exiting || ps_dialog_open(&p->ps);
    printf("{\"paused_after_processing_frame\":{\"99\":%s,\"100\":%s,\"%d\":%s},\"simulation_advances_from_frame\":%d,"
           "\"gameplay_clock_advances_from_frame\":%d,\"gameplay_input_accepted_from_frame\":%d,\"audio_resumed_from_frame\":%d,"
           "\"triggering_input_forwarded_to_gameplay\":%s,\"menu_accepts_input_after_resume\":%s,\"exit_frames\":%d}\n",
           paused_after[99] ? "true" : "false", paused_after[100] ? "true" : "false", input_frame + 20, paused_after[input_frame + 20] ? "true" : "false",
           sim_from, sim_from, input_from, action_frame < 0 ? -1 : action_frame + 1,
           trigger_to_gameplay ? "true" : "false", accepts ? "true" : "false", sim_from - input_frame);
    beh_free(&f);
}
