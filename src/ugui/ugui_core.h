/* ugui_core.h -- GL-free core of the ugui widget library.
 *
 * Covers: colour helpers, name hash (SPEC 2), animation tracks and script
 * playback (SPEC 3.5), the standard animation set (SPEC 5), the localisation
 * string table (SPEC 8.3).  Nothing here touches SDL or OpenGL.
 *
 * Original implementation written from SPEC.md only. */
#ifndef UGUI_CORE_H
#define UGUI_CORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Colour (SPEC 1.4)                                                   */
/* ------------------------------------------------------------------ */
typedef struct { uint8_t r, g, b, a; } UgColor;

#define UG_RGBA(R, G, B, A) ((UgColor){(R), (G), (B), (A)})
#define UG_ACCENT      UG_RGBA(202, 244, 138, 255)
#define UG_GREEN       UG_RGBA(99, 178, 34, 255)
#define UG_BORDER      UG_RGBA(197, 218, 165, 255)
#define UG_TEXT_DARK   UG_RGBA(50, 50, 50, 255)
#define UG_TEXT_LIGHT  UG_RGBA(230, 230, 230, 255)
#define UG_TEXT_HINT   UG_RGBA(192, 192, 192, 255)
#define UG_WHITE       UG_RGBA(255, 255, 255, 255)
#define UG_BLACK       UG_RGBA(0, 0, 0, 255)

/* Channel-wise product of SPEC 1.4 (amendment 2): (a*b + 127) / 255. */
UgColor ug_color_mul(UgColor a, UgColor b);
uint8_t ug_clamp8(float v);              /* round to nearest, clamp 0..255 */
UgColor ug_color_lerp(UgColor a, UgColor b, float t);
UgColor ug_color_alpha(UgColor c, float a01);  /* scale alpha by 0..1 */

/* ------------------------------------------------------------------ */
/* Name hash (SPEC 2)                                                  */
/* ------------------------------------------------------------------ */
uint32_t ug_hash(const char *s);                  /* h=~0; h=h*33+c */
uint32_t ug_hash_n(const char *s, size_t n);
uint32_t ug_hash_upper(const char *s);            /* hash of upper-cased ASCII */
/* Texture key: hash of upper-cased base name without extension. */
uint32_t ug_texture_key(const char *file_name);

/* Message / script constants (SPEC 3.5, 4.2, 4.3) [interop] */
#define UG_SCRIPT_INITIAL     0x001744B3u
#define UG_MSG_ENTER          0xbdfd2df9u
#define UG_MSG_EXIT           0xaadd0187u
#define UG_MSG_DIM            0xa301c015u
#define UG_MSG_UNDIM          0x9f218e63u
#define UG_MSG_ENTER_DONE     0x35f8620bu
#define UG_MSG_EXIT_DONE      0xe1fde1d1u
#define UG_MSG_REQ_BACK       0xcb1aa27fu
#define UG_MSG_REQ_FORWARD    0xade14ec3u
#define UG_MSG_REQ_DIM        0xc2a825e8u
#define UG_MSG_REQ_UNDIM      0x18bfc04bu
#define UG_MSG_ACCEPT         0x406415e3u
#define UG_MSG_BACK           0x911ab364u
#define UG_MSG_UP             0x72619778u
#define UG_MSG_DOWN           0x911c0a4bu
#define UG_MSG_LEFT           0x9120409eu
#define UG_MSG_RIGHT          0xb5971bf1u
#define UG_MSG_LTRIG          0x5073ef13u
#define UG_MSG_RTRIG          0xd9feec59u
#define UG_MSG_BUTTON0        0xc519bfbfu   /* button n = BUTTON0 + n */
#define UG_MSG_START          0xb5af2461u
#define UG_MSG_POINTER2       0x14dd0705u
#define UG_MSG_FOCUS_GAINED   0xabc08912u
#define UG_MSG_FOCUS_LOST     0x55d1e635u
#define UG_MSG_PRESSED        0x0c407210u
#define UG_MSG_DISABLED       0x5c7a5c2eu

/* ------------------------------------------------------------------ */
/* Animation tracks and scripts (SPEC 3.5)                             */
/* ------------------------------------------------------------------ */
enum { UG_VT_BOOL, UG_VT_INT, UG_VT_FLOAT, UG_VT_VEC2, UG_VT_VEC3,
       UG_VT_QUAT, UG_VT_COLOR };
enum { UG_INTERP_NONE, UG_INTERP_LINEAR, UG_INTERP_SPLINE,
       UG_INTERP_MOVE3, UG_INTERP_MOVE4 };
#define UG_ACT_WRAP     0x01   /* wrap interpolation around a looping track */
#define UG_ACT_CONSTANT 0x80   /* constant track (only a base key) */

/* Property block addressed by "word offset" (SPEC 3.4.3). Colour words hold
 * R,G,B,A (the file's B,G,R,A order is swapped by the parser). */
#define UG_P_COLOR   0
#define UG_P_PIVOT   4
#define UG_P_POS     7
#define UG_P_ROT     10
#define UG_P_SIZE    14
#define UG_P_UV0     17
#define UG_P_UV1     19
#define UG_P_CORNERS 21
#define UG_NPROPS    40
typedef struct { float w[UG_NPROPS]; } UgProps;

typedef struct { int32_t t; float v[4]; } UgKey;

typedef struct {
    uint8_t type, vsize, interp, action;
    uint32_t length;          /* track length ms */
    uint32_t prop;            /* property offset in words */
    int has_base;
    UgKey base;               /* the t = -1 key */
    UgKey *keys;              /* delta keys ascending in time */
    int nkeys;
} UgTrack;

typedef struct { uint32_t msg, target, t_ms; } UgEvent;

typedef struct {
    uint32_t id, length, flags;
    uint32_t chain;           /* valid when has_chain */
    int has_chain;
    UgTrack *tracks; int ntracks;
    UgEvent *events; int nevents;
} UgScript;
#define UG_SCRIPT_LOOP 1u

int  ug_type_ncomp(int vtype);
/* Evaluate one track at time t (ms). Colour values are rounded + clamped. */
void ug_track_eval(const UgTrack *tr, float t_ms, float out[4]);
void ug_script_free(UgScript *s);       /* frees owned arrays only */
void ug_script_apply(const UgScript *s, float t_ms, UgProps *p);

typedef void (*UgEmitFn)(void *user, const UgEvent *ev);
typedef struct {
    const UgScript *scripts; int nscripts;   /* chain lookup set */
    const UgScript *cur;
    float t;                                 /* ms */
    int running;
    UgEmitFn emit; void *user;
} UgPlayer;

void ug_player_init(UgPlayer *pl, const UgScript *scripts, int n,
                    UgEmitFn emit, void *user);
const UgScript *ug_player_find(const UgPlayer *pl, uint32_t id);
/* Start by id (0 on missing). Sets t=0, applies tracks, fires t=0 events. */
int  ug_player_start(UgPlayer *pl, uint32_t id, UgProps *p);
void ug_player_start_script(UgPlayer *pl, const UgScript *s, UgProps *p);
void ug_player_update(UgPlayer *pl, float dt_ms, UgProps *p);

/* ------------------------------------------------------------------ */
/* Standard animation set (SPEC 5)                                     */
/* ------------------------------------------------------------------ */
typedef enum {
    UG_ANIM_FADE_IN,      /* 180 */
    UG_ANIM_FADE_OUT,     /* 140 */
    UG_ANIM_DIM,          /* 100 */
    UG_ANIM_UNDIM,        /* 100 */
    UG_ANIM_FOCUS_PULSE,  /* 1200 loop */
    UG_ANIM_ROW_FOCUS,    /* 0 */
    UG_ANIM_ROW_UNFOCUS,  /* 0 */
    UG_ANIM_BLINK,        /* 400 */
    UG_ANIM_BREATHE,      /* 1200 loop */
    UG_ANIM_PILL_FOCUS,   /* 1200 loop (pill tint) */
    UG_ANIM_PILL_GLOW,    /* 1200 loop (glow overlay) */
    UG_ANIM_ACTIVITY,     /* 3600 loop, param = badge index 0..2 */
    UG_ANIM_STRIP_OUT,    /* 320 */
    UG_ANIM_STRIP_IN,     /* 320 */
    UG_ANIM_DISABLED,     /* 0 */
    UG_ANIM_COUNT
} UgStdAnim;

const char *ug_anim_name(UgStdAnim k);
int  ug_anim_duration(UgStdAnim k);          /* ms (script length) */
/* Build a script for `k` around the resting property values `rest`. The
 * returned script owns its arrays (ug_script_free). */
void ug_anim_build(UgStdAnim k, const UgProps *rest, int param, UgScript *out);
void ug_props_default(UgProps *p, UgColor c, float w, float h);

/* A small owner: script + player + live property block. */
typedef struct {
    UgScript sc;
    UgPlayer pl;
    UgProps props;
    UgProps rest;
    int has_script;
    UgStdAnim kind;
} UgAnim;
void ug_anim_init(UgAnim *a, UgColor c, float w, float h);
void ug_anim_free(UgAnim *a);
void ug_anim_play(UgAnim *a, UgStdAnim k, int param);   /* uses a->rest */
void ug_anim_stop(UgAnim *a);                           /* back to rest */
void ug_anim_update(UgAnim *a, float dt_ms);
int  ug_anim_running(const UgAnim *a);
UgColor ug_anim_color(const UgAnim *a);
float ug_anim_alpha01(const UgAnim *a);

/* ------------------------------------------------------------------ */
/* String table (SPEC 8.3)                                             */
/* ------------------------------------------------------------------ */
typedef struct { uint32_t hash; char *text; } UgStrEntry;
typedef struct {
    UgStrEntry *e; int n, cap;
    char name[17];
} UgStrTab;

void ug_strtab_init(UgStrTab *t);
void ug_strtab_free(UgStrTab *t);
/* `chunk` points at the chunk header (id 0x00039000, size, payload). */
int  ug_strtab_load(UgStrTab *t, const uint8_t *chunk, size_t len,
                    char *err, size_t errlen);
int  ug_strtab_add(UgStrTab *t, const char *label, const char *utf8);
const char *ug_strtab_get(const UgStrTab *t, uint32_t hash);  /* NULL = miss */
/* Text for hash or `fallback` when missing / table is NULL. */
const char *ug_strtab_or(const UgStrTab *t, uint32_t hash, const char *fallback);

size_t ug_utf8_encode(uint32_t cp, char out[4]);
/* Decode one code point; advances *s. Invalid bytes decode as U+FFFD. */
uint32_t ug_utf8_next(const char **s);

#ifdef __cplusplus
}
#endif
#endif
