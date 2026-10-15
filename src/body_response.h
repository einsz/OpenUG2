/* Independent body-response contract (BR-0..6), SI tonnes/kN, world z up.
 * ResBody owns P,V,F,T,z; BodyMotion owns X,B,L,held Omega and selection.
 * Callers provide valid finite input and nondegenerate axes. */
#ifndef OPENUG2_BODY_RESPONSE_H
#define OPENUG2_BODY_RESPONSE_H
#include "resistance.h"
typedef struct { float inertia[3],yaw_scale,limit[2]; } BodyCar;
typedef struct {
 float position[3],angular_momentum[3],omega[3],force_response[3];
 ResBasis axes;int adjusted;
} BodyMotion;
typedef struct { int curve,stabilize;float torque_limit; } BodyMode;
typedef struct { float position[3],angular_momentum[3];ResBasis axes;ResDerivative linear; } BodyDerivative;
int body_selection(int state,unsigned selector);
void body_select(BodyMotion *motion,const BodyCar *car,int adjusted);
/* BC-1: selected local inverse yaw inertia for reduced planar coupling. */
float body_inverse_yaw(const BodyMotion *motion,const BodyCar *car);
/* OM-1: session_status is lifecycle, separate from event family. */
BodyMode body_mode(int state,int session_status,float throttle,int contacts,float forward_z);
float body_torque_limit(int role,const float limits[2],float speed,float sigma,float Lz);
float body_damping(float angular_speed);
void body_angular_derivative(const float L[3],const float T[3],float w,float z,const BodyMode *mode,float out[3]);
void body_initialize(BodyMotion *motion,ResBody *body,const BodyCar *car,const float velocity[3],const float omega[3]);
void body_publish_angular(BodyMotion *motion,const BodyCar *car);
void body_collect(BodyMotion *motion,ResBody *body,const BodyCar *car,const BodyMode *mode,const ResCar *aero,const ResInput *input,BodyDerivative *out);
void body_integrate(BodyMotion *motion,ResBody *body,const BodyDerivative *derivative,float h);
void body_post(BodyMotion *motion,ResBody *body,const BodyCar *car,float gravity);
void body_repair(ResBasis *axes);
void body_point_force(ResBody *body,const float force[3],const float arm[3]);
void body_impulse(BodyMotion *motion,ResBody *body,const float impulse[3],const float arm[3],float angular_scale);
void body_opposing(ResBody *body,const BodyMotion *motion,int state,unsigned selector);
#endif
