/* Original-resource token rendering, SPEC 20.3. No inferred retail bindings. */
#include "ugui_canvas.h"
#include "ugui_controller.h"
#include <string.h>
static UgController *profile;
void ug_glyph_set_controller(UgController *p){profile=p;}
const UgController *ug_glyph_controller(void){return profile;}
const char *ug_glyph_name(UgGlyphId id){return id==UG_G_NONE?"unbound":"original-resource";}
int ug_glyph_for_token(const char *name,UgGlyphMode mode,UgGlyph *out){
    (void)mode; out->id=UG_G_NONE;out->ch=0;
    if(!strcmp(name,"ICON_SPACER")){out->id=UG_G_SPACER;return 1;}
    int tex=0;
    if(profile && ug_controller_token(profile,name,&tex)==UG_VIS_OK)out->id=(UgGlyphId)(UG_G_COUNT+tex);
    /* All controller markup is consumed; unresolved tokens emit no fabricated character. */
    return 1;
}
void ug_glyph_draw(UgCanvas *cv,UgGlyph g,float x,float y,float size,UgColor color){
    int tex=(int)g.id-UG_G_COUNT;
    if(tex>0)ug_cv_image_rect(cv,UG_RECT_C(x,y,size,size),tex,color);
}
