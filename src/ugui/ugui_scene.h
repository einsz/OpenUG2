/* ugui_scene.h -- runtime for a parsed .fng package (SPEC 3.5, 3.6, 4):
 * scene graph, script playback, message dispatch, focus, drawing. GL-free. */
#ifndef UGUI_SCENE_H
#define UGUI_SCENE_H

#include "ugui_fng.h"
#include "ugui_ui.h"
#include "ugui_assets.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UgScene UgScene;

typedef struct {
    const UgFngObject *obj;
    int parent;                 /* node index or -1 */
    UgProps props;              /* animated copy of the stored data */
    UgPlayer pl;
    UgScene *scene;
    int index;
    /* cached world state (valid after ug_scene_update/ug_scene_layout) */
    float wa, wb, wc, wd, we, wf, wz;
    float world[12]; /* row-major 3D affine matrix; project only after composition */
    UgColor wcolor;
    int visible;
    /* resolved assets (ug_scene_set_assets) */
    int tex;                     /* texture library index + 1, 0 = none */
    int untextured;              /* resource is the plain-quad placeholder (3.3.4) */
    const UgFace *face;          /* text font, NULL = current default */
    const char *text_override;   /* application-supplied text (replaces the stored string), or NULL */
    int tex_override;            /* application-supplied texture (library index + 1), or 0 */
    int app_hidden;              /* application decided this object (and its children) is not shown */
} UgNode;

struct UgScene {
    const UgFngPackage *pk;
    UgNode *nodes; int n;
    int focus;                   /* node index or -1 */
    int saved_focus;
    uint32_t appq[32]; int aqh, aqt;
    const UgStrTab *strings;
    int pc_build;                /* 1: hide console-only, show PC-only objects */
    int depth;
    int *order;                  /* draw order (back to front) */
    const UgAssets *assets;      /* original textures / fonts, or NULL when unavailable */
    int hide_hint_text;          /* hide text with object flag 0x4 (SPEC 3.4.2, 7.1) */
    int console_strip;           /* show console-only non-text objects (the strip background pieces, 7.1)
                                    while their text (controller-glyph hints) stays hidden on PC */
};

int  ug_scene_init(UgScene *s, const UgFngPackage *pk, const UgStrTab *strings);
void ug_scene_free(UgScene *s);
/* Binds textures and fonts from the loaded install (SPEC 15.9, 16.4): image
 * resources resolve through the texture key, text resources through the font
 * name. NULL leaves textured/font visuals unavailable. */
void ug_scene_set_assets(UgScene *s, const UgAssets *assets);
/* 1 when the package carries its own bottom strip (SPEC 7.1): a wide image
 * object whose centre lies in the strip band. */
int  ug_scene_has_bottom_strip(const UgScene *s);
/* number of visible image objects whose texture could not be resolved */
int  ug_scene_missing_textures(const UgScene *s);
void ug_scene_update(UgScene *s, float dt_ms);
/* Recompute world transforms and draw order after the application changed node
 * properties (positions, sizes, colours) following an update. */
void ug_scene_relayout(UgScene *s);
/* Posts a GUI message. target 0 / 0xFFFFFFFC broadcast, 0xFFFFFFFF to the app,
 * otherwise an object guid. */
void ug_scene_post(UgScene *s, uint32_t msg, uint32_t target);
/* Input action through the dispatch of SPEC 4.4. Returns 1 when handled. */
int  ug_scene_input(UgScene *s, UgAction a);
int  ug_scene_input_msg(UgScene *s, uint32_t msg);
int  ug_scene_poll_app(UgScene *s, uint32_t *msg);
void ug_scene_set_focus(UgScene *s, int node);
/* Moves a node so that its centre lies at world (x, y) (application-positioned objects such as the icon pool
 * of SPEC 6.4); uses the parent's current world matrix, so call it after ug_scene_update and follow with
 * ug_scene_relayout. */
void ug_scene_set_world_pos(UgScene *s, int node, float x, float y);
/* Re-sends "focus gained" to the focused button, e.g. once the enter animation is over: the data's
 * focus response does not start its highlight while the fade-in script runs (SPEC 3.6.2). */
void ug_scene_refocus(UgScene *s);
int  ug_scene_find_guid(const UgScene *s, uint32_t guid);
int  ug_scene_pick(const UgScene *s, float x, float y);   /* button node or -1 */
void ug_scene_pointer(UgScene *s, const UgPtr *p);
void ug_scene_draw(UgScene *s, UgCtx *c, UgCanvas *cv);
/* name of the script currently running on a node (0 = none) */
uint32_t ug_scene_script_id(const UgScene *s, int node);

#ifdef __cplusplus
}
#endif
#endif
