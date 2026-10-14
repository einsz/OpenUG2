/* Approved CONTROLLER_GLYPH_FOLDER identification boundary. Runtime artwork
 * stays external; this interface carries only sanitized lookup metadata. */
#ifndef UGUI_GLYPH_FOLDER_H
#define UGUI_GLYPH_FOLDER_H
#include "ugui_controller.h"
typedef struct {
    UgControllerFamily family;
    uint32_t key;
    char category[12], language[16], file[160];
    unsigned width, height;
    char png_sha256[65], rgba_sha256[65];
} UgGlyphFolderEntry;
const UgGlyphFolderEntry *ug_glyph_folder_index(size_t *count);
const char *ug_glyph_folder_status(UgGlyphFolderStatus status);
int ug_controller_load_folder(UgController *p,UgTexLib *lib,const char *root);
/* Same boundary with explicit metadata for independent synthetic test vectors.
 * Production CLI always supplies the built-in reviewer-approved index. */
int ug_controller_load_folder_index(UgController *p,UgTexLib *lib,const char *root,
                                   const UgGlyphFolderEntry *entries,size_t count);
#endif
