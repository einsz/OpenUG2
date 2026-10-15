/* Optional observation of the independent host. Values use physics SI units;
 * this writer never changes simulation state or owns the output stream. */
#ifndef OPENUG2_DRIVE_TRACE_H
#define OPENUG2_DRIVE_TRACE_H
#include <stdio.h>
#include "drive_stage1.h"

typedef struct {
    unsigned long long tick;
    int active, race_state;
    float pedal,steer,handbrake;
    float position[3],velocity[2],heading;
    float prepared_force[3]; /* incoming force, before this tick's derivatives */
    float ground_fraction,vehicle_contact;
    int walls,rails;
    const PhysRideState *ride;
} DriveTraceSample;

static int drive_trace_header(FILE *f) {
    fputs("tick,active,race_state,pedal,steer,handbrake,x,y,z,heading,host_vx,host_vy,body_vx,body_vy,body_vz,exported_forward,exported_speed,rpm,gear,target_gear,clutch,engagement,shaft_omega,engine_torque,feedback,throttle,service_brake,shaped_handbrake,reverse,contact_mask,ride_mask,ride_vz,pitch,roll,pitch_rate,roll_rate,incoming_fx,incoming_fy,incoming_fz,prepared_fx,prepared_fy,prepared_fz,prepared_mx,prepared_my,prepared_mz,ground_fraction,walls,rails,vehicle_contact",f);
    for(int i=0;i<4;i++) {
        const char *fields[]={"contact","load","travel","travel_rate","surface",
            "locked","omega","direction","torque","smooth_torque",
            "vx","vy","slip","gripping","long_force","lat_force","service_torque",
            "handbrake_torque","world_fx","world_fy","world_fz"};
        for(unsigned k=0;k<sizeof fields/sizeof *fields;k++)fprintf(f,",wheel%d_%s",i,fields[k]);
    }
    fputc('\n',f);return !ferror(f);
}
static int drive_trace_row(FILE *f,const Stage1Drive *s,const DriveTraceSample *q) {
    const Drivetrain *d=&s->drivetrain;const CtState *c=&s->contacts;
    const PhysRideState zero={0},*r=q->ride?q->ride:&zero;
    fprintf(f,"%llu,%d,%d",q->tick,q->active,q->race_state);
#define VALUE(v) fprintf(f,",%.9g",(double)(v))
    VALUE(q->pedal);VALUE(q->steer);VALUE(q->handbrake);
    for(int k=0;k<3;k++){VALUE(q->position[k]);}
    VALUE(q->heading);
    for(int k=0;k<2;k++)VALUE(q->velocity[k]*PHYS_TICKRATE);
    for(int k=0;k<3;k++)VALUE(s->body.velocity[k]);
    VALUE(c->exported_forward);VALUE(c->exported_speed);VALUE(d->omega_e*DRV_KR);
    VALUE(d->gear-1);VALUE(d->target_gear-1);VALUE(d->mode);VALUE(d->engage);
    VALUE(d->omega_w);VALUE(d->engine_torque);VALUE(d->feedback);
    VALUE(d->throttle);VALUE(d->brake);VALUE(d->handbrake);VALUE(s->reverse);
    VALUE(c->exported_mask);VALUE(r->contact_mask);VALUE(r->vz);
    VALUE(r->pitch);VALUE(r->roll);VALUE(r->pitch_rate);VALUE(r->roll_rate);
    for(int k=0;k<3;k++)VALUE(q->prepared_force[k]);
    for(int k=0;k<3;k++)VALUE(s->body.force[k]);
    for(int k=0;k<3;k++)VALUE(s->body.moment[k]);
    VALUE(q->ground_fraction);VALUE(q->walls);VALUE(q->rails);VALUE(q->vehicle_contact);
    for(int i=0;i<4;i++) {
        const CtWheel *w=c->wheel+i;const DrvWheel *dw=d->wheel+i;
        const DrvWheelEnv *e=d->env.wheel+i;
        VALUE(w->contact);VALUE(w->load);VALUE(w->travel);VALUE(w->rate);VALUE(w->surface);
        VALUE(d->brakes.locked[i]);VALUE(dw->omega);VALUE(dw->dir);
        VALUE(dw->tau);VALUE(dw->tau_smooth);
        VALUE(e->vx);VALUE(e->vy);VALUE(dw->slip_speed);VALUE(dw->gripping);
        VALUE(d->force_long[i]);VALUE(d->force_lat[i]);
        VALUE(d->brakes.frame.service[i]);VALUE(d->brakes.handbrake[i]);
        for(int k=0;k<3;k++)VALUE(s->contact_force[i][k]);
    }
#undef VALUE
    fputc('\n',f);return !ferror(f);
}
#endif
