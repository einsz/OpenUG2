/* Adapter checks on the declarative fixture (SPEC_GUI_ART 3 carousel behaviour at the adapter level),
 * the authored evaluation samples and the motion vectors. All synthetic. */

/* Replaces the catalogue with a synthetic one: a count of ordinary entries, or comma-separated names
 * (resume, quit, anything else = an ordinary entry). Rebinds the pause screen. Returns the count. */
static char fx_names[PE_MAX][24];
static int fx_catalogue(PauseMenu *p, const char *spec, int start) {
    PauseEntry e[PE_MAX];
    int n = 0;
    memset(e, 0, sizeof e);
    if (spec[0] >= '0' && spec[0] <= '9') {
        n = atoi(spec);
        assert(n >= 0 && n <= PE_MAX);
        for (int i = 0; i < n; i++) { snprintf(fx_names[i], sizeof fx_names[i], "item%d", i); e[i].label = fx_names[i]; e[i].action = PE_ACT_OPEN_REPLAY; }
    } else {
        char *copy = strdup(spec), *save = NULL;
        for (char *it = strtok_r(copy, ",", &save); it && n < PE_MAX; it = strtok_r(NULL, ",", &save), n++) {
            snprintf(fx_names[n], sizeof fx_names[n], "%s", it);
            e[n].label = fx_names[n];
            e[n].action = !strcmp(it, "resume") ? PE_ACT_RESUME_EXPLORE : !strcmp(it, "quit") ? PE_ACT_CONFIRM_QUIT : PE_ACT_OPEN_REPLAY;
            e[n].immediate = e[n].action == PE_ACT_CONFIRM_QUIT;
            if (!strcmp(it, "resume")) e[n].label = "Return to Explore Mode";
        }
        free(copy);
    }
    for (int i = 0; i < n; i++) { e[i].icon_key = FX_ICON_BASE + (uint32_t)i; e[i].label_key = 0x7000u + (uint32_t)i; }
    PauseContext c;
    pe_context_default(&c);
    ps_open_entries(&p->ps, &c, e, n);
    gc_snap_to(&p->ps.car, start);
    p->ps.fresh = 0;
    pause_drop_view(p);
    pause_views_update(p, 0);
    return n;
}
static float fx_marker_x(const PauseMenu *p) { return gc_tab_x(&p->ps.car) + 320; }

static void authored_checks(void) {
    binding_checks();
    PauseMenu *p = (PauseMenu *)calloc(1, sizeof *p);
    binding_fixture_init(p);
    fx_catalogue(p, "resume,options,quit", 0);
    assert(fabsf(fx_marker_x(p) - 50) < .001f);
    assert(!gc_left_arrow_visible(&p->ps.car) && gc_right_arrow_visible(&p->ps.car));   /* no wrap */
    /* the marker is never interpolated: it is at the logical target at the input frame */
    pause_key(p, PMK_RIGHT);
    assert(p->ps.car.cur == 1 && fabsf(fx_marker_x(p) - 323.0f) < .001f);
    pause_menu_update(p, .08f);
    assert(fabsf(fx_marker_x(p) - 323.0f) < .001f);
    assert(fabsf(p->scr.scene.nodes[p->car_v.tab].we + 320 - 323.0f) < .01f);
    pause_key(p, PMK_LEFT);
    assert(p->ps.car.cur == 0 && fx_marker_x(p) == 50);
    pause_key(p, PMK_UP); pause_key(p, PMK_DOWN);
    assert(p->ps.car.cur == 0);                                   /* Up/Down are ignored on the carousel */
    pause_key(p, PMK_RIGHT);
    /* rail and marker clicks are inert; the slot needs press and release */
    pause_menu_pointer(p, 323, 188.4f, 640, 480);
    assert(p->ps.car.cur == 1 && !p->ps.exiting && fx_marker_x(p) == 323.0f);
    /* a press on a visible arrow steps at press-down, never wraps; hidden arrows are inert */
    pause_menu_pointer(p, 60, 228, 640, 480);
    assert(p->ps.car.cur == 0 && fx_marker_x(p) == 50 && !p->ps.exiting);
    pause_menu_pointer(p, 60, 228, 640, 480);
    assert(p->ps.car.cur == 0);
    pause_menu_pointer(p, 580, 228, 640, 480);
    assert(p->ps.car.cur == 1);
    pause_menu_pointer(p, 580, 228, 640, 480);
    assert(p->ps.car.cur == 2);
    pause_menu_pointer(p, 580, 228, 640, 480);
    assert(p->ps.car.cur == 2 && !p->ps.exiting);
    pause_menu_pointer(p, 60, 228, 640, 480);
    assert(p->ps.car.cur == 1);
    /* the Continue pill activates the focused entry: "options" is an ordinary entry, after the exit */
    pause_menu_pointer(p, 540, 302, 640, 480);
    assert(p->ps.exiting);
    assert(pause_menu_update(p, .3f) == FE_ACTION_NONE && p->open);
    assert(pause_menu_update(p, .03f) == FE_ACTION_REPLAY && !p->open);
    assert(pause_menu_update(p, 0) == FE_ACTION_NONE);            /* consumed once */
    /* reopen: fresh state, the creation frame ignores activation, Back resumes after the exit */
    pause_menu_open(p);
    assert(p->ui_ms == 0 && p->ps.car.cur == 0 && p->ps.fresh);
    pause_key(p, PMK_BACK);
    assert(!p->ps.exiting);
    pause_menu_update(p, .1f);
    pause_key(p, PMK_BACK);
    assert(p->ps.exiting && pause_menu_update(p, .1f) == FE_ACTION_NONE);
    assert(pause_menu_update(p, .3f) == FE_ACTION_RESUME && !p->open);
    fx_remove(p);
    free(p);
    puts("authored integration/moving marker/arrow-click/rail synthetic checks: PASS");
}

static void authored_sample_fixture(void) {
    /* Values are approved vector inputs to independent synthetic tracks, never production constants. */
    const int times[] = {0, 100, 200, 300, 600, 700, 800};
    const int alpha[4][7] = {{0, 78, 209, 235, 235, 235, 235}, {0, 33, 89, 100, 100, 100, 100}, {0, 22, 58, 65, 65, 65, 65}, {0, 58, 108, 90, 117, 162, 207}};
    UgFngObject objects[4] = {0};
    UgScript scripts[4] = {0};
    UgTrack tracks[4] = {0};
    UgKey keys[4][7] = {0};
    for (int j = 0; j < 4; j++) {
        for (int k = 0; k < 7; k++) keys[j][k] = (UgKey){.t = times[k], .v = {20, 30, 40, alpha[j][k]}};
        tracks[j] = (UgTrack){.type = UG_VT_COLOR, .vsize = 4, .length = 800, .prop = UG_P_COLOR, .keys = keys[j], .nkeys = 7};
        scripts[j] = (UgScript){.id = UG_SCRIPT_INITIAL, .length = 800, .tracks = &tracks[j], .ntracks = 1};
        objects[j] = (UgFngObject){.type = UG_OBJ_IMAGE, .guid = (uint32_t)(j + 1), .color = {20, 30, 40, 255}, .size = {100, 20}, .pos = {0, j * 25}, .scripts = &scripts[j], .nscripts = 1};
    }
    UgFngPackage package = {.obj = objects, .nobj = 4};
    UgScene scene;
    assert(ug_scene_init(&scene, &package, NULL));
    ug_scene_set_assets(&scene, NULL);
    printf("{\"effective_object_alpha_before_texture\":{");
    const char *names[] = {"outer", "selector", "caption", "highlight"};
    int values[4][7];
    float previous = 0;
    for (int k = 0; k < 7; k++) {
        ug_scene_update(&scene, times[k] - previous);
        previous = times[k];
        for (int j = 0; j < 4; j++) {
            values[j][k] = scene.nodes[j].wcolor.a;
            assert(abs(values[j][k] - alpha[j][k]) <= 1);
        }
    }
    for (int j = 0; j < 4; j++) {
        printf("%s\"%s\":[", j ? "," : "", names[j]);
        for (int k = 0; k < 7; k++) printf("%s%d", k ? "," : "", values[j][k]);
        printf("]");
    }
    printf("},\"byte_tolerance\":1}\n");
    ug_scene_free(&scene);
}

static void motion_vector(int argc, char **argv) {
    const char *id = argv[2];
    int n = argc > 3 ? atoi(argv[3]) : 3, initial = argc > 4 ? atoi(argv[4]) : 0;
    PauseMenu *p = (PauseMenu *)calloc(1, sizeof *p);
    binding_fixture_init(p);
    char spec[8];
    snprintf(spec, sizeof spec, "%d", n);
    fx_catalogue(p, spec, initial);
    if (!strncmp(id, "clamped_", 8) || !strcmp(id, "singleton")) {
        int left = gc_left_arrow_visible(&p->ps.car), right = gc_right_arrow_visible(&p->ps.car);
        printf("{\"marker_center\":[%.6f,188.4],\"left_arrow\":%s,\"right_arrow\":%s,\"Left_target_i\":%d}\n", fx_marker_x(p), left ? "true" : "false", right ? "true" : "false", n - 1);
    } else if (!strcmp(id, "continue_during_motion")) {
        pause_key(p, PMK_RIGHT);
        pause_menu_update(p, .08f);
        int selected = p->ps.car.cur;
        pause_menu_pointer(p, 540, 302, 640, 480);
        int count = 0;
        for (int t = 0; t < 50; t++) count += pause_menu_update(p, .01f) != FE_ACTION_NONE;
        printf("{\"invoked_entry\":%d,\"action_invocations\":%d}\n", selected, count);
    } else if (!strcmp(id, "glow_cycle")) {
        UgAnim animation;
        ug_anim_init(&animation, UG_WHITE, 0, 0);
        ug_anim_play(&animation, UG_ANIM_BREATHE, 0);
        float times[] = {0, 480, 800, 1200}, values[4], last = 0;
        for (int i = 0; i < 4; i++) { ug_anim_update(&animation, times[i] - last); last = times[i]; values[i] = ug_anim_alpha01(&animation); }
        printf("{\"normalized_glow_alpha\":[%.6f,%.6f,%.6f,%.6f]}\n", values[0], values[1], values[2], values[3]);
        ug_anim_free(&animation);
    }
    fx_remove(p);
    free(p);
}
