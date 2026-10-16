/* Approved Stage-2 synthetic vectors and independent boundary checks. */
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "resistance.h"
#include "drivetrain.h"
static const char *g_dir;
static int g_fail,checks;
#include "test_json.h"
static void eq(const char *id,const char *key,double got,double want) {
    ++checks;
    if(!isfinite(got)||fabs(got-want)>2e-6+2e-6*fabs(want)) {
        printf("FAIL %s %s: %.10g expected %.10g\n",id,key,got,want);++g_fail;
    }
}
static void vec(const J *j,const char *key,float v[3]) {
    J *a=jget(j,key);assert(a&&a->n==3);
    for(int k=0;k<3;k++)v[k]=(float)a->kid[k]->num;
}
static void ev(const char *id,const J *j,const char *key,const float v[3]) {
    float want[3];vec(j,key,want);for(int k=0;k<3;k++)eq(id,key,v[k],want[k]);
}
static ResInput inputs(const J *j) {
    ResInput i={0};i.scale=(float)jnum(j,"G",1);i.reduction=(float)jnum(j,"a",0);
    i.enable=(float)jnum(j,"e",1);i.surface=(float)jnum(j,"d",1);
    i.throttle=(float)jnum(j,"t",0);i.contacts=(int)jnum(j,"n",4);
    i.bonus=jbool(j,"bonus");i.alternate=jbool(j,"j");i.stabilize=jbool(j,"stabilize");return i;
}
static ResCar parameters(const J *j) {
    ResCar c={(float)jnum(j,"cD",0),(float)jnum(j,"cF",0),
        (float)jnum(j,"cR",0),(float)jnum(j,"L",0)};return c;
}
static ResBody body(const J *j) {
    ResBody b={0};b.mass=(float)jnum(j,"m",1);b.z=(float)jnum(j,"z",0);
    if(jget(j,"P"))vec(j,"P",b.momentum);
    else if(jget(j,"V")) {
        vec(j,"V",b.velocity);
        for(int k=0;k<3;k++)b.momentum[k]=b.mass*b.velocity[k];
    }
    if(jget(j,"F"))vec(j,"F",b.force);
    return b;
}
static void cases(void) {
    J *root=jload("s2_cases.json");assert(root);J *all=jget(root,"cases");assert(all&&all->n==52);
    for(int index=0;index<all->n;index++) {
        J *c=all->kid[index],*i=jget(c,"input"),*e=jget(c,"expected");
        const char *id=jstrv(c,"id"),*kind=jstrv(c,"kind");
        ResInput in=inputs(i);ResCar car=parameters(i);ResBody b=body(i);
        ResBasis basis=res_basis_level(0);
        if(!strcmp(kind,"drag")) {
            float v[3],want[3];double f[3];vec(i,"V",v);res_drag(&car,&in,v,f);vec(e,"D",want);
            for(int k=0;k<3;k++)eq(id,"D",f[k],want[k]);
        } else if(!strcmp(kind,"vertical_aero")) {
            J *a=jget(i,"B");
            for(int k=0;k<3;k++) {
                basis.forward[k]=(float)a->kid[0]->kid[k]->num;
                basis.lateral[k]=(float)a->kid[1]->kid[k]->num;
                basis.up[k]=(float)a->kid[2]->kid[k]->num;
            }
            ResAero out;res_prepare_aero(&b,&car,&in,&basis,&out);
            eq(id,"WF",out.front_load,jnum(e,"WF",0));eq(id,"WR",out.rear_load,jnum(e,"WR",0));
            eq(id,"reported",out.load,jnum(e,"reported",0));
            ev(id,e,"applied_front",out.front);ev(id,e,"applied_rear",out.rear);
        } else if(!strcmp(kind,"rolling_torque")) {
            Drivetrain d={0};d.car.idle_rpm=1000;d.car.shift_rpm=6000;d.car.max_rpm=7000;
            d.car.engine_inertia=1;d.car.mass=1;d.gear=d.target_gear=1;
            d.mode=DRV_CLUTCH_OPEN;d.car.wheel_radius[0]=1;
            d.env.wheel[0].contact=jbool(i,"contact");d.env.wheel[0].roll_drag=(float)jnum(i,"rho",0);
            d.env.wheel[0].drag_flag=(int)jnum(i,"damage",0);
            d.wheel[0].omega=(float)jnum(i,"omega",0);
            double torque=drv_wheel_resistance(d.wheel[0].omega,d.env.wheel[0].roll_drag,d.env.wheel[0].drag_flag);
            eq(id,"Troll",torque,jnum(e,"Troll",0));
            float before=d.wheel[0].omega;drv_integrate(&d,1);
            eq(id,"omega_dot",d.wheel[0].omega-before,jnum(e,"omega_dot_without_slip_correction",0));
            eq(id,"unsmoothed",d.wheel[0].tau_smooth,0);
        } else if(!strcmp(kind,"limiter_local_force")) {
            float f[3];vec(i,"force_local",f);ResLimit out;
            res_limit_local(&car,&in,(float)jnum(i,"horizontal_speed",0),(float)jnum(i,"forward_z",0),f,&out);
            eq(id,"threshold",out.threshold_mph,jnum(e,"threshold_mph",0));
            eq(id,"measured",out.measured_mph,jnum(e,"measured_mph",0));
            eq(id,"activated",out.activated,jbool(e,"activated"));ev(id,e,"force_local",f);
        } else if(!strcmp(kind,"stabilization")) {
            ResDerivative out;res_derivative(&b,&car,&in,&basis,(float)jnum(i,"w",0),&out);
            eq(id,"curve",out.curve,jnum(e,"curve",0));eq(id,"zdot",out.zdot,jnum(e,"zdot",0));
            ev(id,e,"Pdot",out.momentum);res_integrate(&b,&out,(float)jnum(i,"h",0));res_post(&b,0);
            eq(id,"z_post",b.z,jnum(e,"z_post",0));
        } else if(!strcmp(kind,"absolute_cap")) {
            int activated=res_post(&b,0);eq(id,"activated",activated,jbool(e,"activated"));
            ev(id,e,"P",b.momentum);ev(id,e,"V",b.velocity);
        } else if(!strcmp(kind,"prepared_force_euler")) {
            vec(i,"prepared_wheel_force",b.force);ResAero aero;
            res_prepare_aero(&b,&car,&in,&basis,&aero);float want[3];vec(e,"prepared_aero",want);
            for(int k=0;k<3;k++)eq(id,"prepared_aero",aero.drag[k],want[k]);
            ResDerivative out;res_derivative(&b,&car,&in,&basis,0,&out);ev(id,e,"Pdot",out.momentum);
            res_integrate(&b,&out,(float)jnum(i,"h",0));ev(id,e,"P_after",b.momentum);
        } else { printf("FAIL unknown kind %s\n",kind);++g_fail; }
    }
    jfree(root);
}
static void boundaries(void);
int main(int argc,char **argv) {
    g_dir=argc>1?argv[1]:"docs/cleanroom-driving-physics/stage2/test-vectors";
    (void)hexkey;cases();boundaries();printf("Stage 2: %d checks, %d failures\n",checks,g_fail);return g_fail?1:0;
}
/* Additional invented inputs exercise approved rules with no supplied vectors. */
static void boundaries(void) {
    ResBasis basis=res_basis_level(0);ResInput in={.scale=1,.enable=1,.surface=1,.contacts=4};
    ResCar car={0};ResBody b={.mass=1};ResAero a;
    car.drag=FLT_MAX;b.velocity[0]=10;b.velocity[1]=20;
    res_prepare_aero(&b,&car,&in,&basis,&a);
    eq("huge drag","direction",b.force[1]/b.force[0],2);
    eq("huge drag","cap",sqrt(res_dot(b.force,b.force)),1200);
    eq("huge drag","moment",b.moment[1],100/sqrt(5.0));
    assert(isfinite(a.drag[0])&&isfinite(a.drag[1]));
    b=(ResBody){.mass=1};b.velocity[0]=10;car=(ResCar){.front=FLT_MAX,.rear=FLT_MAX};
    res_prepare_aero(&b,&car,&in,&basis,&a);
    assert(isfinite(a.front_load)&&isfinite(a.rear_load)&&isfinite(a.load));
    eq("huge aero","separate caps",b.force[2],-2400);
    eq("huge aero","balanced moment",b.moment[1],0);
    b=(ResBody){.mass=1};car=(ResCar){0};
    car.drag=100;b.velocity[0]=100;
    res_prepare_aero(&b,&car,&in,&basis,&a);
    eq("point caps","drag",b.force[0],-1200);eq("point caps","moment",b.moment[1],100);
    eq("point caps","raw",a.drag[0],-2000000);
    b=(ResBody){.mass=1};b.velocity[0]=100;car=(ResCar){.front=100,.rear=100};
    res_prepare_aero(&b,&car,&in,&basis,&a);
    eq("separate caps","sum",b.force[2],-2400);eq("separate caps","moment cancel",b.moment[1],0);
    /* A tilted basis rotates body-up aero into XY; projection on f stays zero. */
    basis=(ResBasis){{.8f,0,.6f},{0,1,0},{-.6f,0,.8f}};
    b=(ResBody){.mass=1};b.velocity[0]=8;b.velocity[2]=6;car=(ResCar){.front=.1f,.rear=.2f};
    res_prepare_aero(&b,&car,&in,&basis,&a);
    eq("slope aero","world x",b.force[0],1.8);eq("slope aero","world z",b.force[2],-2.4);
    eq("slope aero","forward projection",res_dot(b.force,basis.forward),0);
    /* Low-contact orientation suppression and world-relative drag arm. */
    in.contacts=1;b=(ResBody){.mass=1};b.velocity[0]=8;b.velocity[2]=6;
    res_prepare_aero(&b,&car,&in,&basis,&a);eq("one contact","load",a.load,2.4);
    in.contacts=0;b=(ResBody){.mass=1};b.velocity[0]=8;b.velocity[2]=6;
    res_prepare_aero(&b,&car,&in,&basis,&a);eq("no contact","load",a.load,1.944);
    basis.up[2]=-1;b=(ResBody){.mass=1};b.velocity[0]=8;b.velocity[2]=6;
    res_prepare_aero(&b,&car,&in,&basis,&a);eq("inverted air","load",a.load,0);
    /* Strict low-speed fallback, and per-application arm/contact boundary. */
    basis=(ResBasis){{.8f,0,.6f},{0,1,0},{-.6f,0,.8f}};
    in.contacts=4;car=(ResCar){.front=10000};b=(ResBody){.mass=1};b.velocity[1]=.0001f;
    res_prepare_aero(&b,&car,&in,&basis,&a);eq("fallback equality","WF",a.front_load,.64);
    b=(ResBody){.mass=1};b.velocity[1]=nextafterf(.0001f,1);
    res_prepare_aero(&b,&car,&in,&basis,&a);eq("above fallback","WF",a.front_load,.4);
    basis=res_basis_level(0);car=(ResCar){.drag=.5f};in.contacts=1;
    b=(ResBody){.mass=1};b.velocity[0]=10;res_prepare_aero(&b,&car,&in,&basis,&a);
    eq("one contact arm","moment",b.moment[1],0);
    in.contacts=2;b=(ResBody){.mass=1};b.velocity[0]=10;res_prepare_aero(&b,&car,&in,&basis,&a);
    eq("two contact arm","moment",b.moment[1],10);
    /* World limiter preserves supplied lateral/up components on a slope. */
    basis=(ResBasis){{.8f,0,.6f},{0,1,0},{-.6f,0,.8f}};
    car=(ResCar){.limit_mph=10};in.scale=1;
    b=(ResBody){.mass=1};b.velocity[0]=100;b.force[0]=-.8f;b.force[1]=3;b.force[2]=4.4f;
    ResDerivative sloped;res_derivative(&b,&car,&in,&basis,0,&sloped);
    eq("world limiter","forward",res_dot(b.force,basis.forward),-1.5);
    eq("world limiter","lateral",res_dot(b.force,basis.lateral),3);
    eq("world limiter","up",res_dot(b.force,basis.up),4);
    b=(ResBody){.mass=1};b.velocity[2]=1000;b.force[0]=2;
    res_derivative(&b,&car,&in,&basis,0,&sloped);eq("horizontal measure","unchanged",b.force[0],2);
    /* Scalar mode mapping uses only contacting body-surface factors. */
    float factors[4]={2,6,10,14};in.enable=.25f;in.surface=3;in.stabilize=0;
    res_frame_inputs(&in,3,0,0,5,factors);eq("retained mode","enable",in.enable,.25);eq("retained mode","surface",in.surface,3);
    res_frame_inputs(&in,0,0,0,5,factors);eq("ordinary mode","surface",in.surface,6);eq("ordinary mode","contacts",in.contacts,2);
    eq("ordinary mode","stabilize",in.stabilize,1);
    res_frame_inputs(&in,2,1,1,0,factors);eq("alternate movement","enable",in.enable,0);
    eq("exception","stabilize",in.stabilize,0);eq("air surface","surface",in.surface,1);
    res_frame_inputs(&in,1,0,0,15,factors);eq("state one","surface",in.surface,1);eq("state one","enable",in.enable,0);
    /* Exact threshold equality: both predicates remain strict. */
    car=(ResCar){.limit_mph=0};in=(ResInput){.scale=1};float force[3]={2,3,4};ResLimit limit;
    res_limit_local(&car,&in,0,0,force,&limit);eq("L zero","force",force[0],2);
    car.limit_mph=2.236024856567383f-1.9f;in.scale=1;
    force[0]=2;res_limit_local(&car,&in,1,0,force,&limit);eq("equal speed predicate","activated",limit.activated,0);
    force[0]=0;res_limit_local(&car,&in,100,0,force,&limit);eq("zero forward predicate","activated",limit.activated,0);
    /* Curve endpoint and each exact mathematical knot (binary32 input tolerance). */
    const float values[8]={1.5f,1,.6f,.22f,.06f,.03f,.015f,0};
    for(int k=0;k<8;k++)eq("curve knot","C",res_stabilization_curve((double)k/7),values[k]);
    eq("curve below","C",res_stabilization_curve(-1),1.5);eq("curve above","C",res_stabilization_curve(2),0);
    b=(ResBody){.mass=1,.z=2};b.force[0]=7;ResDerivative derivative;
    basis=res_basis_level(0);res_derivative(&b,&car,&in,&basis,0,&derivative);
    eq("z above curve","zdot",derivative.zdot,-10);res_integrate(&b,&derivative,.01f);res_post(&b,1);
    eq("no upper z clamp","z",b.z,1.9);eq("post saved","force",b.saved_force[0],7);
    eq("gravity seed","Fz",b.force[2],-9.8);eq("post cleared","Fx",b.force[0],0);
}
