/* GL-free host-side construction of the HUD's context inputs. main.c owns the
 * live engine values; this header owns the policies that decide which spec
 * inputs are supplied (docs/cleanroom-hud/SPEC.md: "Inputs to the HUD", Cash,
 * C2-4/C3-8 admissions, C3-2, C3-4). Kept static-inline so the unit tests can
 * exercise it without linking the engine. */
#ifndef OPENUG2_HUD_HOST_H
#define OPENUG2_HUD_HOST_H

#include <math.h>
#include "hud.h"
#include "hud_asset.h"

/* Shift indicator (C3-2, amended by set 7 H). The spec leaves the shift RPM
 * to the host (H2: the HUD reads no car data; this policy reads none either,
 * only the virtual gearbox's own constant). The
 * virtual automatic gearbox upshifts the moment its computed RPM is strictly
 * above its upshift point and, in that same step, drops into the clutch window
 * without publishing that RPM, so the HUD's RPM never reaches the upshift
 * point itself in gears below top: an indicator keyed to it would only ever
 * light in top gear. The host therefore reports a shift RPM a fixed lead below
 * the automatic upshift point, the "shift now" band the gearbox is about to
 * act on. */
#define HUD_HOST_SHIFT_LEAD_RPM 500.0f
static inline float hud_host_shift_rpm(float auto_upshift_rpm) {
    if (!isfinite(auto_upshift_rpm) || auto_upshift_rpm <= 0.0f) return 0.0f;
    if (auto_upshift_rpm <= 2.0f * HUD_HOST_SHIFT_LEAD_RPM) return auto_upshift_rpm;
    return auto_upshift_rpm - HUD_HOST_SHIFT_LEAD_RPM;
}

/* H3/H4 host inputs. The icon is default-on in the HUD, so the host decides
 * whether it can appear by supplying (or withholding) the shift RPM: an
 * invalid upshift point is withheld rather than supplied as 0, which would
 * only produce the D5 diagnostic. No top-gear exclusion (H3 item 4: none is
 * specified; this host supplies the RPM in every gear). The transmission mode
 * is reported honestly: the free-roam virtual gearbox shifts by itself
 * (automatic); the drag drivetrain is shifted by the player (manual). */
static inline void hud_host_shift(HudState *s, float auto_upshift_rpm, int manual) {
    float rpm = hud_host_shift_rpm(auto_upshift_rpm);
    s->have_shift_rpm = rpm > 0.0f;
    s->shift_rpm = rpm;
    s->transmission = (unsigned char)(manual ? HA_TRANSMISSION_MANUAL : HA_TRANSMISSION_AUTOMATIC);
}

/* Context input. The engine has no career layer, so the only context it can
 * honestly state is "free roam" (not in an event). Spec: the Bank panel and
 * the top-left widget are career free-roam elements; this supplies that
 * context and nothing else. The Bank additionally needs a real balance, so it
 * stays hidden without --hud-preview. */
static inline void hud_host_context(HudState *s) {
    s->career_context = s->racing ? 0 : 1;
}

/* --hud-preview sample values. Used only for readouts with no live source;
 * a live value is never replaced. Every value is a labelled sample (the
 * auxiliary preview banner is drawn by the baseline whenever preview is on). */
#define HUD_PREVIEW_MONEY 250000L
#define HUD_PREVIEW_TANK_START 0.75f
#define HUD_PREVIEW_TANK_DRAIN_PER_S 0.25f   /* about 4 s from full (E4) */
#define HUD_PREVIEW_TANK_REFILL_PER_S 0.05f  /* slow refill, 20 s empty to full */

/* The engine has no tank model, so the preview tank drains while N is held
 * and refills slowly otherwise. Bad inputs leave a clamped, finite level. */
static inline float hud_host_preview_tank_step(float level, int n_held, float dt) {
    if (!isfinite(level)) level = HUD_PREVIEW_TANK_START;
    if (!isfinite(dt) || dt < 0.0f) dt = 0.0f;
    if (dt > 0.25f) dt = 0.25f;           /* a stalled frame must not empty it */
    level += (n_held ? -HUD_PREVIEW_TANK_DRAIN_PER_S : HUD_PREVIEW_TANK_REFILL_PER_S) * dt;
    return level < 0.0f ? 0.0f : level > 1.0f ? 1.0f : level;
}

static inline void hud_host_preview_apply(HudState *s, float tank_level) {
    if (!s->racing) {                     /* career free-roam readouts */
        s->career_context = 1;
        if (!s->have_money) { s->have_money = 1; s->money = HUD_PREVIEW_MONEY; }
        if (!s->message_waiting) s->message_waiting = 1;
    }
    if (!s->have_nitro_tank) { s->have_nitro_tank = 1; s->nitro_tank = tank_level; }
}

#endif
