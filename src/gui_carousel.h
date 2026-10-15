/* gui_carousel.h -- icon carousel engine of the pause menu and the pause options hub
 * (SPEC_GUI_ART 3, tvg08). Pure layout/animation state, no GL/SDL, no package access: the view
 * (gui_view.h) applies the numbers to the authored objects. Coordinates are centred 640x480 units. */
#ifndef OPENUG2_GUI_CAROUSEL_H
#define OPENUG2_GUI_CAROUSEL_H

#ifdef __cplusplus
extern "C" {
#endif

#define GC_MAX 10                    /* option_1 ... option_10 */
#define GC_PITCH 158.0f              /* slot width 118 + spacing 40 (3.2) */
#define GC_ITEM_W 118.0f
#define GC_ITEM_H 59.0f
#define GC_SMALL 0.8f                /* size of every non-focused slot (3.2) */
#define GC_WINDOW 240.0f             /* a slot is shown while |centre - master| <= 240 */
#define GC_TWEEN_S 0.3f              /* cubic Hermite scroll animation (3.2) */
#define GC_FADE_W 94.4f              /* edge-fade ramp width = 0.8 x slot width (3.3) */
#define GC_RAIL_PAUSE 562.0f         /* ScrollBar_Backing widths (3.1) */
#define GC_RAIL_HUB 564.0f

typedef struct GuiCarousel {
    float master_x, master_y;        /* OPTION_MASTER centre (0.388, -14.296) */
    float rail_cx, rail_cy, rail_w;  /* ScrollBar_Backing centre (3, -52) and width */
    int n;                           /* entries */
    int cur;                         /* focused entry, 0-based */
    int disabled[GC_MAX];
    float scroll;                    /* current scroll offset s (virtual units; -x_focus at rest) */
    float from, to, vel;             /* tween: start offset, target offset, start velocity (units/s) */
    float t;                         /* seconds into the tween */
    int tweening;
    int fade;                        /* edge fade enabled (3.3) */
} GuiCarousel;

void gc_init(GuiCarousel *c, float rail_w);
void gc_set_master(GuiCarousel *c, float x, float y);
/* Replaces the entry list: focus on the first enabled entry, scroll snaps (first build, 3.2). */
void gc_set_entries(GuiCarousel *c, int n, const int *disabled);
/* Focus entry i (0-based) and snap the scroll to rest, no animation. */
void gc_snap_to(GuiCarousel *c, int i);
/* Moves focus to the next enabled entry in `dir` (<0 left, >0 right). No wrap: at the ends nothing
 * happens. The focus changes at once; only the scroll offset is animated. Returns 1 when moved. */
int gc_step(GuiCarousel *c, int dir);
void gc_update(GuiCarousel *c, float seconds);
int gc_tweening(const GuiCarousel *c);
float gc_scroll_value(const GuiCarousel *c);   /* current offset s */

void gc_enable_fade(GuiCarousel *c, int on);

typedef struct GcSlot { float cx, cy, w, h; int visible; } GcSlot;
GcSlot gc_slot(const GuiCarousel *c, int i);
/* Edge-fade factor f in 0..1 for slot i (3.3); always 1 while the fade is disabled. */
float gc_fade_factor(const GuiCarousel *c, int i);
/* Colour alpha of slot i: 255 while the fade is disabled, else trunc(f * A), A = 150 for a disabled
 * entry, 255 otherwise. */
int gc_alpha(const GuiCarousel *c, int i);

int gc_tab_visible(const GuiCarousel *c);        /* more than one entry */
float gc_tab_x(const GuiCarousel *c);            /* centred x of ScrollBar_Handle */
int gc_left_arrow_visible(const GuiCarousel *c); /* not on the first entry (and N > 1) */
int gc_right_arrow_visible(const GuiCarousel *c);

/* ---- pointer (centred coordinates; edges inclusive, SPEC_GUI_ART 4.6 style) -------------------------
 * A press on a visible navigation arrow steps at press-down, exactly like Left/Right; a press on the
 * focused slot arms an activation that fires on the release when it ends on the slot again. Neighbour
 * slots, hidden arrows, the rail and the tab are inert. */
#define GC_ARROW_L_X (-250.291f)     /* centres of ScrollBar_Arrow_1 / _2 (26x26) */
#define GC_ARROW_R_X 259.140f
#define GC_ARROW_Y (-12.769f)
#define GC_ARROW_SIZE 26.0f
typedef enum { GC_PT_NONE = 0, GC_PT_LEFT, GC_PT_RIGHT, GC_PT_ACTIVATE } GcPtAction;
typedef struct GcPointer { int armed; } GcPointer;
int gc_arrow_at(float x, float y);                        /* -1 left arrow box, +1 right, 0 none (geometry only) */
int gc_slot_hit(const GuiCarousel *c, float x, float y);  /* inside the 118x59 selection slot about OPTION_MASTER */
GcPtAction gc_pointer_press(const GuiCarousel *c, GcPointer *p, float x, float y);
GcPtAction gc_pointer_release(const GuiCarousel *c, GcPointer *p, float x, float y);

#ifdef __cplusplus
}
#endif
#endif
