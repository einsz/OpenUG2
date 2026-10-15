#include "body_response.h"
#include <math.h>
#include <string.h>
static double magnitude(const float v[3]){return sqrt(res_dot(v,v));}
static void cross(const float a[3],const float b[3],float out[3]){float v[3]={(float)((double)a[1]*b[2]-(double)a[2]*b[1]),(float)((double)a[2]*b[0]-(double)a[0]*b[2]),(float)((double)a[0]*b[1]-(double)a[1]*b[0])};memcpy(out,v,sizeof v);}
static void cap(float v[3],float limit){double length=magnitude(v);if(length>limit)for(int k=0;k<3;k++)v[k]=(float)(limit*v[k]/length);}
static void inertia_map(const ResBasis*b,const float diagonal[3],const float world[3],int inverse,float out[3]){
 const float *axis[3]={b->forward,b->lateral,b->up};double local[3];for(int k=0;k<3;k++)local[k]=res_dot(axis[k],world)*(inverse?1/(double)diagonal[k]:diagonal[k]);
 for(int k=0;k<3;k++)out[k]=(float)(axis[0][k]*local[0]+axis[1][k]*local[1]+axis[2][k]*local[2]);
}
int body_selection(int state,unsigned selector){return (state==0||state==2)&&selector!=0;}
void body_select(BodyMotion*m,const BodyCar*c,int selected){if(m->adjusted&&!selected)m->angular_momentum[2]/=c->yaw_scale;m->adjusted=selected;}
float body_inverse_yaw(const BodyMotion*m,const BodyCar*c){float yaw=m->adjusted?(float)((double)c->inertia[2]*c->yaw_scale):c->inertia[2];return (float)(1/(double)yaw);}
BodyMode body_mode(int state,int race,float throttle,int contacts,float fz){int enabled=state==0||state==2;return (BodyMode){.curve=enabled,.stabilize=enabled&&!(race==4&&(throttle>.3f||(contacts==4&&fabsf(fz)>.15f)))};}
float body_torque_limit(int role,const float limits[2],float speed,float sigma,float Lz){float limit=limits[role!=0];if(role&&limit==0){if(speed<1)sigma=0;if((double)sigma*Lz<0&&fabsf(sigma)>.1f)limit=20;}return limit;}
float body_damping(float w){static const float y[6]={.05f,.05f,.25f,.7f,2,5};if(w<=0)return y[0];if(w>=10)return y[5];int k=(int)(w/2);double t=((double)w-2*k)/2;return (float)(y[k]+t*((double)y[k+1]-y[k]));}
void body_angular_derivative(const float L[3],const float T[3],float w,float z,const BodyMode*mode,float out[3]){
 double damping=(mode->curve?body_damping(w):0)+(mode->stabilize?2*(double)z:0);for(int k=0;k<3;k++)out[k]=(float)(T[k]-damping*L[k]);
 float K=mode->torque_limit;if(K!=0){if(T[2]>K)out[2]=K;else if(T[2]<-K)out[2]=-K;}
}
void body_initialize(BodyMotion*m,ResBody*b,const BodyCar*c,const float velocity[3],const float omega[3]){float v[3],w[3];memcpy(v,velocity,sizeof v);memcpy(w,omega,sizeof w);for(int k=0;k<3;k++)b->momentum[k]=(float)((double)b->mass*v[k]);memcpy(b->velocity,v,sizeof v);memcpy(m->omega,w,sizeof w);inertia_map(&m->axes,c->inertia,w,0,m->angular_momentum);}
void body_publish_angular(BodyMotion*m,const BodyCar*c){float diagonal[3];memcpy(diagonal,c->inertia,sizeof diagonal);if(m->adjusted)diagonal[2]=(float)((double)diagonal[2]*c->yaw_scale);inertia_map(&m->axes,diagonal,m->angular_momentum,1,m->omega);cap(m->omega,250);}
void body_collect(BodyMotion*m,ResBody*b,const BodyCar*c,const BodyMode*mode,const ResCar*aero,const ResInput*input,BodyDerivative*out){
 (void)c;float w=(float)magnitude(m->omega);ResInput in=*input;in.stabilize=mode->stabilize;res_derivative(b,aero,&in,&m->axes,w,&out->linear);
 memcpy(out->position,b->velocity,sizeof out->position);cross(m->omega,m->axes.forward,out->axes.forward);cross(m->omega,m->axes.lateral,out->axes.lateral);cross(m->omega,m->axes.up,out->axes.up);body_angular_derivative(m->angular_momentum,b->moment,w,b->z,mode,out->angular_momentum);
}
void body_integrate(BodyMotion*m,ResBody*b,const BodyDerivative*d,float h){
 for(int k=0;k<3;k++){m->position[k]=(float)(m->position[k]+(double)h*d->position[k]);m->angular_momentum[k]=(float)(m->angular_momentum[k]+(double)h*d->angular_momentum[k]);m->axes.forward[k]=(float)(m->axes.forward[k]+(double)h*d->axes.forward[k]);m->axes.lateral[k]=(float)(m->axes.lateral[k]+(double)h*d->axes.lateral[k]);m->axes.up[k]=(float)(m->axes.up[k]+(double)h*d->axes.up[k]);}res_integrate(b,&d->linear,h);
}
void body_post(BodyMotion*m,ResBody*b,const BodyCar*c,float gravity){for(int k=0;k<3;k++)m->force_response[k]=(float)((double).10204081237316132f*b->force[k]/b->mass);res_post(b,0);body_publish_angular(m,c);if(gravity>0)b->force[2]=(float)(-(double)b->mass*gravity);}
void body_repair(ResBasis*b){double len=magnitude(b->forward);for(int k=0;k<3;k++)b->forward[k]=(float)(b->forward[k]/len);cross(b->forward,b->lateral,b->up);len=magnitude(b->up);for(int k=0;k<3;k++)b->up[k]=(float)(b->up[k]/len);cross(b->up,b->forward,b->lateral);}
void body_point_force(ResBody*b,const float force[3],const float arm[3]){res_point_force(b,force,arm,NULL);}
void body_impulse(BodyMotion*m,ResBody*b,const float impulse[3],const float arm[3],float scale){float j[3],angular[3];memcpy(j,impulse,sizeof j);cap(j,1000);cross(arm,j,angular);cap(angular,10);for(int k=0;k<3;k++){b->momentum[k]+=j[k];m->angular_momentum[k]=(float)(m->angular_momentum[k]+(double)scale*angular[k]);}}
void body_opposing(ResBody*b,const BodyMotion*m,int state,unsigned selector){double w=magnitude(m->omega);if(state!=0||selector>=2||w>=.5f)return;double H=w<=.4f?0:20*(w-.4f)/(2-(double).4f);float scale=(float)(-b->mass*H);for(int k=0;k<3;k++)b->moment[k]=(float)(b->moment[k]+(double)scale*m->omega[k]);}
