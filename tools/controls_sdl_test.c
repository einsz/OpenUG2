/* controls_sdl_test -- the SDL side of the Controls feature (src/controls_sdl.c) against an SDL virtual
 * joystick: device discovery and defaults, frame samples for the capture, the driving lookups, and
 * persistence across cx_shutdown/cx_init. Headless (joystick subsystem only). If this SDL has no virtual
 * joystick support the test skips. User directories are redirected to build/controls_sdl_tmp.
 * Exit code is nonzero if any check fails. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "controls_sdl.h"
#include "controls_store.h"

static int g_checks, g_fail;
#define CHECK(cond, ...) do { g_checks++; if (!(cond)) { g_fail++; \
    if (g_fail <= 60) { printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } } while (0)

static int find_virtual(const CxInput *c) {
    for (int i = 0; i < CB_MAX_PADS; i++) {
        if (!c->joy[i]) continue;
        for (int di = 0; di < SDL_NumJoysticks(); di++)
            if (SDL_JoystickIsVirtual(di) && SDL_JoystickGetDeviceInstanceID(di) == SDL_JoystickInstanceID(c->joy[i])) return i;
    }
    return -1;
}

int main(void) {
    setenv("HOME", "build/controls_sdl_tmp", 1);
    setenv("XDG_CONFIG_HOME", "/tmp/claude-1000/controls_sdl_cfg_unused", 0);
    unsetenv("XDG_CONFIG_HOME");
    unsetenv("XDG_DATA_HOME");
    if (SDL_Init(SDL_INIT_JOYSTICK) != 0) { printf("controls_sdl_test: SKIP (%s)\n", SDL_GetError()); return 0; }
    int vi = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 12, 1);
    if (vi < 0) { printf("controls_sdl_test: SKIP (no virtual joystick: %s)\n", SDL_GetError()); SDL_Quit(); return 0; }
    if (system("rm -rf build/controls_sdl_tmp")) return 1;

    CxInput *c = (CxInput *)calloc(1, sizeof *c);
    cx_init(c);
    int vp = find_virtual(c);                       /* the machine may have real devices too: find ours */
    CHECK(vp >= 0 && c->b.pad[vp].present, "the virtual pad is among the pads");
    if (vp < 0) { printf("controls_sdl_test: %d checks, %d failed\n", g_checks, g_fail); return 1; }
    for (int i = 0; i < CB_MAX_PADS; i++) if (i != vp) c->b.pad[i].present = 0;   /* other devices must not colour the lookups */
    CHECK(c->b.pad[vp].info.buttons == 12 && c->b.pad[vp].info.axes_mask == 0x3F && !c->b.pad[vp].info.wheel, "pad info: buttons %d mask %x",
          c->b.pad[vp].info.buttons, c->b.pad[vp].info.axes_mask);
    const StInputDevice *d = &c->b.pad[vp].dev;
    CHECK(d->binding[0] == CB_ACT_BRAKE && d->binding[1] == CB_ACT_THROTTLE && d->binding[2] == CB_ACT_LOOK_BACK, "12 buttons: the >= 8 button defaults");
    CHECK(d->assigned[0] && d->axis[CB_ROLE_COMBINED] == 20 && d->assigned[3] && d->combined_mode, "six axes: steering X, pedals on Rz");

    /* names: no platform name for a raw axis without a controller mapping -> NULL */
    CmPlatform plat;
    cx_platform(c, &plat);
    CHECK(plat.key_name(plat.user, SDL_SCANCODE_W) && !strcmp(plat.key_name(plat.user, SDL_SCANCODE_W), "W"), "SDL key name");
    CbCtl axis = { CB_CTL_AXIS, 0 };
    (void)plat.ctl_name(plat.user, vp, axis);        /* must not crash whatever the mapping says */

    /* samples and lookups */
    SDL_Joystick *j = c->joy[vp];
    SDL_JoystickSetVirtualButton(j, 1, 1);          /* bound to Throttle */
    SDL_JoystickUpdate();
    CbPadSample s;
    cx_sample(c, vp, &s);
    CHECK((s.buttons & 2) && s.pov[0] == -1, "sample: button 1 down, hat centred");
    CHECK(cx_action_down(c, NULL, CB_ACT_THROTTLE) && !cx_action_down(c, NULL, CB_ACT_BRAKE), "pad button -> Throttle");
    CHECK(cx_throttle(c, NULL) == 1.0f, "digital throttle");
    SDL_JoystickSetVirtualButton(j, 1, 0);
    SDL_JoystickSetVirtualHat(j, 0, SDL_HAT_LEFT);  /* Steering (Left) on every POV left */
    SDL_JoystickUpdate();
    cx_sample(c, vp, &s);
    CHECK(s.pov[0] == 27000, "hat left is 27000 centidegrees");
    CHECK(cx_steer(c, NULL) == 1.0f, "POV left steers left (+1)");
    SDL_JoystickSetVirtualHat(j, 0, SDL_HAT_RIGHTUP);
    SDL_JoystickUpdate();
    cx_sample(c, vp, &s);
    CHECK(s.pov[0] == 4500 && cb_pov_direction(s.pov[0]) == CB_DIR_RIGHT, "a diagonal reads as right");
    CHECK(cx_steer(c, NULL) == -1.0f, "POV right steers right (-1)");
    SDL_JoystickSetVirtualHat(j, 0, SDL_HAT_CENTERED);
    /* axes: steering X with direction +1: positive = right */
    SDL_JoystickSetVirtualAxis(j, 0, 32767);
    SDL_JoystickUpdate();
    CHECK(cx_steer(c, NULL) < -0.99f, "X full right steers -1");
    SDL_JoystickSetVirtualAxis(j, 0, -32768);
    SDL_JoystickUpdate();
    CHECK(cx_steer(c, NULL) > 0.99f, "X full left steers +1");
    SDL_JoystickSetVirtualAxis(j, 0, 1000);
    SDL_JoystickUpdate();
    CHECK(cx_steer(c, NULL) == 0.0f, "inside the dead zone");
    /* combined pedals on Rz, direction +1: negative output = throttle */
    SDL_JoystickSetVirtualAxis(j, 5, -32768);
    SDL_JoystickUpdate();
    CHECK(cx_throttle(c, NULL) > 0.99f, "combined axis pushed negative: throttle");
    SDL_JoystickSetVirtualAxis(j, 5, 32767);
    SDL_JoystickUpdate();
    CHECK(cx_throttle(c, NULL) < -0.99f, "combined axis pushed positive: brake");
    SDL_JoystickSetVirtualAxis(j, 5, 0);
    SDL_JoystickSetVirtualAxis(j, 0, 0);
    SDL_JoystickUpdate();
    /* keyboard table */
    Uint8 keys[SDL_NUM_SCANCODES];
    memset(keys, 0, sizeof keys);
    keys[SDL_SCANCODE_W] = 1;
    CHECK(cx_action_down(c, keys, CB_ACT_THROTTLE) && cx_throttle(c, keys) == 1.0f, "W is the Throttle secondary");
    keys[SDL_SCANCODE_W] = 0;
    keys[SDL_SCANCODE_DOWN] = 1; keys[SDL_SCANCODE_LEFT] = 1;
    CHECK(cx_throttle(c, keys) == -1.0f && cx_steer(c, keys) == 1.0f, "Down brakes, Left steers left");
    CHECK(cx_key_is(c, SDL_SCANCODE_ESCAPE, CB_ACT_PAUSE) && cx_key_is(c, SDL_SCANCODE_P, CB_ACT_PAUSE) && !cx_key_is(c, SDL_SCANCODE_Q, CB_ACT_PAUSE),
          "Pause is Escape and P");
    CHECK(cx_key_bound(c, SDL_SCANCODE_K) && cx_key_bound(c, SDL_SCANCODE_PERIOD) && !cx_key_bound(c, SDL_SCANCODE_G), "developer keys: K and . are bound, G is not");
    CHECK(cx_joy_button_is(c, SDL_JoystickInstanceID(j), 1, CB_ACT_THROTTLE) && !cx_joy_button_is(c, SDL_JoystickInstanceID(j), 1, CB_ACT_PAUSE), "button event lookup");

    /* rebind, save, restart */
    cb_kb_capture(&c->b.kb, CB_ACT_NITROUS, 0, SDL_SCANCODE_N);
    cb_pad_assign_control(&c->b.pad[vp].dev, CB_ACT_NITROUS, 0, 9);
    for (int i = 0; i < CB_MAX_PADS; i++) if (c->joy[i] && i != vp) c->b.pad[i].present = 0;
    CHECK(cx_save(c) == 0, "save");
    cx_shutdown(c);
    memset(c, 0, sizeof *c);
    cx_init(c);
    vp = find_virtual(c);
    CHECK(vp >= 0, "the pad is found again");
    if (vp < 0) vp = 0;
    CHECK(cx_key_is(c, SDL_SCANCODE_N, CB_ACT_NITROUS) && !cx_key_is(c, SDL_SCANCODE_LALT, CB_ACT_NITROUS), "keyboard binding survives a restart");
    CHECK(c->b.pad[vp].present && c->b.pad[vp].dev.binding[9] == CB_ACT_NITROUS && c->b.pad[vp].dev.binding[8] == CB_ACT_HYD_PRESSURE, "pad record survives a restart");
    cx_shutdown(c);
    free(c);
    SDL_JoystickDetachVirtual(vi);
    SDL_Quit();
    printf("controls_sdl_test: %d checks, %d failed\n", g_checks, g_fail);
    return g_fail ? 1 : 0;
}
