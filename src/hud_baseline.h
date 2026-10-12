/* Preserved permitted independent baseline auxiliary renderer. */
#ifndef HUD_BASELINE_H
#define HUD_BASELINE_H
#include "hud.h"
typedef struct HudBaseline HudBaseline;
HudBaseline *hud_baseline_init(const char *root);
void hud_baseline_free(HudBaseline **h);
void hud_baseline_draw(HudBaseline *h,const RProg *rp,GpuMesh *quad,const HudState *s,int w,int height);
void hud_baseline_set_preview(HudBaseline *h,int on);
int hud_baseline_preview(const HudBaseline *h);
const char *hud_baseline_missing(const HudBaseline *h);
/* ---- pure helpers, unit-tested without GL (see tools/hud_baseline_test.c) ---- */

/* Signed angle from `from` to `to`, wrapped to (-pi, +pi]. The navigation
 * arrow rotates by this, so the wrap must be exact or the arrow spins the long
 * way round every time the car crosses the +/-pi seam. */
float hud_baseline_angle_delta(float from, float to);

/* Current speed magnitude, converted for display and truncated to an integer.
 * The input contract remains km/h in either display mode. Invalid telemetry
 * returns zero; oversized telemetry saturates to avoid an undefined C cast. */
int hud_baseline_speed_value(float speed_kmh, int imperial);

/* Current sampled RPM, bounded by the engine maximum, as a fraction of the
 * selected visual dial scale. Stateless: render cadence adds no extra lag.
 * A missing maximum uses redline for existing/provisional callers. */
float hud_baseline_tach_fraction(const HudState *s, float dial_scale);

/* World -> minimap transform. The map is a north-up-or-heading-up window of
 * radius `radius_m` around (cx,cy); `rot` is the rotation applied (0 for
 * north-up, -heading-pi/2 for heading-up). Writes normalised map coordinates
 * in [-1,1] where 1 = the window edge; the caller scales into its panel.
 * Points outside the window are returned unclamped so the caller can cull. */
void hud_baseline_map_project(float wx, float wy, float cx, float cy,
                     float radius_m, float rot, float *mx, float *my);

/* The secondary status line, as a pure function of the state, because the
 * rule it encodes is a correctness requirement rather than a style choice:
 * a SPRINT has no laps, so printing "LAP 1/2" there would be a fabricated
 * circuit readout. Circuits get laps, sprints get real gate progress, and
 * free roam gets nothing. Always NUL-terminates. */
void hud_baseline_status_text(const HudState *s, char *out, int cap);

/* Is the N2O section present at all? Fitted parts are removed, not greyed
 * out: an unfitted car shows no nitrous bar and the cluster closes up around
 * the gap. `have_turbo` is the matching switch for a boost dial; the standard
 * HUD has no turbo art to draw (the only turbo gauge in the game is
 * DRAG_TURBO_* inside InGameDrag.bun, which belongs to the drag HUD), so
 * nothing consumes it yet and an unfitted turbo is simply the whole story. */
int hud_baseline_shows_nos(const HudState *s);

/* Money is hidden during a race, and shown in free roam only when a career
 * economy actually exists. Both halves are requirements, so both are tested. */
int hud_baseline_shows_money(const HudState *s);

/* Fraction of texels whose quantised colour equals their left neighbour's.
 * HUD art is flat-shaded, so this is high; a mis-decoded record is noise, so
 * it is low. Measured on the shipped packs: every usable sprite scores
 * 0.719..0.996 (HUD_CINGULAR_LOGO 0.992, MINIMAP_BORDER 0.978, the 16x16
 * MINIMAP_ICON_CAR 0.719), while ARROW_MARKERA -- which decodes to coloured
 * noise -- scores 0.192. hud_baseline_init refuses anything below HUD_ART_RUN_MIN.
 * rgb is w*h*3, top-down. */
#define HUD_ART_RUN_MIN 0.50f
float hud_baseline_art_run_fraction(const unsigned char *rgb, int w, int h);

/* Hit-test a click in NDC against the minimap panel and convert it to world
 * coordinates, so a destination can be chosen from the map with no developer
 * UI present. Returns 0, leaving the outputs alone, when the point is outside
 * the panel. The inverse of the transform hud_baseline_draw uses, same frame. */
int hud_baseline_map_pick(float ndc_x, float ndc_y,
                 int width, int height, const HudState *s,
                 float *wx, float *wy);

/* Uniform-scale layout: converts a position in HUD units (a 2 x 2/aspect box
 * whose HEIGHT is always 2.0, so a unit is the same on screen at 16:9 and 4:3)
 * into NDC. anchor_x/anchor_y in [-1,1] select the screen corner to hang off.
 * This is what keeps proportions and safe margins identical across aspects. */
void hud_baseline_place(float aspect, float anchor_x, float anchor_y,
               float off_x, float off_y, float *ndc_x, float *ndc_y);


#endif
