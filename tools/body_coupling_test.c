/* Approved BC-1..3 synthetic arithmetic vectors; no original data. */
#include <math.h>
#include <stdio.h>
#include "drivetrain.h"
#include "body_response.h"
static const char *g_dir="docs/cleanroom-driving-physics/body-coupling/test-vectors";
static int g_fail;
#include "test_json.h"
static float number(const J *o,const char *k){return (float)jnum(o,k,0);}
static void array(const J *a,float *out,int n){
 if(!a||a->t!=J_ARR||a->n!=n){fprintf(stderr,"invalid vector array\n");g_fail++;return;}
 for(int k=0;k<n;k++)out[k]=(float)a->kid[k]->num;
}
static void equal(const char *id,const char *field,float actual,float expected){
 if(!isfinite(actual)||fabsf(actual-expected)>1e-6f*fmaxf(1,fabsf(expected))){fprintf(stderr,"FAIL %s %s: %.9g expected %.9g\n",id,field,actual,expected);g_fail++;}
}
static void vectors(void){
 J *root=jload("provisional.json"),*rows=jget(root,"vectors");
 if(!rows||rows->t!=J_ARR||rows->n!=7){fprintf(stderr,"expected seven approved vectors\n");g_fail++;jfree(root);return;}
 for(int v=0;v<rows->n;v++){
  const J *row=rows->kid[v],*in=jget(row,"input"),*out=jget(row,"expected");const char *id=jstrv(row,"id");
  BodyCar car={.inertia={1,1,number(in,"Iz")},.yaw_scale=number(in,"kI")};
  BodyMotion motion={.adjusted=(int)number(in,"s")};
  J *columns=jget(in,"B_columns");float *axes[3]={motion.axes.forward,motion.axes.lateral,motion.axes.up};
  for(int k=0;k<3;k++)array(columns->kid[k],axes[k],3);
  ResBody body={.mass=number(in,"M")};array(jget(in,"Fprepared_world"),body.force,3);array(jget(in,"Tprepared_world"),body.moment,3);array(jget(in,"held_Omega_world"),motion.omega,3);
  float expected_local[3];array(jget(out,"e_local"),expected_local,3);
  float local[3];for(int k=0;k<3;k++){local[k]=(float)res_dot(axes[k],body.force);equal(id,"e_local",local[k],expected_local[k]);}
  DrvBodyPlanar coupling={.mass=body.mass,.inv_iz=body_inverse_yaw(&motion,&car),.ex=local[0],.ey=local[1],.external_moment=(float)res_dot(body.moment,motion.axes.up)};
  equal(id,"inverse_selected_yaw",coupling.inv_iz,number(out,"inverse_selected_yaw"));
  float force[4],lateral[4];J *wheels=jget(in,"wheels");
  for(int i=0;i<4;i++){
   J *w=wheels->kid[i];float h[3],l[3];array(jget(w,"mount_xy"),coupling.pos[i],2);array(jget(w,"heading"),h,3);array(jget(w,"lateral"),l,3);
   for(int k=0;k<2;k++){coupling.head[i][k]=h[k];coupling.lat[i][k]=l[k];}
   force[i]=number(w,"Fx");lateral[i]=number(w,"Fy");
   /* Supplied normal load is intentionally absent from this planar boundary. */
  }
  float components[3],acceleration[4],expected[4];
  drv_body_planar_acceleration(&coupling,force,lateral,components);
  equal(id,"ax",components[0],number(out,"ax"));equal(id,"ay",components[1],number(out,"ay"));equal(id,"alpha",components[2],number(out,"alpha"));
  drv_body_accel_planar(&coupling,force,lateral,acceleration);array(jget(out,"Ai"),expected,4);
  for(int i=0;i<4;i++)equal(id,"Ai",acceleration[i],expected[i]);
 }
 jfree(root);
}
int main(int argc,char **argv){if(argc>1)g_dir=argv[1];vectors();if(g_fail)return 1;puts("BC-1..3: seven approved body-coupling vectors passed");return 0;}
