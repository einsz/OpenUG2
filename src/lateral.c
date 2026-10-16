#include <math.h>
#include <string.h>
#include "lateral.h"
static const float pi=3.141592741012573f,turn=.00009587380191078410f;
static double clamp(double x,double a,double b){return fmax(a,fmin(b,x));}
static double curve(double x,double a,double k,const float*y,int count){
 double p=(x-a)*k;if(p<=0)return y[0];if(p>=count-1)return y[count-1];
 int i=(int)p;return y[i]+(p-i)*((double)y[i+1]-y[i]);
}
float lat_curve(const char*name,float x){
 static const float I[]={-1,-.712f,-.453f,-.303f,-.216f,-.148f,-.116f,-.080f,-.061f,-.034f,0,.034f,.061f,.080f,.116f,.148f,.216f,.303f,.453f,.712f,1};
 static const float J[]={-1,-.800f,-.615f,-.483f,-.388f,-.288f,-.220f,-.161f,-.111f,-.057f,0,.057f,.111f,.161f,.220f,.288f,.388f,.483f,.615f,.800f,1};
 static const float Lk[]={40,20,10,5,3.5f,3.1f,2.9f,2.9f,2.9f,2.9f},La[]={45,15,11,8,7,7,7,7,7,7},R[]={1,.92f,.78f,.64f,.54f,.48f,.42f,.38f,.34f,.32f};
 static const float H[]={1,1,1.1f,1.2f,1.25f,1.35f},T[]={1,1.05f,1.1f,1.2f,1.3f,1.4f},E[]={1,1.05f,1.1f,1.5f,2.2f,3.1f},C[]={.5f,.6f,.7f,.8f,.95f};
#define CVAL(n,a,k) if(!strcmp(name,#n))return (float)curve(x,a,k,n,sizeof n/sizeof *n)
 CVAL(I,-1,10);CVAL(J,-1,10);CVAL(Lk,0,.05624999850988388f);CVAL(La,0,.05624999850988388f);CVAL(R,0,.05624999850988388f);CVAL(H,0,5);CVAL(T,0,5);CVAL(E,0,.5);CVAL(C,0,4);
#undef CVAL
 return NAN;
}
LatCommand lat_filter(const LatFilter*i){
 LatCommand o={0};double j=lat_curve(i->analog?"J":"I",i->analog?(float)clamp(i->raw,-1,1):i->raw);
 double response=lat_curve("R",i->vctl),p=1-(i->throttle+1-(i->brake+i->handbrake)*.5)*.5;
 double limit=lat_curve(i->analog?"La":"Lk",i->vctl)*(1+response*p)*lat_curve("H",fabsf(i->history));
 o.latch=i->latch;if(o.latch){if(fabsf(i->sigma)<5||i->speed<0)o.latch=0;}
 else if(fabsf(i->sigma)>15&&i->speed>=13.410818099975586f)o.latch=1;
 if(j*i->sigma<0&&(o.latch||fabs(i->sigma)>limit))limit=fmin(fabs(i->sigma),45)*(i->analog?1:lat_curve("C",(float)fabs(j)));
 else if(fabsf(i->history)>=.5f)limit=fmax(limit,i->previous_limit);
 o.previous_limit=i->previous_limit;if(!i->analog){limit=fmin(limit,45);o.previous_limit=(float)limit;}
 double angle=clamp(limit*i->gain*j,-45,45);
 if(!i->analog){double step=180*i->rate_history*response*lat_curve("T",fabsf(i->history))*i->gain*i->dt;angle=clamp(angle,i->old_degrees-step,i->old_degrees+step);}
 /* Two signed truncations are deliberate (S4-2). */
 int32_t units=(int32_t)(angle*65536);o.command=(uint16_t)(units/360);
 o.mapped=(float)j;o.limit=(float)limit;o.degrees=(float)angle;return o;
}
float lat_decode(uint16_t u){double a=u*(360.0/65536);return (float)(a>=180?a-360:a);}
void lat_geometry(uint16_t u,float length,float width,float a[4]){
 float theta=(float)(u*turn);if(theta>pi)theta=(float)(theta-2*(double)pi);
 double v=fabs(theta),c=length*v/(length+v*width);a[0]=(float)(theta>=0?c:-v);a[1]=(float)(theta>=0?v:-c);a[2]=a[3]=0;
}
uint16_t lat_heading(float x,float y){
 double ax=fabs(x),ay=fabs(y);if(ax==0&&ay==0)return 0;
 int a;if(ax==ay)a=8192;else {
  int n=(int)(256*fmin(ax,ay)/fmax(ax,ay));
  int q=(int)floor(atan(n/256.0)*1024/3.14159265358979323846+.5);
  switch(n){case 5:case 9:case 20:case 24:case 44:case 48:case 57:case 62:case 67:case 85:case 92:case 101:case 113:++q;break;
  case 154:case 166:case 193:case 198:case 203:case 223:case 230:case 243:case 246:--q;break;default:break;}
  a=ax>ay?32*q:16384-32*q;
 }
 if(x<0)a=32768-a;
 if(y<0)a=-a;
 return (uint16_t)a;
}
float lat_slip(float vx,float vy){
 double a=lat_heading(vx,vy)*(double)turn;if(a>pi)a-=2*(double)pi;
 if(vx<0){a+=pi;if(a>pi)a-=2*(double)pi;}return (float)a;
}
float lat_body_slip(float forward,const float body_forward[2],const float velocity[2]){
 if(forward<1)return 0;
 int difference=(int)lat_heading(body_forward[0],body_forward[1])-(int)lat_heading(velocity[0],velocity[1]);
 if(difference<=-32768)difference+=65536;
 if(difference>32768)difference-=65536;
 return (float)(difference*(360.0/65536));
}
float lat_grip(float load,float scale,float reference,float factor,float alpha,float vx,float mult){
 static const float g[7][6]={{0,0,0,0,0,0},{0,1.2f,2.3f,3,3,2.8f},{0,1.7f,3.2f,4.3f,5.1f,5.2f},{0,1.8f,3.5f,4.9f,5.8f,6.1f},{0,1.83f,3.6f,5,5.96f,6.4f},{0,1.86f,3.7f,5.1f,6.13f,6.7f},{0,1.9f,3.8f,5.2f,6.3f,7.1f}};
 double z=fmin(6,6*fabs(alpha)*(double)57.2957763671875f/scale),x=(double).8f*load/reference;int k=(int)z;
 double f=curve(x,0,.5,g[k],6);if(k<6)f+=(z-k)*(curve(x,0,.5,g[k+1],6)-f);
 f*=factor*2.5*mult;if((alpha>0)!=(vx<=0))f=-f;return (float)f;
}
int lat_sliding(float s,float alpha,float scale,int locked,float vx,float vy){return hypot(s,vy)>0&&(vx!=0||vy!=0)&&(fabsf(s)>.5f||fabsf(alpha)>scale||locked);}
void lat_cap(float*fx,float*fy,float limit,float tau,int locked,float vx,float alpha){
 double f=*fx,l=*fy,test=tau>0&&!locked?1.5*f:f,mag=hypot(test,l);
 if(mag>limit&&mag>.001){f=test*limit/mag;l*=limit/mag;}
 if(fabsf(vx)>1)f-=(double).15f*sin(alpha)*l;
 *fx=(float)f;*fy=(float)l;
}
void lat_contact(const float v[2],float w,const float arm[2],float a,float u[2],float*vx,float*vy){
 u[0]=(float)(v[0]-(double)w*arm[1]);u[1]=(float)(v[1]+(double)w*arm[0]);
 *vx=(float)(cos(a)*u[0]+sin(a)*u[1]);*vy=(float)(-sin(a)*u[0]+cos(a)*u[1]);
}
void lat_accumulate(float f[2],float*m,const float arm[2],float a,float fx,float fy){
 double x=cos(a)*fx-sin(a)*fy,y=sin(a)*fx+cos(a)*fy;
 f[0]=(float)(f[0]+x);f[1]=(float)(f[1]+y);*m=(float)(*m+arm[0]*y-arm[1]*x);
}
float lat_inertia(float mass,const float d[3]){return (float)((double)mass*((double)d[0]*d[0]+(double)d[1]*d[1])*(double)(1.0f/12));}
