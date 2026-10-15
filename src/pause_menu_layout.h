/* Independently authored geometry, pause SPEC 2. Top-left virtual coordinates.
 * The carousel itself (slot layout, scroll, arrows, tab) is gui_carousel.h; this file keeps the window
 * transform and the regions of the PC Back / Continue pills. */
#ifndef OPENUG2_PAUSE_MENU_LAYOUT_H
#define OPENUG2_PAUSE_MENU_LAYOUT_H
#include "ugui/ugui_canvas.h"
#include <math.h>
typedef struct { double scale, ox, oy; } PauseTransform;
static inline PauseTransform pause_menu_transform(int w, int h) {
    double s=w/640.0<h/480.0?w/640.0:h/480.0;
    return (PauseTransform){s,(w-640*s)/2,(h-480*s)/2};
}
static inline void pause_menu_to_virtual(PauseTransform t, double px, double py, float *x, float *y) {
    *x=(float)((px-t.ox)/t.scale);*y=(float)((py-t.oy)/t.scale);
}
enum { PAUSE_PANEL, PAUSE_FRAME, PAUSE_BACK, PAUSE_CONTINUE };
static inline UgRect pause_menu_bounds(int region) {
    static const UgRect r[]={
        {24.4f,152.0f,598.3f,170.2f}, {257.3f,195.6f,127.1f,61.7f},
        {30.7f,291.1f,146.6f,21.8f}, {466.7f,291.1f,146.6f,21.8f}
    };
    return region>=0 && region<4?r[region]:(UgRect){0,0,0,0};
}
/* Visible ink region of a pill label (pause SPEC corrections). */
static inline UgRect pause_menu_label_ink(int accept) {
    return accept?(UgRect){520.9f,296,40.9f,8.9f}:(UgRect){94.2f,296,21.4f,8.9f};
}
#endif
