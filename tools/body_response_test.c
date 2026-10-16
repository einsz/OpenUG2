/* Every approved synthetic BR vector, entered at its declared boundary. */
#include <math.h>
#include <stdio.h>
#include "body_response.h"
#include "contact.h"
static const char *g_dir;static int g_fail;static const char *case_id;
#include "test_json.h"
static void check(double actual,double expected,const char *field){if(!isfinite(actual)||fabs(actual-expected)>2e-5+2e-6*fabs(expected)){printf("FAIL %s %s: %.10g != %.10g\n",case_id,field,actual,expected);g_fail++;}}
static void vector(const J *j,float a[3]){if(j)for(int i=0;i<3;i++)a[i]=(float)j->kid[i]->num;}
static void getv(const J*j,const char*k,float a[3]){vector(jget(j,k),a);}
static void basis(const J*j,ResBasis*b){if(j){vector(j->kid[0],b->forward);vector(j->kid[1],b->lateral);vector(j->kid[2],b->up);}}
static void cv(const J*j,const char*k,const float a[3]){J*v=jget(j,k);if(v)for(int i=0;i<3;i++)check(a[i],v->kid[i]->num,k);}
static void cb(const J*j,const char*k,const ResBasis*b){J*v=jget(j,k);if(v){const float *a[3]={b->forward,b->lateral,b->up};for(int i=0;i<3;i++)for(int q=0;q<3;q++)check(a[i][q],v->kid[i]->kid[q]->num,k);}}
static void state(const J*e,const BodyMotion*m,const ResBody*b){cv(e,"X",m->position);cv(e,"P",b->momentum);cv(e,"V",b->velocity);cv(e,"L",m->angular_momentum);cv(e,"Omega",m->omega);cv(e,"F",b->force);cv(e,"T",b->moment);cb(e,"B_columns",&m->axes);}
int main(int argc,char**argv){g_dir=argc>1?argv[1]:"docs/cleanroom-driving-physics/body-response/test-vectors";J*root=jload("body-response.json");J*vs=jget(root,"vectors");if(!vs)return 1;int count=0;
 for(int n=0;n<vs->n;n++){J*v=vs->kid[n],*i=jget(v,"inputs"),*e=jget(v,"expected");case_id=jstrv(v,"id");const char*op=jstrv(v,"operation");
  ResBody b={.mass=(float)jnum(i,"M",1),.z=(float)jnum(i,"z",0)};BodyMotion m={.axes={{1,0,0},{0,1,0},{0,0,1}},.adjusted=(int)jnum(i,"s",0)};BodyCar c={.inertia={1,1,1},.yaw_scale=(float)jnum(i,"kI",1)};
  getv(i,"P",b.momentum);getv(i,"V",b.velocity);getv(i,"F",b.force);getv(i,"T",b.moment);getv(i,"L",m.angular_momentum);getv(i,"Omega",m.omega);getv(i,"X",m.position);getv(i,"I_selected",c.inertia);basis(jget(i,"B_columns"),&m.axes);
  BodyMode mode={.curve=jbool(i,"curve"),.stabilize=jbool(i,"stabilize"),.torque_limit=(float)jnum(i,"K",0)};
#define NUM(k) ((float)jnum(i,k,0))
  if(!strcmp(op,"inertia transition")){body_select(&m,&c,(int)NUM("desired_s"));check(m.adjusted,jnum(e,"s",0),"s");cv(e,"L",m.angular_momentum);}
  else if(!strcmp(op,"frame selection"))check(body_selection((int)NUM("state"),(unsigned)NUM("c")),jnum(e,"desired_s",0),"desired_s");
  else if(!strcmp(op,"frame damping gates")){mode=body_mode((int)NUM("state"),(int)NUM("race"),NUM("throttle"),(int)NUM("contact_count"),NUM("fz"));check(mode.curve,jbool(e,"curve"),"curve");check(mode.stabilize,jbool(e,"stabilize"),"stabilize");}
  else if(!strcmp(op,"frame torque-limit selection")){float limits[2]={NUM("K0"),NUM("K1")};check(body_torque_limit((int)NUM("p"),limits,NUM("v"),NUM("sigma"),NUM("Lz")),jnum(e,"K",0),"K");}
  else if(!strcmp(op,"angular derivative before restriction")){float d[3];body_angular_derivative(m.angular_momentum,b.moment,NUM("w"),b.z,&mode,d);cv(e,"Ldot",d);if(jget(e,"D"))check(body_damping(NUM("w")),jnum(e,"D",0),"D");}
  else if(!strcmp(op,"world-vertical derivative replacement")){float L[3]={0,0,1},T[3]={0,0,NUM("raw_Tz")},d[3];mode.curve=0;mode.stabilize=1;body_angular_derivative(L,T,0,-NUM("damping_z")/2,&mode,d);check(d[2],jnum(e,"Ldot_z",0),"Ldot_z");}
  else if(!strcmp(op,"body angular publication")){body_publish_angular(&m,&c);state(e,&m,&b);}
  else if(!strcmp(op,"orientation repair")){basis(jget(i,"candidate_columns"),&m.axes);body_repair(&m.axes);cb(e,"B_columns",&m.axes);}
  else if(!strcmp(op,"point-force addition")||!strcmp(op,"two identical point-force additions")){float q[3]={0},r[3]={0};getv(i,"Q",q);getv(i,"r",r);body_point_force(&b,q,r);if(op[0]=='t')body_point_force(&b,q,r);state(e,&m,&b);}
  else if(!strcmp(op,"impulse addition")){float q[3]={0},r[3]={0};getv(i,"J",q);getv(i,"r",r);body_impulse(&m,&b,q,r,NUM("angular_scale"));state(e,&m,&b);}
  else if(!strcmp(op,"late prepared angular torque")){body_opposing(&b,&m,(int)NUM("state"),(unsigned)NUM("c"));cv(e,"T",b.moment);}
  else if(!strcmp(op,"pose/velocity initialization")){float omega[3]={0};getv(i,"I_basic",c.inertia);getv(i,"Omega_input",omega);float vel[3];memcpy(vel,b.velocity,sizeof vel);body_initialize(&m,&b,&c,vel,omega);state(e,&m,&b);check(m.adjusted,jnum(e,"s",0),"s");}
  else if(!strcmp(op,"body force publication and reseed")){body_post(&m,&b,&c,NUM("gravity"));cv(e,"Aforce",m.force_response);cv(e,"consumed_F",b.saved_force);cv(e,"consumed_T",b.saved_moment);state(e,&m,&b);}
  else if(!strcmp(op,"candidate-basis contact alignment")){float l[3]={0},normal[3]={0};getv(i,"wheel_lateral",l);getv(i,"normal",normal);float load=ct_align(NUM("S"),l,normal);check(load,jnum(e,"N",0),"N");check(load/NUM("S"),jnum(e,"alignment",0),"alignment");}
  else if(!strcmp(op,"one Euler substep then body publication then final repair")||!strcmp(op,"two substeps with supplied constant force/moment; all unspecified contributions zero")){
   ResInput in={.scale=1};ResCar aero={0};float force[3],torque[3];memcpy(force,b.force,sizeof force);memcpy(torque,b.moment,sizeof torque);int steps=op[0]=='t'?2:1;
   for(int k=0;k<steps;k++){BodyDerivative d;body_collect(&m,&b,&c,&mode,&aero,&in,&d);body_integrate(&m,&b,&d,NUM("h"));body_post(&m,&b,&c,NUM("gravity"));if(steps==1)cb(e,"B_before_repair_columns",&m.axes);body_repair(&m.axes);state(steps==1?e:jget(e,k?"after_second":"after_first"),&m,&b);memcpy(b.force,force,sizeof force);memcpy(b.moment,torque,sizeof torque);}
  }else{printf("FAIL unhandled %s\n",op);g_fail++;}
  count++;
#undef NUM
 }
 jfree(root);printf("body-response: %d vectors, %d failures\n",count,g_fail);return g_fail?1:0;
}
