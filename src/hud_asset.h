/* Independent, bounded asset model for the approved interoperability spec. */
#ifndef HUD_ASSET_H
#define HUD_ASSET_H
#include <stdint.h>
#include <stddef.h>
#define HA_LIMIT 4096
#define HA_DEPTH 64
typedef struct { uint32_t id, kind; char *name; } HaResource;
typedef struct {
 uint32_t kind, name, id, parent_id; int parent, has_parent, appearance, valid;
 uint16_t resource, justification; unsigned char color[4];
 float pivot[3], position[3], quaternion[4], size[3], uv[4];
 char *sample;
} HaNode;
typedef struct { HaNode *nodes; size_t count; HaResource *resources; size_t resource_count; unsigned diagnostics; } HaPackage;
typedef struct { uint16_t code, x,y, advance; unsigned char width,height,top; int8_t bearing; } HaGlyph;
typedef struct { char name[256],texture[256]; HaGlyph *glyphs; size_t count; float inv_w,inv_h; uint32_t line; } HaFont;
uint32_t ha_hash(const char *s);
uint32_t ha_texture_hash(const char *alias);
int ha_package_select(const unsigned char *data,size_t len,const char *name,HaPackage *out);
int ha_header_name(const unsigned char *data,size_t len,const char *name);
int ha_object(const unsigned char *data,size_t len,HaNode *out);
int ha_package(const unsigned char *data,size_t len,HaPackage *out);
int ha_hierarchy(HaPackage *p);
void ha_free(HaPackage *p);
int ha_font(const unsigned char *data,size_t len,HaFont *font);
void ha_point(const HaPackage *p,size_t node,const float in[3],float out[3],float added_angle);
int ha_speed(double mps,int metric,char out[32]);
int ha_gear(int gear,char out[32]);
int ha_fit(int width,int height,float x,float y,float out[3]);
/* Project policy (not original behaviour): HUD fitment policy, selected at run
 * time. HA_FIT_WIDE is the project default (and the zero value). Both keep
 * s = min(W/640, H/480) with nothing stretched; `wide` additionally moves whole
 * top-level element groups outward by the extra logical half-width D when
 * W/H > 4/3, and equals `4:3` otherwise. */
enum { HA_FIT_WIDE=0, HA_FIT_43=1 };
/* Scale s and extra logical half-width D (0 unless wide and W/H > 4/3).
 * 0 for a nonpositive viewport. Either output may be NULL. */
int ha_fit_scale(int policy,int width,int height,double *scale,double *extra);
/* Group classification by the horizontal centre of its composed logical bounds
 * (frame centred at 0, before any shift): -1 left (cx < -640/6), +1 right
 * (cx > 640/6), 0 centred (exactly +-640/6, or nonfinite, is not moved). */
int ha_fit_anchor(double group_cx);
/* Logical translation t of a group with classification centre `group_cx`:
 * -D, 0 or +D; 0 for an invalid viewport. */
double ha_fit_translation(int policy,int width,int height,double group_cx);
/* Final output of logical point (x, y) in that group: out = {s, X, Y} with
 * X = W/2 + s*(x+t), Y = H/2 + s*y. Returns 0 for a nonpositive viewport. */
int ha_fit_point(int policy,int width,int height,double group_cx,double x,double y,double out[3]);
/* Command-line spelling: "wide" or "4:3" (also "4x3", "43"), case-insensitive.
 * Returns HA_FIT_* or -1 when unrecognised. ha_fit_name gives the canonical spelling. */
int ha_fit_parse(const char *name);
const char *ha_fit_name(int policy);
int ha_scale(double maximum);
double ha_rpm_angle(double rpm,double maximum,double initial);
int ha_needle_calibrate(HaNode *node);
int ha_skin(int skin);
int ha_cash(int career,int supplied,int suppressed,int64_t balance,char separator,char out[64]);
int ha_map(double ox,double oy,double extent,double x,double y,double heading,double window,double out[3]);
typedef struct { double center[2],basis[4],size[2],world[2],heading,window; int width,height,enabled; } HaMapFrame;
int ha_map_pick(const HaMapFrame *frame,double screen_x,double screen_y,double coverage,double out[2]);
float ha_text_start(float anchor,float scale,unsigned bits,float advances);
/* Capture set 1-3 amendments (approved SPEC sections "Runtime capture
 * amendments"). Pure, GL-free contracts; hud.c applies them when drawing. */
enum { HA_KIND_IMAGE=1, HA_KIND_TEXT=2, HA_KIND_GROUP=5 };
enum { HA_TEXT_SPEED, HA_TEXT_GEAR, HA_TEXT_UNIT, HA_TEXT_BANK_LABEL, HA_TEXT_BANK_VALUE };
/* C1-3/C3-3/E1 as amended by F1 (provisional). Gear text only: the gear-colour
 * setting replaces RGB (alpha stays authored) when (a) a forward gear is shown
 * and `change` (the host's optional "gear change in progress"; absent, false
 * or non-boolean all map to 0) is nonzero, or (b) forward gear 1 is shown and
 * |speed_mps| < `standstill`. A nonfinite speed or threshold, or a threshold
 * <= 0, makes no standstill claim (Q7). N and R (gear < 1), gears 2+ at
 * standstill without a change, and all other text roles keep the authored tint. */
#define HA_STANDSTILL_MPS 0.5 /* named setting default (guessed) */
void ha_text_tint(int role,int gear,int change,double speed_mps,double standstill,const unsigned char authored[4],const unsigned char gear_colour[3],unsigned char out[4]);
/* Set 4 D2: the highlight setting replaces the whole tint including alpha. */
void ha_setting_rgba(const unsigned char rgba[4],unsigned char out[4]);
/* Ordinary source-over result of texel*tint over an opaque background. */
void ha_blend_over(const unsigned char texel[4],const unsigned char tint[4],const unsigned char bg[3],double out[3]);
/* Set 4 D4: inherited phone/message visuals on the standard path only when
 * the authored widget is not drawn and a message is pending. */
int ha_inherited_message_visuals(int race_path,int widget_drawn,int pending);
/* Named colour settings replacing RGB only (shift C3-2/D2, gear). */
void ha_setting_tint(const unsigned char authored[4],const unsigned char rgb[3],unsigned char out[4]);
/* C1-4 dial angle in degrees [0,360) of a y-down offset from the needle's
 * composed rotation centre: 0 straight down, increasing screen-clockwise. */
double ha_dial_angle(double dx,double dy);
/* Indicated RPM as used by the needle: nonfinite -> 0, clamped to 0..max
 * (positive max) or 0..scale. */
double ha_indicated_rpm(double rpm,double maximum);
/* C1-5 opt-in inferred redline. 1: draw dial angles >= *start up to the
 * authored trailing end; 0: hidden; -1: hidden with diagnostic. */
int ha_redline(int enabled,int supplied,double redline_rpm,int scale,double *start);
/* C3-2 opt-in shift-up indicator: 1 visible, 0 hidden, -1 hidden with
 * diagnostic for a nonfinite or nonpositive shift RPM (set 4 D5). */
int ha_shift(int enabled,int supplied,double shift_rpm,int gear,double rpm,double maximum);
/* C3-4 opt-in inferred NOS level. 2: backing plus alpha arc up to *limit
 * degrees; 1: backing only; 0: hidden; -1: hidden with diagnostic. */
int ha_nos_level(int enabled,int installed,int supplied,double level,double a0,double a1,double *limit);
/* C1-8/C3-8 bank panel: backing, label and value share one visibility.
 * 1 visible; 0 hidden; -1 hidden with diagnostic (negative balance or
 * unresolved targets while the cash contract would show). */
extern const char ha_bank_label[6]; /* whitelisted interim display string (C1-8) */
int ha_bank(int career,int supplied,int suppressed,int64_t balance,char separator,int targets_resolved,char value[64]);
/* C2-4/C3-8 top-left widget element visibility. */
typedef struct { int visible, backing, dpad, centre, mail, highlight_right, diagnostic; } HaWidget;
void ha_widget(int career,int pending,int targets_resolved,int centre_texture_present,HaWidget *out);
/* C3-8 key resolution: exactly one descriptor carries `name`; kind matches;
 * parent is the root (parent_root) or has stored name `parent_name`; images
 * with a non-NULL alias match it case-insensitively. Returns index or -1. */
typedef struct { uint32_t name, kind; int parent_root; uint32_t parent_name; const char *alias; } HaKey;
int ha_lookup(const HaPackage *p,const HaKey *key);
/* Capture set 5 E3: opt-in N2O glow label. `t` is in animation units from the
 * event. Alpha raw keys (100,0) (340,255) (360,500) (480,500) (620,0) are
 * linear and the displayed byte is floor(raw) mod 256 (inferred wrap); size
 * and rotation follow the grow/hold/shrink keys. Returns visible (the label
 * is hidden before unit 100, from unit 800 and wherever alpha is 0). */
#define HA_GLOW_START 100.0
#define HA_GLOW_END 800.0
#define HA_GLOW_MS_PER_UNIT 0.82 /* named setting default (fitted) */
typedef struct { int visible; unsigned char alpha_byte; double alpha, width, height, rotation_ccw_deg; } HaGlow;
int ha_glow(double t,HaGlow *out);
/* Milliseconds to animation units; NaN for a nonfinite input or nonpositive scale. */
double ha_glow_units(double ms,double ms_per_unit);
/* 1 when a NOS glow event should (re)start the sequence: the setting is on,
 * the clock and scale are valid, and no sequence is running (guessed). */
int ha_glow_trigger(int enabled,double now_ms,int have_start,double start_ms,double ms_per_unit);
/* Capture set 7 G3: one glyph quad. Geometry comes from the stored cell
 * (bearing, top offset, width, height) scaled by (sx, sy) at pen `pen` and
 * text anchor y `anchor_y`; the advance plays no part (the caller steps the
 * pen by advance*sx). Texture rectangle is the whole cell; `inset` moves each
 * edge inward by half a texel (none for a 1-texel extent). No clipping of any
 * kind. Returns 0 (nothing to draw) for a zero width or height. */
typedef struct { double x0, y0, x1, y1, u0, v0, u1, v1; } HaGlyphQuad;
int ha_glyph_quad(const HaGlyph *g,float inv_w,float inv_h,double pen,double anchor_y,double sx,double sy,int inset,HaGlyphQuad *out);
/* Set 8 G5/G6 item 2: glyph atlas repack. Each non-empty cell of `glyphs`
 * is copied from the source RGBA atlas (w x h) into its own slot with a
 * one-texel gutter on all sides (diagonal corners included). Gutter texels
 * have alpha 0 and the RGB of the nearest cell texel, so bilinear sampling
 * just outside a cell reads zero alpha whatever the source neighbours or
 * atlas edge hold. cells[i] gives glyph i's new cell origin; ok = 0 for a
 * cell that does not lie inside the source atlas (empty cells are ok and take
 * no slot). Returns 1 on success, 0 on bad input, allocation failure or a
 * packed atlas over 4096 texels. Free with ha_glyph_atlas_free. */
typedef struct { uint16_t x, y; unsigned char ok; } HaGlyphCell;
typedef struct { unsigned char *rgba; unsigned width, height; HaGlyphCell *cells; size_t count; } HaGlyphAtlas;
#define HA_GLYPH_GUTTER 1
int ha_glyph_atlas(const unsigned char *rgba,unsigned w,unsigned h,const HaGlyph *glyphs,size_t count,HaGlyphAtlas *out);
void ha_glyph_atlas_free(HaGlyphAtlas *atlas);
/* Reference model of the sampler state the HUD sets (GL_LINEAR, GL_CLAMP_TO_EDGE,
 * no mipmap): bilinear between texel centres i+0.5 at normalised (u, v);
 * straight (non-premultiplied) RGBA in 0..1. */
void ha_sample_linear(const unsigned char *rgba,unsigned w,unsigned h,double u,double v,double out[4]);
/* Set 8 G7: coverage = filtered alpha (0..1) x tint alpha/255, linear, no
 * doubling, squaring or gamma. ha_glyph_over returns the source-over output
 * value (0..255) for one channel: texel (0..1) x tint/255 over `dst`. */
double ha_glyph_coverage(double alpha,double tint_alpha);
double ha_glyph_over(double texel,double alpha,double tint,double tint_alpha,double dst);
/* Capture set 7 H3/H4: shift-up icon. Transmission is an optional host
 * input; any value other than automatic applies H3 unchanged. */
enum { HA_TRANSMISSION_ABSENT, HA_TRANSMISSION_MANUAL, HA_TRANSMISSION_AUTOMATIC };
#define HA_SHIFT_CYCLE_UNITS 1200.0
#define HA_SHIFT_LIT_UNITS 580.0
#define HA_SHIFT_DARK_UNITS 600.0
/* ha_shift plus H4: an automatic transmission with `automatic_setting` off
 * hides the icon (0). Same 1 / 0 / -1 (diagnostic) results as ha_shift. */
int ha_shift_mode(int enabled,int supplied,double shift_rpm,int gear,double rpm,double maximum,int transmission,int automatic_setting);
/* H1/H3 item 5 flash envelope in 0..1 for `elapsed_ms` since the icon became
 * visible: u = elapsed/ms_per_unit mod 1200; 1 below 580, linear to 0 at
 * 600, 0 to 1200. Flash off: 1. Nonfinite or negative elapsed, or a
 * nonfinite or nonpositive time unit: 1 with *diagnostic = 1. */
double ha_shift_flash(int flash_enabled,double elapsed_ms,double ms_per_unit,int *diagnostic);
/* H3 item 5 phase (provisional): restarts at 0 whenever the icon becomes
 * visible. Returns ms since that moment; 0 while hidden; NaN for a
 * nonfinite clock. A clock earlier than the start restarts the phase. */
typedef struct { int shown; double start_ms; } HaShiftPhase;
double ha_shift_elapsed(HaShiftPhase *phase,int visible,double now_ms);
/* H1/H3 item 6 default shift-colour setting: the authored lit RGB. */
void ha_shift_default_colour(unsigned char out[3]);
#endif
