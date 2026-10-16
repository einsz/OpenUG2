/* Independent Stage4 ordinary, level-planar components. All producers are
 * supplied by callers; finite inputs and SPEC S4-0 domains are required. */
#ifndef OPENUG2_LATERAL_H
#define OPENUG2_LATERAL_H
#include <stdint.h>
typedef struct {
 float mount[4][3],angle[2],reference[2],factor[2],dimensions[3],gain;
} LatCar;
typedef struct {
 int analog,latch;
 float raw,vctl,history,rate_history,old_degrees,previous_limit,sigma,speed;
 float throttle,brake,handbrake,gain,dt;
} LatFilter;
typedef struct {
 uint16_t command;int latch;float mapped,limit,degrees,previous_limit;
} LatCommand;
float lat_curve(const char *name,float x);
LatCommand lat_filter(const LatFilter *input);
float lat_decode(uint16_t command);
void lat_geometry(uint16_t command,float wheelbase,float track,float angles[4]);
uint16_t lat_heading(float x,float y);
float lat_slip(float vx,float vy);
float lat_body_slip(float signed_forward,const float body_forward[2],const float world_velocity[2]);
float lat_grip(float load,float angle_scale,float reference,float factor,float alpha,float vx,float multiplier);
int lat_sliding(float slip,float alpha,float angle_scale,int locked,float vx,float vy);
void lat_cap(float *fx,float *fy,float limit,float torque,int locked,float vx,float alpha);
void lat_contact(const float velocity[2],float yaw_rate,const float arm[2],float angle,float centre_velocity[2],float *vx,float *vy);
/* Force, arm and angle use a common body basis. No normal force is added. */
void lat_accumulate(float force[2],float *moment,const float arm[2],float angle,float fx,float fy);
float lat_inertia(float mass,const float dimensions[3]);
#endif
