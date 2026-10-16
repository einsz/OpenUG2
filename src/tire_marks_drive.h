/* Adapter: per-wheel tire-mark inputs (TM-2) from the coupled Stage1-5 driving
 * state. Header-only so tire_marks.c stays independent of the drive modules.
 * Reads only values the frame has already published; changes none. */
#ifndef OPENUG2_TIRE_MARKS_DRIVE_H
#define OPENUG2_TIRE_MARKS_DRIVE_H
#include "drive_stage1.h"
#include "tire_marks.h"

/* TM-2 vehicle-level gate G. Effects are enabled for this host; the special
 * mover mode (aero disabled, alternate movement) and a suspended session do
 * not sample (TM-U2: the global game-state code range has no host producer). */
static inline int tm_drive_gate(const Stage1Drive *s) {
    return s->contact_ready && s->prepared && !s->alternate_movement &&
           s->session.phase != DRIVE_SESSION_SUSPENDED;
}

/* Fills the four wheels' TmWheelIn and the vehicle origin O = P - B*c.
 * width = tyre widths from the car record (tm_car_read); mask = per-wheel
 * suppression mask (TM-U3; 0 = none). */
static inline void tm_drive_inputs(const Stage1Drive *s, const float width[4], unsigned mask,
                                   TmWheelIn out[4], float O[3]) {
    const CtPose *p = &s->contact_pose;
    const float *c = s->contacts.car.centre;
    for (int k = 0; k < 3; k++)
        O[k] = (float)(p->position[k] - (double)p->axes.forward[k] * c[0]
                       - (double)p->axes.lateral[k] * c[1] - (double)p->axes.up[k] * c[2]);
    for (int i = 0; i < 4; i++) {
        const CtWheel *w = &s->contacts.wheel[i];
        ResBasis axes; ct_wheel_axes(&p->axes, s->wheel_angle[i], &axes);
        TmWheelIn *o = &out[i];
        o->contact = w->contact;
        o->masked = (mask >> i) & 1u;
        o->sliding = !s->drivetrain.wheel[i].gripping;
        o->u = s->drivetrain.wheel[i].utilisation;
        o->N = w->load;
        /* U = V + Omega x (B r), world frame (S4-4/S5-3) */
        o->U[0] = p->velocity[0] + p->omega[1] * w->arm[2] - p->omega[2] * w->arm[1];
        o->U[1] = p->velocity[1] + p->omega[2] * w->arm[0] - p->omega[0] * w->arm[2];
        o->U[2] = p->velocity[2] + p->omega[0] * w->arm[1] - p->omega[1] * w->arm[0];
        for (int k = 0; k < 3; k++) { o->C[k] = w->centre[k]; o->l[k] = axes.lateral[k]; }
        o->surface = w->surface;
        o->width = width[i];
    }
}
#endif
