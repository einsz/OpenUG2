#include <math.h>
#include <string.h>
#include "drive_stage1.h"

static int valid_car(const DrvCar *c) {
    if(!c || !isfinite(c->mass) || c->mass<=0 ||
       !isfinite(c->engine_inertia) || c->engine_inertia<=0 ||
       !isfinite(c->idle_rpm) || c->idle_rpm<=0 ||
       !isfinite(c->shift_rpm) || c->shift_rpm<=c->idle_rpm ||
       !isfinite(c->max_rpm) || c->max_rpm<c->shift_rpm ||
       !isfinite(c->rear_share) || c->rear_share<0 || c->rear_share>1 ||
       !isfinite(c->clutch_k) || c->clutch_k<0 ||
       !isfinite(c->clutch_rate) || c->clutch_rate<=0 ||
       !isfinite(c->launch_fraction) ||
       !isfinite(c->final_front) || c->final_front<=0 ||
       !isfinite(c->final_rear) || c->final_rear<=0 ||
       c->gear_count<1 || c->gear_count>6 ||
       !isfinite(c->ratio[0]) || c->ratio[0]>=0 || c->ratio[1]!=0)return 0;
    for(int k=0;k<4;k++)if(!isfinite(c->wheel_radius[k]) || c->wheel_radius[k]<=0)return 0;
    for(int k=2;k<c->gear_count+2;k++)if(!isfinite(c->ratio[k]) || c->ratio[k]<=0)return 0;
    for(int k=0;k<2;k++)if(!isfinite(c->theta_max[k]) || c->theta_max[k]<0 ||
       !isfinite(c->mu_peak[k]) || c->mu_peak[k]<0 || !isfinite(c->mu_slide[k]) || c->mu_slide[k]<0)return 0;
    for(int k=0;k<9;k++)if(!isfinite(c->torque_primary[k]) || c->torque_primary[k]<0 ||
       !isfinite(c->torque_secondary[k]) || c->torque_secondary[k]<0)return 0;
    for(int k=0;k<3;k++)if(!isfinite(c->engine_brake[k]) || c->engine_brake[k]<0)return 0;
    return 1;
}

int stage1_car_read(const unsigned char *data,long length,const char *name,DrvCar *out) {
    if(!data || !name || !out)return 0;
    long row=n2_global_car_record(data,length,name);
    if(row<0 || row>length-0x890)return 0;
    DrvCar c={0};
#define READ(field,offset) memcpy(&(field),data+row+(offset),sizeof(field))
    READ(c.mass,0x220);
    for(int i=0;i<4;i++){
        int w=0x120+0x30*i;
        if(n2_u32(data+row+w+0x18)!=(unsigned)i)return 0;
        READ(c.wheel_radius[i],w+0x10);
    }
    for(int a=0;a<2;a++){
        int w=0x1e0+0x20*a;
        READ(c.theta_max[a],w);READ(c.mu_peak[a],w+0xc);READ(c.mu_slide[a],w+0x10);
    }
    READ(c.launch_fraction,0x2c0);READ(c.final_front,0x2c8);READ(c.final_rear,0x2cc);
    READ(c.rear_share,0x2d0);READ(c.clutch_k,0x2d4);
    c.gear_count=(int)n2_u32(data+row+0x2d8);READ(c.clutch_rate,0x2dc);
    READ(c.ratio,0x2e0);READ(c.idle_rpm,0x300);READ(c.shift_rpm,0x304);
    READ(c.max_rpm,0x308);READ(c.engine_inertia,0x30c);
    READ(c.torque_primary,0x310);READ(c.torque_secondary,0x334);READ(c.engine_brake,0x358);
#undef READ
    if(!valid_car(&c) || drv_car_finalize(&c))return 0;
    *out=c;return 1;
}

int stage1_drive_init(Stage1Drive *s,const DrvCar *car) {
    if(!s || !valid_car(car))return -1;
    Stage1Drive fresh={0};DrvTune tune;drv_tune_default(&tune);
    tune.is_player=1; /* S1-7.8: this adapter belongs to the human player */
    if(drv_init(&fresh.drivetrain,car,&tune))return -1;
    fresh.ready=1;*s=fresh;return 0;
}
void stage1_drive_reset(Stage1Drive *s,const float velocity[2],float heading) {
    if(!s || !s->ready)return;
    DrvCar car=s->drivetrain.car;
    stage1_drive_init(s,&car);
    float v=(velocity[0]*cosf(heading)+velocity[1]*sinf(heading))*PHYS_TICKRATE;
    for(int k=0;k<4;k++)s->drivetrain.wheel[k].omega=v/car.wheel_radius[k];
}
float stage1_drive_rpm(const Stage1Drive *s){return s->drivetrain.omega_e*DRV_KR;}
int stage1_drive_gear(const Stage1Drive *s){return s->drivetrain.gear-1;}

float stage1_drive_step(Stage1Drive *s,float pos[3],float vel[2],float *heading,
    float *speed,float pedal,float steer,int handbrake,const PhysSurface *sf,
    const PhysVehicle *vh,const PhysRideState *ride) {
    if(!s || !s->ready)return 0;
    if(!sf)sf=&PHYS_SURF_ROAD;
    Drivetrain *d=&s->drivetrain;
    s->wheel_travel=0;
    float ch=cosf(*heading),sh=sinf(*heading);
    float forward=(vel[0]*ch+vel[1]*sh)*PHYS_TICKRATE;
    /* Host keyboard policy; not the unresolved retail menu mapping U-1/U-2. */
    if(pedal<0 && forward<=.3f)s->reverse=1;
    if(pedal>0 && forward>=-.3f)s->reverse=0;
    int braking=(pedal<0 && !s->reverse)||(pedal>0 && s->reverse);
    DrvInput input={.throttle=braking||handbrake?0:fabsf(pedal),
        .brake=braking?fabsf(pedal):0,.handbrake=handbrake?1:0,
        .gear_cmd=s->reverse?DRV_GEAR_R:DRV_CMD_AUTO};
    drv_frame_begin(d,&input,1/PHYS_TICKRATE,-1);
    float h;int n=drv_substep_count(1/PHYS_TICKRATE,&h);
    unsigned mask=ride?ride->contact_mask:0;
    DrvEnv env;drv_env_default(&env);
    /* Explicit provisional Stage 4/5 interface: symmetric longitudinal slice,
       equal static per-wheel preload, ordinary-road surface and rear factor 1.
       Steering/lateral response remains in the host chassis, outside Stage 1. */
    env.rear_grip_multiplier=1;
    for(int k=0;k<4;k++){
        env.wheel[k].contact=(mask>>k)&1u;
        env.wheel[k].load=env.wheel[k].contact?d->car.mass*9.8f/4:0;
        env.wheel[k].mu_peak_surface=env.wheel[k].mu_slide_surface=sf->accel;
        env.wheel[k].locked=handbrake;
    }
    /* Body external-force input for the coupling passes. The next derivative
       consumes the prior coupling forces, as required by S1-2.4. */
    DrvBodyStraight body={.mass=d->car.mass};
    for(int k=0;k<n;k++){
        d->env=env; /* current host contact/lock flags before wheel derivatives */
        float force=0;
        for(int i=0;i<4;i++)force+=env.wheel[i].contact&&!handbrake?d->force_long[i]:0;
        float drag=mask?(powf(sf->drag,h*PHYS_TICKRATE)-1)*forward/h:0;
        float decel=mask && braking && !handbrake
            ?10.5f*g_phys_tune.brake*(vh&&vh->brake>0?vh->brake:1)*d->brake:0;
        float before=forward;
        body.ex=d->car.mass*(drag-copysignf(decel,forward));
        forward+=h*(force/d->car.mass+drag);
        if(decel>0)forward=copysignf(fmaxf(0,fabsf(forward)-decel*h),forward);
        if(decel>0 && before*forward<0)forward=0;
        /* Locked-wheel forces are not added on top of the host handbrake.
           The shared chassis applies its existing 5.5 m/s^2 stop once. */
        for(int i=0;i<4;i++)s->wheel_travel+=h*d->wheel[i].omega*.25f;
        drv_integrate(d,h);
        for(int i=0;i<4;i++){
            env.wheel[i].vx=forward;
            /* Service-brake wheel torque is deferred. Host brake adapter keeps
               supported wheels rolling; handbrake holds shaft/wheels at zero. */
            if(env.wheel[i].contact && (braking||handbrake)){
                d->wheel[i].omega=handbrake?0:forward/d->car.wheel_radius[i];
                d->wheel[i].tau_smooth=0;
            }
        }
        env.body_speed=fabsf(forward);
        drv_couple(d,&env,drv_body_accel_straight,&body);
    }
    float old=(vel[0]*ch+vel[1]*sh)*PHYS_TICKRATE;
    vel[0]+=ch*(forward-old)/PHYS_TICKRATE;vel[1]+=sh*(forward-old)/PHYS_TICKRATE;
    if(mask)return phys_stage1_chassis_step(pos,vel,heading,speed,steer,handbrake,sf,vh);
    pos[0]+=vel[0];pos[1]+=vel[1];*speed=forward/PHYS_TICKRATE;
    return fabsf(vel[0]*sh-vel[1]*ch);
}
