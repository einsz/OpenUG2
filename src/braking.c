/* Independent implementation of approved S3-2, S3-3, S3-4 and S3-6. */
#include "braking.h"
#include <math.h>

void brk_pedals(BrkPedals *p,int exclusivity) {
    if(!exclusivity)return;
    if(p->throttle>p->service && p->throttle>p->handbrake) {
        if(p->service<.5f)p->service=0;
        if(p->handbrake<.5f)p->handbrake=0;
    }else if(p->throttle<.5f)p->throttle=0;
}
void brk_distribute(const BrkCar *car,const BrkPedals *p,double mph,BrkFrame *f) {
    for(int i=0;i<4;i++) {
        double a=2*(i<2?1-(double)car->rear_share:(double)car->rear_share)*p->service;
        if(i>=2 && p->throttle>.8f && p->service>.5f && mph<10)
            a=mph*(double).05f;
        f->coefficient[i]=(float)a;
        f->service[i]=(float)(4*(double)car->service*f->coefficient[i]);
    }
}
void brk_handbrake(const BrkCar *car,const float eta[4],float magnitude[4]) {
    for(int i=0;i<4;i++)magnitude[i]=(float)(10*(double)car->handbrake*eta[i]);
}
void brk_compare(double a,double c,float *omega,int *locked) {
    *locked=0;
    if(a>c) { *omega=0;*locked=a>1; }
}
BrkResult brk_prepare(const BrkInput *i) {
    BrkResult r={.omega=i->omega,.tau=i->tau,.locked=i->locked,.relief_scale=1};
    double tau=i->tau;
    if(!i->locked) {
        double sign=i->omega>0?-1:1;
        tau=(float)(tau+sign*i->service);
        tau=(float)(tau+sign*i->handbrake);
    }
    const double s=(double)i->radius*i->omega-i->vx;
    r.slip=(float)s;
    if(s<-.5f && tau<0 && i->handbrake==0 && !i->relief_disabled) {
        r.relief_scale=(double).5f/-s;
        tau*=r.relief_scale;
    }
    r.tau=(float)tau;
    r.decision=hypot(s,i->vy)>0 && (i->vx!=0 || i->vy!=0);
    if(r.decision) {
        r.q=fabs((double)i->vx)*i->load*i->mu/hypot(i->vx,i->vy);
        r.a=(double)1.2f*((double)i->service+i->handbrake);
        r.c=(double)i->radius*r.q+(double).1f*fabs(i->omega);
        brk_compare(r.a,r.c,&r.omega,&r.locked);
    }
    return r;
}
void brk_direction_post(float *omega,int *direction) {
    if((*direction>0 && *omega<0)||(*direction<0 && *omega>0))*omega=0;
    *direction=*omega>.00001f?1:*omega>=-.00001f?0:-1;
}
