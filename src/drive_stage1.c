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

int stage2_car_read(const unsigned char *data,long length,const char *name,ResCar *out) {
    if(!data || !name || !out)return 0;
    long row=n2_global_car_record(data,length,name);
    if(row<0 || row>length-0x890)return 0;
    ResCar c;
    memcpy(&c.drag,data+row+0x370,4);memcpy(&c.front,data+row+0x374,4);
    memcpy(&c.rear,data+row+0x378,4);memcpy(&c.limit_mph,data+row+0x3a0,4);
    if(!isfinite(c.drag)||c.drag<0||!isfinite(c.front)||c.front<0||
       !isfinite(c.rear)||c.rear<0||!isfinite(c.limit_mph)||c.limit_mph<0)return 0;
    *out=c;return 1;
}
int stage1_drive_init(Stage1Drive *s,const DrvCar *car,const ResCar *resistance,float reduction) {
    if(!s || !valid_car(car) || !resistance || !isfinite(reduction)||reduction<0||reduction>1)return -1;
    const float fields[]={resistance->drag,resistance->front,resistance->rear,resistance->limit_mph};
    for(unsigned k=0;k<sizeof fields/sizeof *fields;k++)if(!isfinite(fields[k])||fields[k]<0)return -1;
    Stage1Drive fresh={0};DrvTune tune;drv_tune_default(&tune);
    tune.is_player=1;
    if(drv_init(&fresh.drivetrain,car,&tune))return -1;
    fresh.resistance_car=*resistance;
    fresh.resistance_input=(ResInput){.scale=1,.reduction=reduction,.enable=1,.surface=1,.stabilize=1};
    for(int k=0;k<4;k++)fresh.body_surface[k]=fresh.rolling_surface[k]=1;
    fresh.body.mass=car->mass;res_post(&fresh.body,0);
    fresh.ready=1;*s=fresh;return 0;
}
void stage1_drive_reset(Stage1Drive *s,const float velocity[2],float heading) {
    if(!s || !s->ready)return;
    /* Keep supplied tuning/environment while resetting dynamic state. */
    Stage1Drive previous=*s;
    stage1_drive_init(s,&previous.drivetrain.car,&previous.resistance_car,previous.resistance_input.reduction);
    s->resistance_input=previous.resistance_input;
    memcpy(s->body_surface,previous.body_surface,sizeof s->body_surface);
    memcpy(s->rolling_surface,previous.rolling_surface,sizeof s->rolling_surface);
    s->damage_mask=previous.damage_mask;s->state_code=previous.state_code;
    s->alternate_movement=previous.alternate_movement;s->disable_stabilize=previous.disable_stabilize;
    float v=(velocity[0]*cosf(heading)+velocity[1]*sinf(heading))*PHYS_TICKRATE;
    for(int k=0;k<4;k++)s->drivetrain.wheel[k].omega=v/s->drivetrain.car.wheel_radius[k];
    for(int k=0;k<2;k++)s->body.momentum[k]=s->body.mass*velocity[k]*PHYS_TICKRATE;
    res_post(&s->body,0);
}
float stage1_drive_rpm(const Stage1Drive *s){return s->drivetrain.omega_e*DRV_KR;}
int stage1_drive_gear(const Stage1Drive *s){return s->drivetrain.gear-1;}

/* Planar adapter: the independent suspension supplies the body-up constraint.
 * Keep vertical aero load/moments as diagnostics; no invented load feedback. */
static void prepare(Stage1Drive *s,DrvEnv *env,const ResBasis *basis,float brake_force) {
    Drivetrain *d=&s->drivetrain;
    res_prepare_aero(&s->body,&s->resistance_car,&s->resistance_input,basis,&s->aero);
    s->body.force[2]=0;
    DrvBodyStraight coupling={.mass=d->car.mass,
        .ex=(float)res_dot(s->body.force,basis->forward)+brake_force};
    drv_couple(d,env,drv_body_accel_straight,&coupling);
    memset(s->prepared_wheel,0,sizeof s->prepared_wheel);
    for(int i=0;i<4;i++)if(env->wheel[i].contact&&!env->wheel[i].locked)
        for(int k=0;k<2;k++) {
            s->prepared_wheel[i][k]=d->force_long[i]*basis->forward[k];
            s->body.force[k]+=s->prepared_wheel[i][k];
        }
    s->prepared=1;
}
float stage1_drive_step(Stage1Drive *s,float pos[3],float vel[2],float *heading,
    float *speed,float pedal,float steer,int handbrake,const PhysSurface *sf,
    const PhysVehicle *vh,const PhysRideState *ride) {
    if(!s || !s->ready)return 0;
    if(!sf)sf=&PHYS_SURF_ROAD;
    Drivetrain *d=&s->drivetrain;ResBody *b=&s->body;
    s->wheel_travel=0;
    ResBasis basis=res_basis_level(*heading);
    for(int k=0;k<2;k++) { b->velocity[k]=vel[k]*PHYS_TICKRATE;b->momentum[k]=b->mass*b->velocity[k]; }
    float forward=(float)res_dot(b->velocity,basis.forward);
    /* Host keyboard policy, retained from the Stage-1 playable adapter. */
    if(pedal<0 && forward<=.3f)s->reverse=1;
    if(pedal>0 && forward>=-.3f)s->reverse=0;
    int braking=(pedal<0 && !s->reverse)||(pedal>0 && s->reverse);
    DrvInput input={.throttle=braking||handbrake?0:fabsf(pedal),
        .brake=braking?fabsf(pedal):0,.handbrake=handbrake?1:0,
        .gear_cmd=s->reverse?DRV_GEAR_R:DRV_CMD_AUTO};
    unsigned mask=ride?ride->contact_mask:0;
    ResInput *r=&s->resistance_input;
    res_frame_inputs(r,s->state_code,s->alternate_movement,s->disable_stabilize,mask,s->body_surface);
    /* One shared scale/bonus; this ordinary adapter has no exceptional engine multiplier. */
    d->tune.torque_scale=r->scale;d->tune.torque_bonus=r->bonus;
    DrvEnv env;drv_env_default(&env);env.rear_grip_multiplier=1;
    for(int k=0;k<4;k++){
        env.wheel[k].contact=(mask>>k)&1u;
        env.wheel[k].load=env.wheel[k].contact?d->car.mass*9.8f/4:0;
        env.wheel[k].mu_peak_surface=env.wheel[k].mu_slide_surface=sf->accel;
        env.wheel[k].locked=handbrake;
        env.wheel[k].roll_drag=s->rolling_surface[k];
        env.wheel[k].drag_flag=(s->damage_mask>>k)&1u;
        env.wheel[k].vx=forward;
    }
    /* Frame setup supplies a consistent current contact snapshot to selector/derivatives. */
    d->env=env;drv_frame_begin(d,&input,1/PHYS_TICKRATE,-1);r->throttle=d->throttle;
    float h;int n=drv_substep_count(1/PHYS_TICKRATE,&h);
    float decel=mask && braking && !handbrake
        ?10.5f*g_phys_tune.brake*(vh&&vh->brake>0?vh->brake:1)*d->brake:0;
    if(!s->prepared)prepare(s,&env,&basis,-copysignf(decel*b->mass,forward));
    for(int k=0;k<n;k++){
        /* Remove previous contact thrust when the host releases/locks a wheel.
         * Its prior prepared aero remains; there is no extra resistance pass. */
        for(int i=0;i<4;i++)if(!env.wheel[i].contact||handbrake) {
            for(int axis=0;axis<2;axis++) {
                b->force[axis]-=s->prepared_wheel[i][axis];
                s->prepared_wheel[i][axis]=0;
            }
        }
        float before=forward;
        float brake_force=-copysignf(fminf(decel*b->mass,fabsf(forward)*b->mass/h),forward);
        for(int axis=0;axis<2;axis++)b->force[axis]+=brake_force*basis.forward[axis];
        ResDerivative derivative;res_derivative(b,&s->resistance_car,r,&basis,0,&derivative);
        /* Body derivative is collected before wheel/shaft integration. The
         * independent blocks share no state mutated by either Euler update. */
        for(int i=0;i<4;i++)s->wheel_travel+=h*d->wheel[i].omega*.25f;
        res_integrate(b,&derivative,h);
        res_post(b,0);
        drv_integrate(d,h);
        forward=(float)res_dot(b->velocity,basis.forward);
        if(decel>0 && before*forward<0) {
            for(int axis=0;axis<2;axis++) {
                b->velocity[axis]-=forward*basis.forward[axis];
                b->momentum[axis]=b->mass*b->velocity[axis];
            }
            forward=0;
        }
        for(int i=0;i<4;i++){
            env.wheel[i].vx=forward;
            if(env.wheel[i].contact && (braking||handbrake)){
                d->wheel[i].omega=handbrake?0:forward/d->car.wheel_radius[i];
                d->wheel[i].tau_smooth=0;
            }
        }
        env.body_speed=hypotf(b->velocity[0],b->velocity[1]);
        prepare(s,&env,&basis,-copysignf(decel*b->mass,forward));
    }
    vel[0]=b->velocity[0]/PHYS_TICKRATE;vel[1]=b->velocity[1]/PHYS_TICKRATE;
    if(mask)return phys_stage1_chassis_step(pos,vel,heading,speed,steer,handbrake,sf,vh);
    pos[0]+=vel[0];pos[1]+=vel[1];*speed=forward/PHYS_TICKRATE;
    return fabsf(vel[0]*basis.forward[1]-vel[1]*basis.forward[0]);
}
