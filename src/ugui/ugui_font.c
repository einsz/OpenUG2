/* Amendment 4: generated font rasterizer removed. Original font atlases only. */
#include "ugui_canvas.h"
const UgFont *ug_font(void) { static const UgFont empty={0}; return &empty; }
const UgGlyphInfo *ug_font_glyph(const UgFont *f,uint32_t cp){(void)f;(void)cp;return NULL;}
float ug_font_adv(const UgFont *f,uint32_t cp){(void)f;(void)cp;return 0;}
