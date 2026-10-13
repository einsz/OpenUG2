/* ugui_fng.h -- screen package (.fng) reader, SPEC 3. GL-free.
 *
 * ug_fng_parse() decodes a raw package (or a bundle chunk 0x00030203 /
 * 0x00030210 holding an uncompressed package) into plain structs. Compressed
 * packages (JDLZ/HUFF) are decoded first (ugui_codec.h), SPEC 3.2 / 13. */
#ifndef UGUI_FNG_H
#define UGUI_FNG_H

#include "ugui_core.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { UG_OBJ_IMAGE = 1, UG_OBJ_TEXT = 2, UG_OBJ_MODEL = 3, UG_OBJ_LIST = 4,
       UG_OBJ_GROUP = 5, UG_OBJ_CODELIST = 6, UG_OBJ_MOVIE = 7,
       UG_OBJ_EFFECT = 8, UG_OBJ_GRADIENT = 9, UG_OBJ_ANIMIMAGE = 10,
       UG_OBJ_SIMPLEIMAGE = 11, UG_OBJ_MULTIIMAGE = 12 };

/* object flags (SPEC 3.4.2) */
#define UG_OF_INVISIBLE   0x00000001u
#define UG_OF_PC_ONLY     0x00000008u
#define UG_OF_CONSOLE     0x00000040u
#define UG_OF_NO_DIRFOCUS 0x00080000u
#define UG_OF_BUTTON      0x10000000u

/* message response commands (SPEC 3.6.2) */
enum {
    UG_CMD_START_SCRIPT = 0x000, UG_CMD_POST_GUI = 0x001,
    UG_CMD_POST_APP = 0x002, UG_CMD_POST_SOUND = 0x003,
    UG_CMD_FOCUS = 0x100, UG_CMD_INPUT_ENABLE = 0x101,
    UG_CMD_FOCUS_SAVE = 0x102, UG_CMD_FOCUS_RESTORE = 0x103,
    UG_CMD_SCREEN_REPLACE = 0x200, UG_CMD_SCREEN_POP = 0x203,
    UG_CMD_IF_SCRIPT_IS = 0x300, UG_CMD_IF_SCRIPT_NOT = 0x301,
    UG_CMD_ELSE = 0x500, UG_CMD_ENDIF = 0x501
};

typedef struct { uint32_t id; uint32_t name_off; uint32_t kind; char *name; } UgFngResource;

typedef struct {
    uint32_t id;       /* command id */
    uint32_t arg;      /* Ru */
    uint32_t target;   /* Rt */
    char *str;         /* Rs or NULL */
} UgFngCmd;

typedef struct {
    uint32_t msg;
    UgFngCmd *cmds; int ncmds;
} UgFngResponse;

typedef struct {
    uint32_t type, guid, name_hash, flags, parent_guid;
    int32_t  res_index;
    int      has_parent;
    UgColor  color;                    /* R,G,B,A */
    float    pivot[3], pos[3], rot[4], size[3];
    float    uv[4];                    /* tl u,v ; br u,v (images) */
    int      has_uv;
    UgColor  corners[4];               /* gradient image: TL TR BR BL */
    int      has_corners;
    float    extra[15];                /* multi-image words */
    int      nextra;
    /* text */
    int      is_text;
    char    *text;                     /* UTF-8 default text */
    uint32_t buffer_len, justify, max_width, label_hash;
    int32_t  leading;
    int      has_label_hash;
    uint32_t image_flags;
    UgScript *scripts; int nscripts;
    UgFngResponse *resp; int nresp;
} UgFngObject;

typedef struct { uint32_t msg; uint32_t *guids; int nguids; } UgFngTarget;

typedef struct {
    uint32_t header_words[6];  /* version, 0, nres, nroot, N1, N2 */
    char *name, *path;
    int ntypes; uint32_t (*type_sizes)[2];
    UgFngResource *res; int nres;
    uint32_t button_count; int has_button_count;
    UgFngObject *obj; int nobj;
    UgFngResponse *pkg_resp; int npkg_resp;
    uint32_t target_count; int has_targets;
    UgFngTarget *targets; int ntargets;
} UgFngPackage;

/* Returns 1 on success; on failure returns 0 and fills `err`. */
int  ug_fng_parse(const uint8_t *data, size_t len, UgFngPackage *out,
                  char *err, size_t errlen);
void ug_fng_free(UgFngPackage *p);
const UgFngObject *ug_fng_find_guid(const UgFngPackage *p, uint32_t guid);
/* Reads only the package name from `PkHd` (malloc'ed, NULL when absent). Used to
 * key packages without a full parse (SPEC 14.3). */
char *ug_fng_peek_name(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif
#endif
