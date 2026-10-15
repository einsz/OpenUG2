#ifndef OPENUG2_HUD_RACE_H
#define OPENUG2_HUD_RACE_H
#include "hud.h"
#include "hud_asset.h"

enum { HUD_RACE_NONE, HUD_RACE_COUNTDOWN, HUD_RACE_RUNNING, HUD_RACE_FINISHED, HUD_RACE_FAILED };
enum { HR_POSITION, HR_ORDINAL, HR_CARS, HR_LAP_LABEL, HR_LAP, HR_LAPS,
       HR_TIME_LABEL, HR_TIME, HR_BANNER, HR_DRIFT_CHAIN, HR_DRIFT_TOTAL,
       HR_DRIFT_TOTAL_LABEL, HR_DRIFT_ANGLE, HR_FIELD_COUNT };
typedef struct { int event, phase, active; double elapsed; } HudRaceClock;
const char *hud_race_package(int kind);
int hud_race_package_read(const unsigned char *data, size_t len,
                          const char *name, HaPackage *out);
void hud_race_clock_step(HudRaceClock *clock, int event, int phase, double dt);
int hud_race_time(double seconds, char out[64]);
int hud_race_text(const HudState *state, int field, char out[64]);
uint32_t hud_race_field_key(int kind, int field);

/* Project a world point into the displayed authored map (logical pixels). */
int hud_race_map_point(const HaMapFrame *frame,double x,double y,double out[2]);

float hud_race_drag_angle(const HaNode *needle,double rpm);

#endif
