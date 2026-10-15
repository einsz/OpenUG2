#include <math.h>
#include <string.h>
#include "resistance.h"

double res_dot(const float a[3],const float b[3]) {
    return (double)a[0]*b[0]+(double)a[1]*b[1]+(double)a[2]*b[2];
}
static double length(const float v[3]) { return sqrt(res_dot(v,v)); }
ResBasis res_basis_level(float heading) {
    float c=cosf(heading),s=sinf(heading);
    ResBasis b={{c,s,0},{-s,c,0},{0,0,1}};return b;
}
void res_frame_inputs(ResInput *in,int state,int alternate,int disable,
    unsigned mask,const float surface[4]) {
    int n=0;double sum=0;
    for(int k=0;k<4;k++)if(mask&(1u<<k)){++n;sum+=surface[k];}
    in->contacts=n;
    if(state==0||state==2) {
        in->enable=alternate?0:1;in->surface=n?(float)(sum/n):1;
        in->stabilize=disable?0:1;
    } else if(state==1) { in->enable=0;in->surface=1;in->stabilize=0; }
}
void res_drag(const ResCar *car,const ResInput *in,const float v[3],float f[3]) {
    double factor=-(double)car->drag*in->surface*(1-(double)in->reduction)*in->enable
        *(2-(double)in->throttle)*length(v)/in->scale;
    for(int k=0;k<3;k++)f[k]=(float)(factor*v[k]);
}
static void point_force(ResBody *b,const float raw[3],const float arm[3],float applied[3]) {
    double mag=length(raw),scale=mag>1200?1200/mag:1;
    double f[3],moment[3];
    for(int k=0;k<3;k++) { f[k]=raw[k]*scale;applied[k]=(float)f[k];b->force[k]=(float)(b->force[k]+f[k]); }
    moment[0]=arm[1]*f[2]-arm[2]*f[1];moment[1]=arm[2]*f[0]-arm[0]*f[2];moment[2]=arm[0]*f[1]-arm[1]*f[0];
    mag=sqrt(moment[0]*moment[0]+moment[1]*moment[1]+moment[2]*moment[2]);scale=mag>100?100/mag:1;
    for(int k=0;k<3;k++)b->moment[k]=(float)(b->moment[k]+moment[k]*scale);
}
void res_prepare_aero(ResBody *b,const ResCar *car,const ResInput *in,const ResBasis *basis,ResAero *out) {
    ResAero result={0};float arm[3]={0,0,0},applied[3];
    res_drag(car,in,b->velocity,result.drag);
    if(in->contacts>=2)arm[2]=-.1f*(1-in->throttle);
    point_force(b,result.drag,arm,applied);
    double speed=length(b->velocity)/in->scale;
    double c=speed>(double).0001f?fmax(res_dot(basis->forward,b->velocity)/speed,0):fmax(basis->forward[0],0);
    double orientation=in->alternate?c*c*c*c:fmax(c*c,(double).4f);
    double upright=in->contacts<2?fmax(basis->up[2],0):1;
    double wf,wr;
    if(in->contacts==0) {
        double total=((double)car->front+car->rear)*orientation*upright*speed;
        wf=(double).4f*total;wr=(double).41f*total;
    } else { wf=car->front*orientation*upright*speed;wr=car->rear*orientation*upright*speed; }
    result.front_load=(float)wf;result.rear_load=(float)wr;result.load=(float)(wf+wr);
    float force[3];
    for(int k=0;k<3;k++){force[k]=(float)(-in->enable*wf*basis->up[k]);arm[k]=basis->forward[k];}
    point_force(b,force,arm,result.front);
    for(int k=0;k<3;k++){force[k]=(float)(-in->enable*wr*basis->up[k]);arm[k]=-basis->forward[k];}
    point_force(b,force,arm,result.rear);
    if(out)*out=result;
}
static double threshold(const ResCar *car,const ResInput *in,double b,double *slope) {
    double k=fmin(fabs(b),(double).3f);
    double h=b>0?1-(1-(double).9f)/(double).3f*k:1+((double)1.1f-1)/(double).3f*k;
    *slope=k;return ((double)in->scale*car->limit_mph+(double)1.9f)*(in->bonus?(double)1.03f:1)*h;
}
void res_limit_local(const ResCar *car,const ResInput *in,float speed,float bz,float f[3],ResLimit *out) {
    double slope,t=threshold(car,in,bz,&slope),mph=(double)speed*(double)2.236024856567383f;
    int active=car->limit_mph!=0&&mph>t&&f[0]>0;
    if(active)f[0]=(float)fmin(-5*slope,-.5);
    if(out){out->threshold_mph=(float)t;out->measured_mph=(float)mph;out->activated=active;}
}
double res_stabilization_curve(double x) {
    static const float value[8]={1.5f,1,.6f,.22f,.06f,.03f,.015f,0};
    if(x<=0)return value[0];
    if(x>=1)return value[7];
    double p=x*7;int i=(int)p;double fraction=p-i;
    return (1-fraction)*value[i]+fraction*value[i+1];
}
void res_derivative(ResBody *b,const ResCar *car,const ResInput *in,const ResBasis *basis,float w,ResDerivative *out) {
    double slope,t=threshold(car,in,basis->forward[2],&slope);
    double forward=res_dot(basis->forward,b->force);
    double mph=hypot(b->velocity[0],b->velocity[1])*(double)2.236024856567383f;
    if(car->limit_mph!=0&&mph>t&&forward>0) {
        double delta=fmin(-5*slope,-.5)-forward;
        for(int k=0;k<3;k++)b->force[k]=(float)(b->force[k]+delta*basis->forward[k]);
    }
    double curve=res_stabilization_curve(fmax(length(b->velocity)/4,(double)w/2));
    out->curve=(float)curve;out->zdot=curve>b->z?.1f:-10;
    for(int k=0;k<3;k++)out->momentum[k]=(float)(b->force[k]-(in->stabilize?8*(double)b->z*b->momentum[k]:0));
}
void res_integrate(ResBody *b,const ResDerivative *d,float h) {
    for(int k=0;k<3;k++)b->momentum[k]=(float)(b->momentum[k]+(double)h*d->momentum[k]);
    b->z=(float)(b->z+(double)h*d->zdot);
}
int res_post(ResBody *b,int gravity) {
    for(int k=0;k<3;k++)b->velocity[k]=(float)((double)b->momentum[k]/b->mass);
    double speed=length(b->velocity);int active=speed>200&&(double)b->mass>.7;
    if(active) {
        float divisor=(float)speed;
        for(int k=0;k<3;k++) {
            b->momentum[k]=(float)(b->momentum[k]*(200/(double)divisor));
            b->velocity[k]=(float)((double)b->momentum[k]/b->mass);
        }
    }
    if(b->z<0)b->z=0;
    memcpy(b->saved_force,b->force,sizeof b->force);memcpy(b->saved_moment,b->moment,sizeof b->moment);
    memset(b->force,0,sizeof b->force);memset(b->moment,0,sizeof b->moment);
    if(gravity)b->force[2]=(float)(-9.8*b->mass);
    return active;
}
