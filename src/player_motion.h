#ifndef OPENUG2_PLAYER_MOTION_H
#define OPENUG2_PLAYER_MOTION_H

typedef struct { int awaiting_drive; } PlayerMotion;

/* Spawn and non-driving screens hold the player; releasing the pedal after
   starting to drive must retain ordinary coasting. */
static inline int player_motion_tick(PlayerMotion *s,int active,int placed,int requested) {
    if(placed || !active)s->awaiting_drive=1;
    if(active && requested)s->awaiting_drive=0;
    return active && !s->awaiting_drive;
}
#endif
