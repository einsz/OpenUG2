/* Approved Stage4 components. All fixture fields come from reviewed vectors. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "lateral.h"
static const char *g_dir;static int g_fail,checks;
#include "test_json.h"
static float n(const J*j,const char*k){return (float)jnum(j,k,0);}
static double at(const J*j,int k){assert(j&&k<j->n);return j->kid[k]->num;}
static void eq(const J*c,const char*k,double got,double want,int exact){
 ++checks;double tol=exact?0:jnum(c,"absolute_tolerance",0)+jnum(c,"relative_tolerance",0)*fabs(want);
 if(!isfinite(got)||fabs(got-want)>tol){printf("FAIL %s %s got %.12g want %.12g\n",jstrv(c,"id"),k,got,want);++g_fail;}
}
static void vectors(void){
 J*root=jload("components.json");assert(root);J*cases=jget(root,"cases");assert(cases&&cases->n==72);
 for(int v=0;v<cases->n;v++){
  J*c=cases->kid[v],*i=jget(c,"input"),*e=jget(c,"expected");const char*kind=jstrv(c,"component");
#define EQ(k,x) eq(c,k,x,jnum(e,k,0),0)
#define EX(k,x) eq(c,k,x,jnum(e,k,0),1)
  if(!strcmp(kind,"uniform_curve")){EQ("value",lat_curve(jstrv(i,"curve"),n(i,"x")));}
  else if(!strcmp(kind,"steering_kernel")){
   LatFilter in={.analog=!strcmp(jstrv(i,"device"),"analog"),.raw=n(i,"raw"),.vctl=n(i,"vctl"),.history=n(i,"h"),.rate_history=n(i,"e"),.old_degrees=n(i,"old_degrees"),.previous_limit=n(i,"previous_limit"),.sigma=n(i,"sigma_degrees"),.latch=jbool(i,"latch"),.speed=n(i,"body_speed"),.throttle=n(i,"throttle"),.brake=n(i,"brake"),.handbrake=n(i,"handbrake"),.gain=n(i,"gain"),.dt=n(i,"dt")};
   LatCommand out=lat_filter(&in);EQ("mapped",out.mapped);EQ("limit",out.limit);EQ("angle_degrees_before_quantization",out.degrees);EX("command",out.command);EQ("next_previous_limit",out.previous_limit);eq(c,"latch",out.latch,jbool(e,"latch"),1);
  }else if(!strcmp(kind,"front_geometry")){
   float a[4];lat_geometry((unsigned)n(i,"command"),n(i,"wheelbase"),n(i,"track"),a);
   for(int k=0;k<4;k++)eq(c,"angles",a[k],at(jget(e,k<2?"front_angles":"rear_angles"),k%2),0);
  }else if(!strcmp(kind,"contact_heading_slip")){EX("heading_units",lat_heading(n(i,"vx"),n(i,"vy")));EQ("alpha",lat_slip(n(i,"vx"),n(i,"vy")));}
  else if(!strcmp(kind,"gripping_lateral")){EQ("Fy",lat_grip(n(i,"N"),n(i,"A"),n(i,"D"),n(i,"K"),n(i,"alpha"),n(i,"vx"),n(i,"wT")));}
  else if(!strcmp(kind,"combined_cap")){
   float fx=n(i,"Fx"),fy=n(i,"Fy");lat_cap(&fx,&fy,n(i,"limit"),n(i,"tau"),jbool(i,"locked"),n(i,"vx"),n(i,"alpha"));EQ("Fx",fx);EQ("Fy",fy);
  }else if(!strcmp(kind,"planar_contact_velocity")){
   float velocity[2]={(float)at(jget(i,"body_velocity"),0),(float)at(jget(i,"body_velocity"),1)},arm[2]={(float)at(jget(i,"arm"),0),(float)at(jget(i,"arm"),1)},u[2],vx,vy;
   lat_contact(velocity,n(i,"yaw_rate"),arm,n(i,"wheel_angle"),u,&vx,&vy);EQ("vx",vx);EQ("vy",vy);
   for(int k=0;k<2;k++)eq(c,"centre_velocity",u[k],at(jget(e,"centre_velocity"),k),0);
  }else if(!strcmp(kind,"planar_body_contribution")){
   float f[2]={(float)at(jget(i,"external_force"),0),(float)at(jget(i,"external_force"),1)},m=n(i,"external_moment");J*rows=jget(i,"contact_arm_force_rows_rx_ry_Fx_Fy");
   for(int k=0;k<rows->n;k++){J*r=rows->kid[k];float arm[2]={(float)at(r,0),(float)at(r,1)};lat_accumulate(f,&m,arm,0,(float)at(r,2),(float)at(r,3));}
   for(int k=0;k<2;k++){double a=f[k]/n(i,"mass");eq(c,"acceleration",a,at(jget(e,"acceleration"),k),0);eq(c,"delta_velocity",a*n(i,"h"),at(jget(e,"delta_velocity"),k),0);}
   EQ("yaw_acceleration",m*n(i,"Iz_inverse"));EQ("delta_yaw_rate",m*n(i,"Iz_inverse")*n(i,"h"));
  }else if(!strcmp(kind,"tyre_branch")){eq(c,"sliding",lat_sliding(n(i,"stored_slip"),n(i,"alpha"),n(i,"A"),jbool(i,"new_locked"),n(i,"vx"),n(i,"vy")),jbool(e,"sliding"),1);}
  else {printf("FAIL unhandled %s\n",kind);++g_fail;}
#undef EQ
#undef EX
 }jfree(root);
}
int main(int argc,char**argv){g_dir=argc>1?argv[1]:"docs/cleanroom-driving-physics/stage4/test-vectors";vectors();printf("Stage4: %d checks, %d failures\n",checks,g_fail);return !!g_fail;}
