/* Gameplay adapters for approved Stages 1–5. Stage5 keeps world selection and
 * the prescribed ride trajectory explicit. See docs/DRIVETRAIN.md. No IO. */
#ifndef OPENUG2_DRIVE_STAGE1_H
#define OPENUG2_DRIVE_STAGE1_H
#include "drivetrain.h"
#include "resistance.h"
#include "lateral.h"
#include "contact.h"
#include "physics.h"
typedef struct {
    Drivetrain drivetrain;
    int contact_ready;CtState contacts;CtQuery contact_query;void *contact_user;
    CtPose contact_pose;float contact_force[4][3],contact_moment[4][3];
    float contact_event,contact_v0,contact_s0; /* explicit U5-3 supplied host channels */
    int ready,reverse,prepared;
    ResCar resistance_car;
    ResInput resistance_input;
    ResBody body;
    ResAero aero;
    float prepared_wheel[4][2],prepared_moment[4];
    int lateral_ready;
    LatCar lateral_car;
    LatCommand steering;
    float wheel_angle[4],exported_forward,yaw_rate,yaw_momentum,inverse_yaw_inertia;
    /* Explicit host choices for U4-2/5: 60 Hz box histories; supplied angular damping. */
    float input_history[33],rate_history[9],previous_mapped,yaw_damping;
    unsigned input_cursor,rate_cursor;
    /* Supplied host channels; independent of existing arcade surface.drag. */
    float body_surface[4],rolling_surface[4];
    unsigned damage_mask;
    int state_code,alternate_movement,disable_stabilize;
    unsigned initial_lock_mask; /* explicit host reset fixture, not a retail default */
    float wheel_travel; /* mean signed angular travel during the last host tick */
} Stage1Drive;
/* Read only approved stock S1-1 fields via independent GLOBALB framing. */
int stage1_car_read(const unsigned char *data,long length,const char *name,DrvCar *out);
/* Read only approved S2-1 stock fields from the same independent framing. */
int stage2_car_read(const unsigned char *data,long length,const char *name,ResCar *out);
/* Read only approved S3-1 stock fields via the same framing. */
int stage3_car_read(const unsigned char *data,long length,const char *name,BrkCar *out);
int stage5_car_read(const unsigned char *data,long length,const char *name,CtCar *out);
int stage5_drive_contacts(Stage1Drive *state,const CtCar *car,CtQuery query,void *user);
void stage1_steering_frame(Stage1Drive *state,float raw,const ResBasis *basis);
float stage5_drive_step(Stage1Drive *state,float position[3],float velocity[2],float *heading,float *speed,float pedal,float steer,int handbrake,const PhysRideState *ride);
int stage4_car_read(const unsigned char *data,long length,const char *name,LatCar *out);
/* Configure stock geometry. Host supplies inertia inverse/damping explicitly.
 * Mounts are also contact arms in this level, zero-centre host approximation. */
int stage4_drive_lateral(Stage1Drive *state,const LatCar *car,float inverse_inertia,float angular_damping);
/* Stage3 configuration is mandatory for gameplay service brakes. Initial lock
 * mask and relief switch are explicit host inputs (U3-2/3), not retail defaults. */
int stage3_drive_brakes(Stage1Drive *state,const BrkCar *car,unsigned initial_lock_mask,int relief_disabled);
/* Initial reduction is mandatory (U2-1). This host supplies ordinary mode,
 * G=1, no bonus/alternate mode/damage and unit surface channels explicitly. */
int stage1_drive_init(Stage1Drive *state,const DrvCar *car,const ResCar *resistance,float reduction);
/* Spawn/reset: retain car tuning, discard transient shaft/clutch state and
 * seed wheel rolling speed from host velocity (metres per 60 Hz tick). */
void stage1_drive_reset(Stage1Drive *state,const float velocity[2],float heading);
/* One host 60 Hz tick, subdivided per S1-2.2. Pedal: W=+1, S=-1.
 * S brakes forward then selects reverse at rest; W brakes reverse then auto.
 * Stage5 configuration selects geometric loads and handbrake/grip; the original
 * lower-stage supplied-load adapter remains for its independent fixtures. */
float stage1_drive_step(Stage1Drive *state,float position[3],float velocity[2],
    float *heading,float *speed,float pedal,float steer,int handbrake,
    const PhysSurface *surface,const PhysVehicle *vehicle,const PhysRideState *ride);
float stage1_drive_rpm(const Stage1Drive *state);
int stage1_drive_gear(const Stage1Drive *state); /* HUD: R=-1,N=0,forward=1..6 */
#endif
