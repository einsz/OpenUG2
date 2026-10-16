/* Independent Stage-2 body resistance. Tonnes, kN, metres, seconds.
 * Finite valid inputs and an orthonormal basis are required (SPEC S2-0).
 * No implicit gameplay defaults: callers supply reduction, mode and surfaces. */
#ifndef OPENUG2_RESISTANCE_H
#define OPENUG2_RESISTANCE_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { float drag,front,rear,limit_mph; } ResCar;
typedef struct {
    float scale,reduction,enable,surface,throttle;
    int contacts,bonus,alternate,stabilize;
} ResInput;
typedef struct { float forward[3],lateral[3],up[3]; } ResBasis;
typedef struct {
    float mass,momentum[3],velocity[3],force[3],moment[3],z;
    float saved_force[3],saved_moment[3];
} ResBody;
typedef struct {
    double drag[3];
    float front[3],rear[3]; /* drag is raw; front/rear are capped applied forces */
    double front_load,rear_load,load; /* before enable and point-force caps */
} ResAero;
typedef struct { float threshold_mph,measured_mph;int activated; } ResLimit;
typedef struct { float momentum[3],zdot,curve; } ResDerivative;
ResBasis res_basis_level(float heading);
double res_dot(const float a[3],const float b[3]);
/* Only established S2-1 mode mappings; other codes retain mode scales.
 * Contact count always comes from this same four-wheel snapshot. */
void res_frame_inputs(ResInput *in,int state_code,int alternate_movement,
    int disable_stabilize,unsigned contact_mask,const float body_surface[4]);
void res_drag(const ResCar *car,const ResInput *in,const float velocity[3],double force[3]);
/* BR-6 per-application caps; moment uses requested force before linear cap. */
void res_point_force(ResBody *body,const float force[3],const float arm[3],float applied[3]);
/* Add the three aerodynamic point forces once, before wheel coupling. */
void res_prepare_aero(ResBody *body,const ResCar *car,const ResInput *in,
    const ResBasis *basis,ResAero *out);
/* Local-force helper and full world-accumulator derivative. Limiter mutates F. */
void res_limit_local(const ResCar *car,const ResInput *in,float horizontal_speed,
    float forward_z,float force_local[3],ResLimit *out);
double res_stabilization_curve(double x);
void res_derivative(ResBody *body,const ResCar *car,const ResInput *in,
    const ResBasis *basis,float angular_speed,ResDerivative *out);
/* Collect all other blocks' derivatives before integrating them. */
void res_integrate(ResBody *body,const ResDerivative *derivative,float h);
/* Cap momentum, clamp z, save/clear accumulators, optionally seed gravity.
 * Does not prepare aero/coupling; repeat post sweeps perform no Euler step. */
int res_post(ResBody *body,int gravity);
#ifdef __cplusplus
}
#endif
#endif
