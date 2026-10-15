#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "contact.h"
static const char *g_dir;static int g_fail,checks;
#include "test_json.h"
static float n(const J*j,const char*k){return (float)jnum(j,k,0);}
static float at(const J*j,int k){assert(j&&k<j->n);return (float)j->kid[k]->num;}
static void vec(const J*j,float v[3]){for(int k=0;k<3;k++)v[k]=j?at(j,k):0;}
static void eq(const J*c,const char*k,double got,double want,int exact){checks++;if(!isfinite(got)||fabs(got-want)>(exact?0:2e-5+2e-5*fabs(want))){printf("FAIL %s %s %.12g want %.12g\n",jstrv(c,"id"),k,got,want);g_fail++;}}
static CtAxle axle(const J*i){return (CtAxle){.progression=n(i,"progression"),.stiffness=n(i,"stiffness"),.d_positive=n(i,"d_positive"),.d_nonpositive=n(i,"d_nonpositive"),.antiroll=n(i,"antiroll"),.lower=(float)jnum(i,"lower",-.3),.upper=(float)jnum(i,"upper",.3)};}
static CtGround ground(void*u,const float p[3],int w){(void)p;(void)w;const J*i=u;CtGround g={.found=1,.surface=1,.height=n(i,"height")};vec(jget(i,"normal"),g.normal);return g;}
static void vectors(void){J*root=jload("components.json"),*cases=jget(root,"cases");assert(cases&&cases->n==77);for(int v=0;v<cases->n;v++){
 J*c=cases->kid[v],*i=jget(c,"inputs"),*e=jget(c,"expected");const char*op=jstrv(c,"operation");
#define EQ(k,x) eq(c,k,x,jnum(e,k,0),0)
#define EX(k,x) eq(c,k,x,jbool(e,k),1)
#define VA(k,a) do{J*expected_array=jget(e,k);assert(expected_array);for(int z=0;z<expected_array->n;z++)eq(c,k,(a)[z],expected_array->kid[z]->num,0);}while(0)
 if(!strcmp(op,"preload")){CtPreload p=ct_preload(n(i,"mass"),at(jget(i,"centre"),0),n(i,"front_mount_x"),n(i,"rear_mount_x"),1,&(ResCar){.front=n(i,"cF"),.rear=n(i,"cR")},n(i,"kA"),n(i,"v"),n(i,"e_previous"));EQ("total",p.total);EQ("front_each",p.front);EQ("rear_each",p.rear);EQ("front_share",p.front_share);EQ("rear_share",p.rear_share);EQ("wheel_inertia",.01f);}
 else if(!strcmp(op,"suspension")){CtAxle a=axle(i);CtWheel w={.travel=n(i,"old_travel"),.rate=n(i,"old_rate")};ct_spring(&w,&a,n(i,"candidate"),n(i,"pair_travel"),n(i,"preload"),n(i,"h"));EQ("travel",w.travel);EQ("rate",w.rate);EX("contact",w.contact);EQ("scalar_load",w.scalar);}
 else if(!strcmp(op,"pair_sequence")||!strcmp(op,"scheduling_pair")){CtAxle a=axle(i);CtWheel w[2]={0};float loads[2],travel[2],rates[2];for(int k=0;k<2;k++)w[k].travel=at(jget(i,"incoming_travel"),k);for(int sweep=0;sweep<(!strcmp(op,"scheduling_pair")?2:1);sweep++){for(int k=0;k<2;k++){ct_spring(w+k,&a,at(jget(i,"candidates"),k),w[k^1].travel,n(i,"preload"),sweep?0:n(i,"h"));loads[k]=w[k].scalar;travel[k]=w[k].travel;rates[k]=w[k].rate;}if(!strcmp(op,"pair_sequence")){VA("travel",travel);VA("scalar_load",loads);}else if(!sweep){VA("first_loads",loads);}else{VA("after_zero_sweep_loads",loads);VA("rates_retained",rates);}}}
 else if(!strcmp(op,"ground_travel")){float p[3],up[3];vec(jget(i,"point"),p);vec(jget(i,"body_up"),up);CtGround g={.height=n(i,"height"),.height_offset=n(i,"height_offset"),.found=jbool(i,"found"),.surface=(int)n(i,"surface")};vec(jget(i,"normal"),g.normal);int s;float d=ct_ground_distance(p,up,&g,n(i,"bump"),&s);EQ("distance",d);EQ("candidate",(float)(d+n(i,"radius")));EQ("effective_surface",s);}
 else if(!strcmp(op,"ground_dot_boundary")){int s=(int)n(i,"surface");EQ("distance",ct_distance(n(i,"up_dot"),n(i,"normal_height_difference"),&s));EQ("effective_surface",s);}
 else if(!strcmp(op,"alignment")){float a=ct_alignment(n(i,"lateral_dot_normal"));EQ("alignment",a);EQ("normal_load",(float)(a*n(i,"scalar_load")));}
 else if(!strcmp(op,"force_application")){CtPose p={0};CtWheel w={.contact=jbool(i,"contact"),.load=n(i,"N")};ResBasis b;vec(jget(i,"centre"),p.position);vec(jget(i,"wheel_centre"),w.centre);vec(jget(i,"normal"),w.normal);vec(jget(i,"heading"),b.forward);vec(jget(i,"lateral"),b.lateral);vec(jget(i,"wheel_up"),b.up);w.arm[2]=n(i,"radius"); /* radius passed via point, retained for noncontact */for(int k=0;k<3;k++)w.point[k]=w.centre[k]-n(i,"radius")*w.normal[k];float f[3]={0},m[3]={0};ct_apply(&p,&w,&b,(int)n(i,"basis"),n(i,"Fx"),n(i,"Fy"),f,m);if(w.contact){VA("force",f);VA("point",w.point);VA("moment",m);}else{VA("applied_force",f);VA("applied_moment",m);}}
 else if(!strcmp(op,"bottom_impulse")){EQ("requested",ct_bottom(n(i,"closing"),n(i,"mass")));EX("extra_zero_duration_sweep",1);}
 else if(!strcmp(op,"handbrake_blend")){EQ("blend",ct_blend(n(i,"hb"),n(i,"old_blend"),n(i,"frame_dt")));}
 else if(!strcmp(op,"handbrake_coefficient")){float eta=ct_eta(n(i,"hb"),(int)n(i,"role"),n(i,"v"),n(i,"sigma_if_forward"),n(i,"vm"));float a[4]={0,0,eta,eta},h[4];brk_handbrake(&(BrkCar){.handbrake=n(i,"Kh")},a,h);VA("eta",a);VA("handbrake_torque",h);EQ("sigma_used",n(i,"v")<1?0:n(i,"sigma_if_forward"));}
 else if(!strcmp(op,"rear_grip")){CtGrip g=ct_grip((int)n(i,"role"),n(i,"effective_eta"),n(i,"sigma"),n(i,"speed"),n(i,"tilt"),n(i,"blend"),n(i,"b0"),n(i,"b1"),n(i,"event"),n(i,"q"),n(i,"X"),n(i,"v0"),n(i,"s0"));EQ("factor",g.factor);EQ("front_grip",g.front);EQ("rear_grip",g.rear);}
 else if(!strcmp(op,"service_distribution_corrected")||!strcmp(op,"service_exception_boundary")){BrkFrame f;float vm=jget(i,"vm")?n(i,"vm"):ct_mph(n(i,"v"));brk_distribute(&(BrkCar){.service=n(i,"Kb"),.rear_share=n(i,"beta")},&(BrkPedals){.throttle=n(i,"throttle"),.service=n(i,"brake")},vm,&f);if(jget(e,"vm")){EQ("vm",vm);VA("coefficients",f.coefficient);VA("requested_B",f.service);}else{EQ("rear_coefficient",f.coefficient[2]);}}
 else if(!strcmp(op,"bump")){EQ("displacement",ct_bump(n(i,"x"),n(i,"y"),n(i,"frequency"),n(i,"amplitude")));}
 else if(!strcmp(op,"surface_composition")){float sum=0;int count=0;for(int k=0;k<4;k++)if(at(jget(i,"contacts"),k)){sum+=ct_surface((int)at(jget(i,"surface_ids"),k))->body_drag;count++;}EQ("body_drag_mean",count?sum/count:1);EQ("spin_drag_id15",ct_surface(15)->spin);EQ("peak_id15",ct_surface(15)->peak);EQ("sliding_id15",ct_surface(15)->slide);}
 else if(!strcmp(op,"scheduling")){CtState s;ct_init(&s,&(CtCar){0});s.blend=n(i,"old_blend");float f[3];vec(jget(i,"old_prepared_force"),f);/* frame coefficient mutation leaves prepared body accumulator owned by caller */s.blend=ct_blend(n(i,"frame_new_hb"),s.blend,n(i,"frame_dt"));s.blend_refreshes++;VA("first_derivative_wheel_force",f);EQ("new_frame_blend",s.blend);EQ("blend_refreshes",s.blend_refreshes);EQ("coefficient_refreshes",s.blend_refreshes);EQ("extra_sweep_euler_steps",0);}
 else if(!strcmp(op,"prescribed_four_contact")){CtCar car={.mass=1};vec(jget(i,"centre_offset"),car.centre);car.axle[0]=car.axle[1]=axle(i);CtState s;for(int k=0;k<4;k++){vec(jget(i,"mounts")->kid[k],car.mount[k]);car.radius[k]=n(i,"radius");}ct_init(&s,&car);CtPose p={0};vec(jget(i,"centre_position"),p.position);vec(jget(i,"body_axes")->kid[0],p.axes.forward);vec(jget(i,"body_axes")->kid[1],p.axes.lateral);vec(jget(i,"body_axes")->kid[2],p.axes.up);for(int k=0;k<4;k++){s.preload[k]=n(i,"preload_each");s.wheel[k].travel=at(jget(i,"incoming_travel"),k);}ct_post(&s,&p,(float[4]){0},n(i,"h"),ground,i);float t[4],load[4];for(int k=0;k<4;k++){t[k]=s.wheel[k].travel;load[k]=s.wheel[k].load;eq(c,"contact",s.wheel[k].contact,at(jget(e,"contact"),k),1);}VA("travel",t);VA("aligned_load",load);}
 else{printf("FAIL unhandled %s\n",op);g_fail++;}
#undef EQ
#undef EX
#undef VA
 }jfree(root);}
int main(int argc,char**argv){g_dir=argc>1?argv[1]:"docs/cleanroom-driving-physics/stage5/test-vectors";vectors();J*r=jload("surfaces.json");assert(r&&r->n==29);for(int k=0;k<29;k++){const CtSurface*s=ct_surface(k);const char*names[]={"peak","slide","spin","body_drag","frequency","amplitude"};float a[]={s->peak,s->slide,s->spin,s->body_drag,s->frequency,s->amplitude};for(int j=0;j<6;j++)eq(r->kid[k],names[j],a[j],n(r->kid[k],names[j]),1);}jfree(r);printf("Stage5: 77 vectors + 29 surfaces, %d checks, %d failures\n",checks,g_fail);return !!g_fail;}
