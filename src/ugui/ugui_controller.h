/* Original controller resource profiles, SPEC 20.3-20.5. */
#ifndef UGUI_CONTROLLER_H
#define UGUI_CONTROLLER_H
#include "ugui_tex.h"
typedef enum { UG_VIS_OK, UG_VIS_UNAVAILABLE, UG_VIS_UNBOUND } UgVisualStatus;
typedef enum { UG_CONTROLLER_PS2, UG_CONTROLLER_XBOX, UG_CONTROLLER_PC } UgControllerFamily;
typedef enum { UG_FOLDER_OK, UG_FOLDER_ABSENT, UG_FOLDER_MISSING, UG_FOLDER_UNREADABLE,
    UG_FOLDER_ESCAPE, UG_FOLDER_HASH, UG_FOLDER_PNG, UG_FOLDER_UNSUPPORTED,
    UG_FOLDER_DIMENSIONS, UG_FOLDER_RGBA, UG_FOLDER_LIMIT, UG_FOLDER_INDEX } UgGlyphFolderStatus;
typedef struct { UgControllerFamily family; uint32_t key; int texture; UgGlyphFolderStatus status; } UgGlyphFolderRecord;
typedef struct { char token[40]; uint32_t key; UgControllerFamily family; } UgControllerBinding;
typedef struct {
    UgControllerFamily family;
    UgTexLib *lib;
    int available, icons[19];
    UgControllerBinding binding[32]; int nbindings;
    char source[512], status[160];
    int xbox_available, xbox_icons[47];
    char xbox_source[512];
    int folder_active, folder_index_failed; size_t folder_count;
    UgGlyphFolderRecord folder_records[66];
    char folder_source[512];
} UgController;
void ug_controller_init(UgController *p);
UgVisualStatus ug_controller_select(UgController *p, UgControllerFamily family);
/* Requires explicit demo configuration, never derives an action binding. */
UgVisualStatus ug_controller_bind(UgController *p, const char *token, uint32_t key);
UgVisualStatus ug_controller_token(const UgController *p, const char *token, int *tex);
const char *ug_controller_icon_name(int index);
uint32_t ug_controller_icon_key(int index);
/* Gallery catalog sizes; texture handles are 1-based, 0 means unavailable.
 * The legacy icon APIs above continue to describe the PS2 catalog. */
int ug_controller_family_icon_count(UgControllerFamily family);
const char *ug_controller_family_icon_name(UgControllerFamily family, int index);
uint32_t ug_controller_family_icon_key(UgControllerFamily family, int index);
int ug_controller_family_icon_texture(const UgController *p, UgControllerFamily family, int index);
int ug_controller_family_available(const UgController *p, UgControllerFamily family);
int ug_controller_load_xbox(UgController *p, UgTexLib *lib, const char *iso_path);
int ug_controller_load_xbox_pack(UgController *p, UgTexLib *lib, const uint8_t *pack, size_t len);
/* Narrow BC1/BC2 primitive permits small synthetic blocks; loader requires 32x32. */
int ug_xbox_icon_decode(int fmt, int w, int h, const uint8_t *pixels, size_t len, uint8_t *rgba);
/* Synthetic XDVDFS source lookup, using the same traversal as the ISO loader. */
int ug_xdvdfs_find(const uint8_t *source, size_t len, const char *path, uint64_t *offset, uint32_t *size);
unsigned ug_ps2_icon_address(unsigned x, unsigned y);
int ug_ps2_icon_decode(const uint8_t *pixels, size_t plen, const uint8_t *palette, size_t clen, uint8_t rgba[4096]);
/* Runtime-only loader. ISO directory walk bounded by declared extents. */
int ug_controller_load_ps2(UgController *p, UgTexLib *lib, const char *iso_path);
/* Synthetic pack tests may call the same validation/decoder without an ISO. */
int ug_controller_load_pack(UgController *p, UgTexLib *lib, const uint8_t *pack, size_t len);
#endif
