/* Every approved Stage-3 component vector, plus composition boundaries. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "braking.h"
#include "drivetrain.h"
static const char *g_dir;
static int g_fail,checks;
#include "test_json.h"
static float num(const J *j,const char *key){return (float)jnum(j,key,0);}
static void eq(const char *id,const char *field,double got,double want){
    double tolerance=fmax(!strcmp(field,"angular_acceleration")?.0001:.00001,.00001*fabs(want));
    ++checks;if(!isfinite(got)||fabs(got-want)>tolerance){
        printf("FAIL %s %s got %.12g expected %.12g\n",id,field,got,want);++g_fail;
    }
}
static void exact(const char *id,const char *field,int got,int want){
    ++checks;if(got!=want){printf("FAIL %s %s got %d expected %d\n",id,field,got,want);++g_fail;}
}
static int direction(const char *s){return !strcmp(s,"forward")?1:!strcmp(s,"backward")?-1:0;}
static void vectors(void){
    J *root=jload("braking.json");assert(root);J *all=jget(root,"cases");assert(all&&all->n==54);
    for(int k=0;k<all->n;k++){
        J *c=all->kid[k],*i=jget(c,"inputs"),*e=jget(c,"expected");
        const char *id=jstrv(c,"id"),*kind=jstrv(c,"component");
        if(!strcmp(kind,"pedals")){
            BrkPedals p={num(i,"u"),num(i,"b"),num(i,"hb")};brk_pedals(&p,jbool(i,"exclusivity"));
            eq(id,"u",p.throttle,jnum(e,"u",0));eq(id,"b",p.service,jnum(e,"b",0));eq(id,"hb",p.handbrake,jnum(e,"hb",0));
        }else if(!strcmp(kind,"distribution")){
            BrkCar car={.service=num(i,"Kb"),.rear_share=num(i,"beta")};
            BrkPedals p={.throttle=num(i,"u"),.service=num(i,"b")};BrkFrame f;
            brk_distribute(&car,&p,num(i,"vmph"),&f);
            for(int w=0;w<4;w++){
                eq(id,"coefficient",f.coefficient[w],jget(e,"coefficients")->kid[w]->num);
                eq(id,"magnitude",f.service[w],jget(e,"service_magnitudes")->kid[w]->num);
            }
        }else if(!strcmp(kind,"preparation")){
            BrkInput in={.radius=num(i,"R"),.omega=num(i,"omega"),.vx=num(i,"vx"),.vy=num(i,"vy"),
                .load=num(i,"N"),.mu=num(i,"mu"),.service=num(i,"B"),.handbrake=num(i,"H"),
                .tau=num(i,"tau_initial"),.locked=jbool(i,"locked"),.relief_disabled=jbool(i,"relief_disabled")};
            BrkResult r=brk_prepare(&in);
            eq(id,"slip",r.slip,jnum(e,"slip",0));eq(id,"tau",r.tau,jnum(e,"tau",0));
            eq(id,"relief_scale",r.relief_scale,jnum(e,"relief_scale",0));eq(id,"omega",r.omega,jnum(e,"omega",0));
            exact(id,"locked",r.locked,jbool(e,"locked"));exact(id,"decision",r.decision,jbool(e,"decision_evaluated"));
            for(int n=0;n<3;n++){
                const char *field=n==0?"Q":n==1?"A":"C";J *v=jget(e,field);
                exact(id,"diagnostic available",r.decision,v->t!=J_NULL);
                if(v->t!=J_NULL)eq(id,field,n==0?r.q:n==1?r.a:r.c,v->num);
            }
            /* Exercise actual tyre preparation at the vector's specified boundary. */
            Drivetrain d={0};d.car.wheel_radius[0]=in.radius;d.car.theta_max[0]=12;d.car.mu_slide[0]=in.mu;
            d.tune.grip_q=1;d.env.body_speed=20;d.env.rear_grip_multiplier=1;
            d.env.wheel[0]=(DrvWheelEnv){.vx=in.vx,.vy=in.vy,.load=in.load,.mu_slide_surface=1,.locked=in.locked};
            d.wheel[0]=(DrvWheel){.omega=in.omega,.tau=in.tau,.w_long=1,.w_lat=1};
            d.brakes.enabled=1;d.brakes.locked[0]=in.locked;d.brakes.relief_disabled=in.relief_disabled;
            d.brakes.frame.service[0]=in.service;d.brakes.handbrake[0]=in.handbrake;
            drv_prepare_tyre(&d,0);
            eq(id,"initial_longitudinal_force",d.wheel[0].force,jnum(e,"initial_longitudinal_force",0));
            exact(id,"sliding",!d.wheel[0].gripping,jbool(e,"sliding"));
            eq(id,"stored slip",d.wheel[0].slip_speed,jnum(e,"slip",0));
            exact(id,"stored lock",d.env.wheel[0].locked,jbool(e,"locked"));
        }else if(!strcmp(kind,"lock_comparison")){
            float omega=num(i,"omega");int locked=1;brk_compare(num(i,"A"),num(i,"C"),&omega,&locked);
            eq(id,"omega",omega,jnum(e,"omega",0));exact(id,"locked",locked,jbool(e,"locked"));
        }else if(!strcmp(kind,"direction_post")){
            float omega=num(i,"integrated_omega");int dir=direction(jstrv(i,"old_dir"));brk_direction_post(&omega,&dir);
            eq(id,"omega",omega,jnum(e,"omega",0));exact(id,"dir",dir,direction(jstrv(e,"dir")));
        }else if(!strcmp(kind,"wheel_derivative")){
            Drivetrain d={0};d.car.wheel_radius[0]=num(i,"R");
            d.wheel[0]=(DrvWheel){.tau=num(i,"tau"),.force=num(i,"force"),.omega=num(i,"omega"),
                .tau_smooth=num(i,"smoothed_previous"),.slip_speed=num(i,"slip"),.gripping=jbool(i,"gripping")};
            d.env.wheel[0]=(DrvWheelEnv){.locked=jbool(i,"locked"),.roll_drag=num(i,"rho"),.drag_flag=(int)num(i,"damage")};
            double acceleration=drv_wheel_derivative(&d,0,num(i,"h"));
            eq(id,"angular_acceleration",acceleration,jnum(e,"angular_acceleration",0));
            eq(id,"smoothed",d.wheel[0].tau_smooth,jnum(e,"smoothed",0));
        }else{printf("FAIL unknown component %s\n",kind);++g_fail;}
    }jfree(root);
}
/* Additional independent supplied-state boundaries, not retail trajectories. */
static Drivetrain fixture(void) {
    DrvCar c={.mass=1,.engine_inertia=1,.gear_count=1,.idle_rpm=1000,.max_rpm=7000,
        .shift_rpm=6000,.rear_share=1,.final_front=4,.final_rear=4};
    c.ratio[0]=-3;c.ratio[2]=3;
    for(int i=0;i<4;i++)c.wheel_radius[i]=.5f;
    for(int i=0;i<2;i++){c.mu_peak[i]=1;c.mu_slide[i]=1;c.theta_max[i]=12;}
    for(int i=0;i<9;i++)c.torque_primary[i]=.3f;
    DrvTune tune;drv_tune_default(&tune);Drivetrain d;
    assert(drv_init(&d,&c,&tune)==0);
    const BrkCar car={.service=.5f,.handbrake=.75f,.rear_share=.25f};
    assert(drv_brakes_supply(&d,&car,0,0,1)==0);
    d.env.body_speed=5;
    for(int i=0;i<4;i++){d.env.wheel[i].load=4;d.env.wheel[i].vx=5;d.wheel[i].omega=10;}
    return d;
}
static void boundaries(void) {
    const BrkCar hand={.handbrake=.75f};const float eta[4]={0,0,.5f,1};float magnitude[4];
    brk_handbrake(&hand,eta,magnitude);
    for(int i=0;i<4;i++)eq("injected handbrake coefficient","H",magnitude[i],7.5*eta[i]);
    Drivetrain d=fixture();DrvEnv env=d.env;
    drv_couple(&d,&env,NULL,NULL);Drivetrain before=d;
    DrvInput pedal={.brake=1,.gear_cmd=DRV_GEAR_N};drv_frame_begin(&d,&pedal,1.0f/60,-1);
    eq("pedal lifecycle","prepared torque",d.wheel[0].tau,before.wheel[0].tau);
    eq("pedal lifecycle","prepared force",d.wheel[0].force,before.wheel[0].force);
    eq("pedal lifecycle","first derivative",drv_wheel_derivative(&d,0,.004f),drv_wheel_derivative(&before,0,.004f));
    drv_integrate(&d,.004f);drv_couple(&d,&env,NULL,NULL);
    exact("pedal lifecycle","subsequent brake torque",d.wheel[0].tau<0,1);
    /* A single frame distribution uses full speed and retains its coefficients
     * while coupling velocities subsequently change. No extra pedal curve. */
    d=fixture();d.env.body_speed=2;pedal=(DrvInput){.throttle=1,.brake=1,.gear_cmd=1};
    drv_frame_begin(&d,&pedal,.016f,-1);BrkFrame frame=d.brakes.frame;
    eq("full speed snapshot","rear",frame.coefficient[2],2*(double)BRK_MPH*.05f);
    env=d.env;env.body_speed=100;drv_couple(&d,&env,NULL,NULL);
    exact("held coefficients","unchanged",memcmp(&frame,&d.brakes.frame,sizeof frame)==0,1);
    d=fixture();d.brakes.exclusivity=0;pedal=(DrvInput){.throttle=.1f,.brake=.1f,.gear_cmd=1};
    drv_frame_begin(&d,&pedal,.016f,-1);
    eq("engine deadzone","engine",d.throttle,0);eq("engine deadzone","brake",d.brake,.1f);
    eq("engine deadzone","front request",d.brakes.frame.service[0],.3f);
    /* Newly locked player rear wheels use pre-decision kv for sliding, then
     * current lock/spin for the independent final cap. */
    d=fixture();d.tune.is_player=1;d.brakes.car.rear_share=1;
    pedal=(DrvInput){.brake=1,.gear_cmd=1};drv_frame_begin(&d,&pedal,.016f,-1);
    d.wheel[2].omega=2;d.wheel[2].w_long=2;env=d.env;
    drv_couple(&d,&env,NULL,NULL);
    exact("new rear lock","locked",d.brakes.locked[2],1);
    eq("new rear lock","stored slip",d.wheel[2].slip_speed,-4);
    eq("new rear lock","final cap uses new lock",d.force_long[2],-4);
    /* Conversely an incoming lock can release while its sliding force uses
     * the incoming multiplier; the final cap uses the unlocked multiplier. */
    d=fixture();d.tune.is_player=1;
    assert(drv_brakes_supply(&d,&d.brakes.car,4,0,1)==0);
    d.wheel[2].omega=0;env=d.env;drv_couple(&d,&env,NULL,NULL);
    exact("rear release","unlocked",d.brakes.locked[2],0);
    eq("rear release","new cap",d.force_long[2],-3.4);
    /* Incoming lock skips wheel drive without skipping the engine reaction. */
    d=fixture();assert(drv_brakes_supply(&d,&d.brakes.car,15,0,1)==0);
    d.gear=d.target_gear=2;d.mode=DRV_CLUTCH_LOCKED;d.throttle=.5f;d.throttle_mult=1;
    d.brakes.car.service=10;pedal=(DrvInput){.throttle=.5f,.brake=1,.gear_cmd=2};
    drv_frame_begin(&d,&pedal,.016f,-1);env=d.env;
    for(int i=0;i<4;i++){d.wheel[i].omega=0;env.wheel[i].load=0;}
    double te=drv_engine_torque(&d);drv_couple(&d,&env,NULL,NULL);
    eq("locked drivetrain","engine reaction",d.feedback,-te);
    for(int i=0;i<4;i++)eq("locked drivetrain","no distributed wheel torque",d.wheel[i].tau,0);
    /* A zero-time post step is not an extra Euler integration and retains
     * smoothed torque. Direction crossing is unconditional. */
    d=fixture();d.wheel[0].dir=1;d.wheel[0].omega=-1;d.wheel[0].tau_smooth=3;
    d.wheel[0].tau=11;drv_post(&d);
    eq("post sweep","spin",d.wheel[0].omega,0);eq("post sweep","torque clear",d.wheel[0].tau,0);
    eq("post sweep","smooth retained",d.wheel[0].tau_smooth,3);
    BrkCar invalid={.service=-1};exact("invalid supply","rejected",drv_brakes_supply(&d,&invalid,0,0,1),-1);
}
int main(int argc,char **argv){
    g_dir=argc>1?argv[1]:"docs/cleanroom-driving-physics/stage3/test-vectors";
    (void)hexkey;vectors();boundaries();printf("Stage 3: %d checks, %d failures\n",checks,g_fail);return g_fail?1:0;
}
