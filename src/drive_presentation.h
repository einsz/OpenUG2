/* Independent host display policy RS-0..5. Every value is output-only. */
#ifndef OPENUG2_DRIVE_PRESENTATION_H
#define OPENUG2_DRIVE_PRESENTATION_H
#include <math.h>
#include <string.h>
#include "drive_stage1.h"
typedef struct {
    int valid;
    float origin[3],hub[4][3],travel[4],steer[4];
    double phase[4];
    ResBasis axes;
} DrivePresentation;
typedef struct { DrivePresentation before,now; } DrivePresentationHistory;
static DrivePresentation drive_presentation_capture(const Stage1Drive *s) {
    DrivePresentation p={.valid=s->dynamic_initialized,.axes=s->motion.axes};
    const float *c=s->contacts.car.centre;
    for(int k=0;k<3;k++)p.origin[k]=(float)(s->motion.position[k]-(double)p.axes.forward[k]*c[0]-(double)p.axes.lateral[k]*c[1]-(double)p.axes.up[k]*c[2]);
    for(int i=0;i<4;i++) {
        memcpy(p.hub[i],s->contacts.car.mount[i],sizeof p.hub[i]);
        p.travel[i]=s->contacts.wheel[i].travel;
        p.hub[i][2]+=p.travel[i];p.steer[i]=s->wheel_angle[i];
        p.phase[i]=s->drivetrain.wheel[i].phi;
    }
    return p;
}
/* Unit quaternion (scalar first), derived only from a copied display basis. */
static void drive_presentation_quaternion(const ResBasis *b,double q[4]) {
    double m[3][3];const float *v[3]={b->forward,b->lateral,b->up};
    for(int r=0;r<3;r++)for(int c=0;c<3;c++)m[r][c]=v[c][r];
    double trace=m[0][0]+m[1][1]+m[2][2];
    if(trace>0){double z=2*sqrt(1+trace);q[0]=z/4;q[1]=(m[2][1]-m[1][2])/z;q[2]=(m[0][2]-m[2][0])/z;q[3]=(m[1][0]-m[0][1])/z;}
    else {int i=0;if(m[1][1]>m[i][i])i=1;if(m[2][2]>m[i][i])i=2;int j=(i+1)%3,k=(i+2)%3;double z=2*sqrt(1+m[i][i]-m[j][j]-m[k][k]);q[0]=(m[k][j]-m[j][k])/z;q[i+1]=z/4;q[j+1]=(m[j][i]+m[i][j])/z;q[k+1]=(m[k][i]+m[i][k])/z;}
    double length=0;for(int k=0;k<4;k++)length+=q[k]*q[k];length=sqrt(length);for(int k=0;k<4;k++)q[k]/=length;
}
static ResBasis drive_presentation_basis(const ResBasis *a,const ResBasis *b,float t) {
    if(!memcmp(a,b,sizeof *a))return *a;
    double x[4],y[4],q[4],dot=0;drive_presentation_quaternion(a,x);drive_presentation_quaternion(b,y);
    for(int k=0;k<4;k++)dot+=x[k]*y[k];
    if(dot<0){dot=-dot;for(int k=0;k<4;k++)y[k]=-y[k];}
    double u=1-t,v=t;
    if(dot<.9999999){double angle=acos(fmin(1,dot)),den=sin(angle);u=sin((1-t)*angle)/den;v=sin(t*angle)/den;}
    double length=0;for(int k=0;k<4;k++){q[k]=u*x[k]+v*y[k];length+=q[k]*q[k];}length=sqrt(length);for(int k=0;k<4;k++)q[k]/=length;
    double w=q[0],xx=q[1],yy=q[2],z=q[3];
    return (ResBasis){{1-2*(yy*yy+z*z),2*(xx*yy+w*z),2*(xx*z-w*yy)},
        {2*(xx*yy-w*z),1-2*(xx*xx+z*z),2*(yy*z+w*xx)},
        {2*(xx*z+w*yy),2*(yy*z-w*xx),1-2*(xx*xx+yy*yy)}};
}
static DrivePresentation drive_presentation_mix(const DrivePresentation *a,const DrivePresentation *b,float t) {
    if(t>=1 || !a->valid)return *b;
    if(t<=0)return *a;
    DrivePresentation p=*b;p.axes=drive_presentation_basis(&a->axes,&b->axes,t);
    for(int k=0;k<3;k++)p.origin[k]=a->origin[k]+t*(b->origin[k]-a->origin[k]);
    for(int i=0;i<4;i++) {
        for(int k=0;k<3;k++)p.hub[i][k]=a->hub[i][k]+t*(b->hub[i][k]-a->hub[i][k]);
        p.travel[i]=a->travel[i]+t*(b->travel[i]-a->travel[i]);p.steer[i]=a->steer[i]+t*(b->steer[i]-a->steer[i]);
        p.phase[i]=a->phase[i]+t*(b->phase[i]-a->phase[i]);
    }
    return p;
}
static void drive_presentation_reset(DrivePresentationHistory *h,const DrivePresentation *p){h->before=h->now=*p;}
static DrivePresentation drive_presentation_sample(const DrivePresentationHistory *h,float alpha,int snap){return snap?h->now:drive_presentation_mix(&h->before,&h->now,alpha);}
/* Mount identity, rather than index parity, establishes the outward face. */
static float drive_presentation_side(const DrivePresentation *p,int wheel){return p->hub[wheel][1]<0?-1:1;}
static void drive_presentation_point(const DrivePresentation *p,const float local[3],float world[3]){
    for(int k=0;k<3;k++)world[k]=(float)(p->origin[k]+(double)p->axes.forward[k]*local[0]+(double)p->axes.lateral[k]*local[1]+(double)p->axes.up[k]*local[2]);
}
static void drive_presentation_model(const DrivePresentation *p,float height,float m[16]) {
    const float *axes[3]={p->axes.forward,p->axes.lateral,p->axes.up};memset(m,0,16*sizeof(float));
    for(int a=0;a<3;a++)for(int k=0;k<3;k++)m[4*a+k]=axes[a][k];
    for(int k=0;k<3;k++)m[12+k]=p->origin[k]+axes[2][k]*height;
    m[15]=1;
}
#endif
