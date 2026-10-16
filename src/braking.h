/* Independent Stage-3 service-brake maths. Supplied finite inputs; no IO. */
#ifndef OPENUG2_BRAKING_H
#define OPENUG2_BRAKING_H
#ifdef __cplusplus
extern "C" {
#endif
#define BRK_MPH 2.236024856567383f
/* Effective stock strengths in kN m, rear share dimensionless (S3-1). */
typedef struct { float service,handbrake,rear_share; } BrkCar;
typedef struct { float throttle,service,handbrake; } BrkPedals;
typedef struct { float coefficient[4],service[4]; } BrkFrame;
typedef struct {
    float radius,omega,vx,vy,load,service,handbrake,tau;
    double mu; /* combined pre-decision coefficient; no extra state store */
    int locked,relief_disabled;
} BrkInput;
typedef struct {
    float omega,tau,slip;
    int locked,decision;
    double relief_scale,q,a,c;
} BrkResult;
void brk_pedals(BrkPedals *pedals,int exclusivity);
/* mph is the once-per-frame full-speed snapshot, not a signed forward speed. */
void brk_distribute(const BrkCar *car,const BrkPedals *pedals,double mph,BrkFrame *frame);
/* S3-8: eta is supplied by the later stage; this is only its unit conversion. */
void brk_handbrake(const BrkCar *car,const float eta[4],float magnitude[4]);
void brk_compare(double a,double c,float *omega,int *locked);
BrkResult brk_prepare(const BrkInput *input);
/* Direction values: -1 backward, 0 stopped, +1 forward. */
void brk_direction_post(float *omega,int *direction);
#ifdef __cplusplus
}
#endif
#endif
