/* Gameplay adapter for approved Stage 1. Chassis/contact/brakes remain
 * OpenUG2 approximations; see docs/DRIVETRAIN.md. No SDL, GL or file IO. */
#ifndef OPENUG2_DRIVE_STAGE1_H
#define OPENUG2_DRIVE_STAGE1_H
#include "drivetrain.h"
#include "physics.h"
typedef struct {
    Drivetrain drivetrain;
    int ready,reverse;
    float wheel_travel; /* mean signed angular travel during the last host tick */
} Stage1Drive;
/* Read only approved stock S1-1 fields via independent GLOBALB framing. */
int stage1_car_read(const unsigned char *data,long length,const char *name,DrvCar *out);
int stage1_drive_init(Stage1Drive *state,const DrvCar *car);
/* Spawn/reset: retain car tuning, discard transient shaft/clutch state and
 * seed wheel rolling speed from host velocity (metres per 60 Hz tick). */
void stage1_drive_reset(Stage1Drive *state,const float velocity[2],float heading);
/* One host 60 Hz tick, subdivided per S1-2.2. Pedal: W=+1, S=-1.
 * S brakes forward then selects reverse at rest; W brakes reverse then auto.
 * Brake/handbrake, contact loads, drag and chassis steering are adapters. */
float stage1_drive_step(Stage1Drive *state,float position[3],float velocity[2],
    float *heading,float *speed,float pedal,float steer,int handbrake,
    const PhysSurface *surface,const PhysVehicle *vehicle,const PhysRideState *ride);
float stage1_drive_rpm(const Stage1Drive *state);
int stage1_drive_gear(const Stage1Drive *state); /* HUD: R=-1,N=0,forward=1..6 */
#endif
