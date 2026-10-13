/* Independently authored geometry, pause SPEC 2. Top-left virtual coordinates. */
#ifndef OPENUG2_PAUSE_MENU_LAYOUT_H
#define OPENUG2_PAUSE_MENU_LAYOUT_H
#include "ugui/ugui_canvas.h"
typedef struct { double scale, ox, oy; } PauseTransform;
static inline PauseTransform pause_menu_transform(int w, int h) {
    double s=w/640.0<h/480.0?w/640.0:h/480.0;
    return (PauseTransform){s,(w-640*s)/2,(h-480*s)/2};
}
static inline void pause_menu_to_virtual(PauseTransform t,double px,double py,float *x,float *y) {
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
/* Current supported catalogue is centred at the selected action. Unobserved
 * left-side positions mirror the independently specified right-side slot. */
static inline UgRect pause_menu_item_bounds(int offset) {
    if(!offset)return pause_menu_bounds(PAUSE_FRAME);
    return (UgRect){446.2f+(offset-1)*158.4f,196.0f,65.8f,62.7f};
}
/* Pause corrections: visible alpha/ink regions, distinct from full quads. */
static inline UgRect pause_menu_icon_ink(int offset) {
    if(!offset)return (UgRect){293.8f,198.7f,53.8f,54.6f};
    return (UgRect){446.2f+(offset-1)*158.4f,211.6f,65.8f,31.1f};
}
static inline UgRect pause_menu_label_ink(int accept) {
    return accept?(UgRect){520.9f,296,40.9f,8.9f}:(UgRect){94.2f,296,21.4f,8.9f};
}
static inline UgRect pause_menu_marker_ink(void) {
    return (UgRect){45.3f,184.4f,9.8f,8.5f};
}
#endif
