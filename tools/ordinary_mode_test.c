/* Approved OM vectors and independent OM-2a host scheduling. No retail data. */
#define STAGE5_TEST_NO_MAIN
#include "drive_stage5_test.c"
#include "drive_session.h"
static const char *g_dir="docs/cleanroom-driving-physics/ordinary-mode/test-vectors";
static int g_fail;
#include "test_json.h"
static void exact(float a,double b){assert(isfinite(a)&&fabs((double)a-b)<=1e-6);}
static void read3(const J*j,const char*k,float v[3]){J*a=jget(j,k);assert(a&&a->n==3);for(int q=0;q<3;q++)v[q]=(float)a->kid[q]->num;}
static void vectors(void){J*root=jload("ordinary-mode.json"),*v=jget(root,"vectors");assert(v&&v->n==13);for(int n=0;n<v->n;n++){
 J*i=jget(v->kid[n],"input"),*e=jget(v->kid[n],"expected");assert(jstrv(v->kid[n],"id"));
 BodyCar car={.inertia={1,1,1},.yaw_scale=1};BodyMotion m={.axes={{1,0,0},{0,1,0},{0,0,1}}};ResBody b={.mass=1,.z=(float)jnum(i,"z",0)};
 m.omega[0]=(float)jnum(i,"held_angular_speed",0);
 read3(i,"P",b.momentum);read3(i,"F",b.force);read3(i,"L",m.angular_momentum);read3(i,"T",b.moment);
 BodyMotion saved_m=m;ResBody saved_b=b;
 int status=-1;DriveSession session={DRIVE_SESSION_SUPPLIED,(int)jnum(i,"session_status",-1)};
 assert(drive_session_status(session,0,&status)&&status==session.status);
 BodyMode mode=body_mode((int)jnum(i,"vehicle_state",0),status,(float)jnum(i,"shaped_throttle_at_refresh",0),(int)jnum(i,"previous_contacts",0),(float)jnum(i,"previous_forward_z",0));
 assert(mode.stabilize==jbool(e,"stabilize")&&mode.curve==jbool(e,"curve"));assert(!memcmp(&saved_m,&m,sizeof m)&&!memcmp(&saved_b,&b,sizeof b));exact(b.z,jnum(e,"z_unchanged_by_mode_refresh",-1));
 ResCar aero={0};ResInput in={.scale=1,.throttle=jbool(i,"force_threshold_disabled")?1:0};BodyDerivative derivative;body_collect(&m,&b,&car,&mode,&aero,&in,&derivative);
 float pd[3],ld[3];read3(e,"Pdot",pd);read3(e,"Ldot",ld);for(int q=0;q<3;q++){exact(derivative.linear.momentum[q],pd[q]);exact(derivative.angular_momentum[q],ld[q]);}
 }jfree(root);puts("ordinary-mode: all 13 approved vectors passed");}
static void dynamic_fixture(Trace*t){contact_init(t,0);BodyCar c={.inertia={1,1,1},.yaw_scale=1};assert(!stage5_drive_body(&t->drive,&c,1,0,1));t->drive.resistance_car=(ResCar){0};}
typedef struct {int count;ResBody preceding;} Observe;
static void observe_mode(void*user,const Stage1Drive*s,float h,int preparation){
 (void)h;if(preparation)return;Observe*o=user;assert(s->prepared&&s->dynamic_initialized&&s->body_race==4&&!s->body_mode.stabilize);
 if(!o->count){assert(s->body.z==o->preceding.z);assert(!memcmp(s->body.force,o->preceding.force,sizeof s->body.force));assert(!memcmp(s->body.moment,o->preceding.moment,sizeof s->body.moment));}
 o->count++;
}
static void lifecycle(void){
 int status=19;DriveSession active={DRIVE_SESSION_ACTIVE,0},start={DRIVE_SESSION_COUNTDOWN,0},pause={DRIVE_SESSION_SUSPENDED,0};
 assert(!drive_session_status(active,0,&status)&&status==19);assert(!drive_session_status(start,0,&status)&&status==19);assert(!drive_session_status(pause,1,&status)&&status==19);
 const char*family[]={"free-roam","circuit","sprint"};for(int f=0;f<3;f++){assert(family[f]);Trace ordinary;dynamic_fixture(&ordinary);ordinary.drive.session=drive_session_local(1,0);tick5(&ordinary,1,0,0,NULL);
 assert(ordinary.drive.body_race==4&&!ordinary.drive.body_mode.stabilize);}
 assert(drive_session_status(drive_session_local(0,1),1,&status)&&status==3);assert(drive_session_status(drive_session_local(1,0),1,&status)&&status==4);
 assert(!drive_session_status(drive_session_local(0,0),1,&status)&&status==4);
 Trace t;dynamic_fixture(&t);t.drive.session=start;tick5(&t,1,0,0,NULL);assert(t.drive.body_race==3&&t.drive.body_mode.stabilize);
 /* Pure producer resolution does not touch prepared physical state. */
 t.drive.body.z=1;Stage1Drive saved=t.drive;t.drive.session=active;assert(drive_session_status(t.drive.session,t.drive.dynamic_initialized&&t.drive.prepared,&status));t.drive.session=saved.session;assert(!memcmp(&t.drive,&saved,sizeof saved));
 t.drive.session=pause;saved=t.drive;float p[3],v[2],h=t.h,s=t.s;memcpy(p,t.p,sizeof p);memcpy(v,t.v,sizeof v);for(int k=0;k<180;k++)stage1_drive_step(&t.drive,t.p,t.v,&t.h,&t.s,1,0,0,NULL,NULL,NULL);
 assert(!memcmp(&t.drive,&saved,sizeof saved)&&!memcmp(t.p,p,sizeof p)&&!memcmp(t.v,v,sizeof v)&&t.h==h&&t.s==s);
 Observe observed={.preceding=t.drive.body};t.drive.session=active;t.drive.body_observe=observe_mode;t.drive.body_observe_user=&observed;tick5(&t,1,0,0,NULL);assert(observed.count>0);t.drive.body_observe=NULL;t.drive.body_observe_user=NULL;
 /* Refresh-only release/reapply keeps positive history and prepared data. */
 saved=t.drive;BodyMode release=body_mode(0,4,0,0,0),reapply=body_mode(0,4,1,0,0);assert(release.stabilize&&!reapply.stabilize);assert(!memcmp(&saved,&t.drive,sizeof saved));
 /* Real coasting advances the approved z law rather than freezing history. */
 t.drive.body.z=.1f;float oldz=t.drive.body.z;tick5(&t,0,0,0,NULL);assert(t.drive.body_mode.stabilize&&t.drive.body.z!=oldz);
 stage1_drive_reset(&t.drive,t.v,t.h);assert(!t.drive.dynamic_initialized&&!t.drive.prepared&&t.drive.body.z==0&&t.drive.session.phase==DRIVE_SESSION_ACTIVE);
 tick5(&t,1,0,0,NULL);assert(t.drive.dynamic_initialized&&t.drive.prepared&&t.drive.body_race==4&&!t.drive.body_mode.stabilize);
 t.drive.session=(DriveSession){DRIVE_SESSION_SUPPLIED,3};tick5(&t,1,0,0,NULL);assert(t.drive.body_race==3&&t.drive.body_mode.stabilize);
 t.drive.session=(DriveSession){DRIVE_SESSION_SUPPLIED,4};tick5(&t,1,0,0,NULL);assert(t.drive.body_race==4&&!t.drive.body_mode.stabilize);
 puts("ordinary-mode: readiness, countdown, pause/resume, preparation, coast and reset passed");
}
int main(void){vectors();lifecycle();return g_fail?1:0;}
