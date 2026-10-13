/* Host adapter for the pause menu. Game state and audio stay with the host. */
#ifndef OPENUG2_PAUSE_MENU_H
#define OPENUG2_PAUSE_MENU_H
#include <SDL.h>
#include "frontend/frontend.h"
typedef struct PauseMenu PauseMenu;
PauseMenu *pause_menu_create(const char *dataroot);
void pause_menu_destroy(PauseMenu *p);
void pause_menu_open(PauseMenu *p);
int pause_menu_is_open(const PauseMenu *p);
void pause_menu_event(PauseMenu *p, const SDL_Event *e, SDL_Window *win);
/* Window-point input; dimensions are logical window units, as in SDL events. */
void pause_menu_pointer(PauseMenu *p, float x, float y, int width, int height);
const char *pause_menu_caption(const PauseMenu *p);
/* Resources may be unavailable while state/input remains usable. */
int pause_menu_visual_available(const PauseMenu *p);
const char *pause_menu_status(const PauseMenu *p);
FeAction pause_menu_update(PauseMenu *p, float seconds);
void pause_menu_draw(PauseMenu *p, int width, int height);
/* Deterministic verification-only UI replay and independent populated authored reference. */
int pause_menu_capture_replay(PauseMenu *p,const char *events,float elapsed_ms,int width,int height);
int pause_menu_draw_reference(PauseMenu *p,int width,int height,float elapsed_ms,const char *events);
#endif
