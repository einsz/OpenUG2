/* S5 component composition with a prescribed OpenUG2 ride trajectory. The host
 * supplies heave/pitch/roll; XY/yaw consume full wheel contributions once. */
#include "drive_stage1.h"
#include <math.h>
#include <string.h>
int stage5_car_read(const unsigned char*data,long length,const char*name,CtCar*out){
 if(!data||!name||!out)return 0;
 long row=n2_global_car_record(data,length,name);if(row<0||row>length-0x890)return 0;
 CtCar c={0};
#define READ(f,o) memcpy(&(f),data+row+(o),sizeof(f))
 READ(c.centre,0x110);READ(c.mass,0x220);READ(c.dimensions,0x224);READ(c.yaw_scale,0x270);READ(c.aero_preload,0x37c);READ(c.bonus,0x390);
 for(int i=0;i<4;i++){READ(c.mount[i],0x120+0x30*i);READ(c.radius[i],0x130+0x30*i);}
 for(int i=0;i<2;i++){int o=0x280+0x20*i;READ(c.axle[i].progression,o);READ(c.axle[i].stiffness,o+4);READ(c.axle[i].d_positive,o+8);READ(c.axle[i].d_nonpositive,o+12);READ(c.axle[i].antiroll,o+16);READ(c.axle[i].upper,o+20);READ(c.axle[i].lower,o+24);READ(c.axle[i].reference,o+28);}
#undef READ
 /* Independent loader validation is stricter than the finite component domain. */
 const float *values=(const float*)&c;for(unsigned i=0;i<sizeof c/sizeof(float);i++)if(!isfinite(values[i]))return 0;
 if(c.mass<=0||c.yaw_scale<=0||c.mount[0][0]<=c.mount[3][0])return 0;
 for(int k=0;k<3;k++)if(c.dimensions[k]<=0)return 0;
 for(int k=0;k<4;k++)if(c.radius[k]<=0)return 0;
 for(int k=0;k<2;k++)if(c.axle[k].upper<=c.axle[k].lower)return 0;
 *out=c;return 1;
}
int stage5_drive_contacts(Stage1Drive*s,const CtCar*c,CtQuery query,void*user){if(!s||!s->ready||!s->lateral_ready||!s->drivetrain.brakes.enabled||!c||!query)return -1;ct_init(&s->contacts,c);s->contact_query=query;s->contact_user=user;s->contact_ready=1;s->prepared=0;s->drivetrain.brakes.signed_distribution=1;return 0;}
static void pose(Stage1Drive*s,const float pos[3],float heading,const PhysRideState*ride,float elapsed){
 CtPose*p=&s->contact_pose;float pitch=ride?ride->pitch+elapsed*ride->pitch_rate:0,roll=ride?ride->roll+elapsed*ride->roll_rate:0,ch=cosf(heading),sh=sinf(heading),cp=cosf(pitch),sp=sinf(pitch),cr=cosf(roll),sr=sinf(roll);
 p->axes=(ResBasis){{ch*cp,sh*cp,sp},{-sh*cr-ch*sp*sr,ch*cr-sh*sp*sr,cp*sr},{sh*sr-ch*sp*cr,-ch*sr-sh*sp*cr,cp*cr}};
 float bottom=0;for(int i=0;i<4;i++)bottom+=(s->contacts.car.mount[i][2]-s->contacts.car.radius[i])*.25f;
 for(int k=0;k<3;k++){p->position[k]=pos[k];p->velocity[k]=s->body.velocity[k];p->omega[k]=(ride?ride->roll_rate:0)*p->axes.forward[k];}
 p->position[2]+=s->contacts.car.centre[2]-bottom+(ride?elapsed*ride->vz:0);
 p->velocity[2]=ride?ride->vz:0;
 p->omega[0]+=(ride?ride->pitch_rate:0)*sh;p->omega[1]-=(ride?ride->pitch_rate:0)*ch;p->omega[2]+=s->yaw_rate;
}
static void geometry(Stage1Drive*s,float h){
 unsigned upper=ct_post(&s->contacts,&s->contact_pose,s->wheel_angle,h,s->contact_query,s->contact_user);
 /* U5-6 explicit host choice: bottoming impulses suppressed. The unknown caps
  * are not guessed. The required extra ordered zero sweep still occurs. */
 for(int i=0;i<4;i++) {
  CtWheel*w=s->contacts.wheel+i;CtPose*p=&s->contact_pose;
  float velocity[3]={p->velocity[0]+p->omega[1]*w->arm[2]-p->omega[2]*w->arm[1],p->velocity[1]+p->omega[2]*w->arm[0]-p->omega[0]*w->arm[2],p->velocity[2]+p->omega[0]*w->arm[1]-p->omega[1]*w->arm[0]};
  s->contacts.bottom_request[i]=(upper&(1u<<i))?ct_bottom(-(float)res_dot(w->normal,velocity),s->contacts.car.mass):0;
 }
 if(upper) {
  /* Repeat every established post callback, without derivatives/Euler. Even
   * clutch mode transitions can be observable in a zero-duration sweep. */
  res_post(&s->body,1);
  drv_post(&s->drivetrain);
  ct_post(&s->contacts,&s->contact_pose,s->wheel_angle,0,s->contact_query,s->contact_user);
 }
}
static void sample(Stage1Drive*s,DrvEnv*env){
 CtPose*p=&s->contact_pose;CtState*c=&s->contacts;env->body_speed=(float)sqrt(res_dot(p->velocity,p->velocity));env->signed_forward=(float)res_dot(p->velocity,p->axes.forward);env->planar_tyres=1;env->supplied_frame_grip=1;env->frame_grip[0]=c->grip_front;env->frame_grip[1]=c->grip_rear;
 for(int i=0;i<4;i++){CtWheel*w=c->wheel+i;DrvWheelEnv*e=env->wheel+i;ResBasis axes;ct_wheel_axes(&p->axes,s->wheel_angle[i],&axes);float velocity[3]={p->velocity[0]+p->omega[1]*w->arm[2]-p->omega[2]*w->arm[1],p->velocity[1]+p->omega[2]*w->arm[0]-p->omega[0]*w->arm[2],p->velocity[2]+p->omega[0]*w->arm[1]-p->omega[1]*w->arm[0]};e->vx=(float)res_dot(velocity,axes.forward);e->vy=(float)res_dot(velocity,axes.lateral);e->slip_angle=lat_slip(e->vx,e->vy);e->contact=w->contact;e->load=w->load;const CtSurface*sf=ct_surface(w->surface);e->mu_peak_surface=sf->peak;e->mu_slide_surface=sf->slide;e->roll_drag=sf->spin;e->locked=s->drivetrain.brakes.locked[i];e->drag_flag=(s->damage_mask>>i)&1u;e->lateral_grip_force=lat_grip(e->load,s->lateral_car.angle[i/2],s->lateral_car.reference[i/2],s->lateral_car.factor[i/2],e->slip_angle,e->vx,1);}
}
static void prepare(Stage1Drive*s,DrvEnv*env){
 ResBody*b=&s->body;CtPose*p=&s->contact_pose;Drivetrain*d=&s->drivetrain;
 res_prepare_aero(b,&s->resistance_car,&s->resistance_input,&p->axes,&s->aero);
 DrvBodyPlanar coupling={.mass=b->mass,.inv_iz=s->inverse_yaw_inertia,.ex=(float)res_dot(b->force,p->axes.forward),.ey=(float)res_dot(b->force,p->axes.lateral),.external_moment=(float)res_dot(b->moment,p->axes.up)};
 for(int i=0;i<4;i++){coupling.pos[i][0]=s->contacts.mount[i][0];coupling.pos[i][1]=s->contacts.mount[i][1];float a=s->wheel_angle[i];coupling.head[i][0]=cosf(a);coupling.head[i][1]=sinf(a);coupling.lat[i][0]=-sinf(a);coupling.lat[i][1]=cosf(a);}
 drv_couple(d,env,drv_body_accel_planar,&coupling);
 memset(s->contact_force,0,sizeof s->contact_force);memset(s->contact_moment,0,sizeof s->contact_moment);
 for(int i=0;i<4;i++){ResBasis axes;ct_wheel_axes(&p->axes,s->wheel_angle[i],&axes);ct_apply(p,s->contacts.wheel+i,&axes,1,d->force_long[i],d->force_lat[i],s->contact_force[i],s->contact_moment[i]);for(int k=0;k<3;k++){b->force[k]+=s->contact_force[i][k];b->moment[k]+=s->contact_moment[i][k];}for(int k=0;k<2;k++)s->prepared_wheel[i][k]=s->contact_force[i][k];s->prepared_moment[i]=s->contact_moment[i][2];}
 s->prepared=1;
}
float stage5_drive_step(Stage1Drive*s,float pos[3],float vel[2],float*heading,float*speed,float pedal,float steer,int hb,const PhysRideState*ride){
 const float dt=1/PHYS_TICKRATE;Drivetrain*d=&s->drivetrain;CtState*c=&s->contacts;ResBody*b=&s->body;ResInput*r=&s->resistance_input;DrvEnv env;drv_env_default(&env);s->wheel_travel=0;
 for(int k=0;k<2;k++){b->velocity[k]=vel[k]*PHYS_TICKRATE;b->momentum[k]=b->mass*b->velocity[k];}b->velocity[2]=ride?ride->vz:0;b->momentum[2]=b->mass*b->velocity[2];pose(s,pos,*heading,ride,0);
 /* Creation prepares with zero pedals, before first frame coefficient refresh. */
 if(!s->prepared){ct_frame(c,&s->resistance_car,r->enable,1,NULL);res_post(b,1);geometry(s,0);sample(s,&env);prepare(s,&env);ct_export(c,&s->contact_pose);}
 ct_frame(c,&s->resistance_car,r->enable,1,NULL);
 for(int i=0;i<4;i++)s->body_surface[i]=ct_surface(c->exported_surface[i])->body_drag;
 res_frame_inputs(r,s->state_code,s->alternate_movement,s->disable_stabilize,c->exported_mask,s->body_surface);
 d->tune.torque_scale=r->scale;d->tune.torque_bonus=r->bonus;
 float forward=c->exported_forward;if(pedal<0&&forward<=.3f)s->reverse=1;if(pedal>0&&forward>=-.3f)s->reverse=0;
 /* Independent keyboard policy (U3-1/U5-4): oppose actual motion with the
  * service brake, and hold the accelerator until both actual and pending
  * gears agree with the requested direction. The pending target matters when
  * an interrupted handoff changes gear during this tick. Keep this as a raw
  * frame input: shaping, selector/clutch progress and all frame consumers
  * retain their usual cadence, with no shaft/wheel/lock/force resets. */
 int braking=(pedal<0&&forward>.3f)||(pedal>0&&forward<-.3f);
 int opposite=s->reverse?(d->gear>DRV_GEAR_N||d->target_gear>DRV_GEAR_N):
     (d->gear==DRV_GEAR_R||d->target_gear==DRV_GEAR_R);
 DrvInput input={.throttle=braking||opposite?0:fabsf(pedal),.brake=braking?fabsf(pedal):0,.handbrake=hb?1:0,.gear_cmd=s->reverse?DRV_GEAR_R:DRV_CMD_AUTO};
 /* Distribution reads the exported frame snapshot. Shaping/exclusivity occurs
  * once inside drv_frame_begin, before hb/grip, and never in each substep. */
 d->env.body_speed=c->exported_speed;d->env.signed_forward=c->exported_forward;drv_frame_begin(d,&input,dt,-1);r->throttle=d->throttle;
 c->blend=ct_blend(d->handbrake,c->blend,dt);c->blend_refreshes++;
 float sigma=lat_body_slip(c->exported_forward,c->exported_heading,c->exported_velocity)*.01745329238474369f;
 float eta=ct_eta(d->handbrake,d->tune.is_player,c->exported_forward,sigma,ct_mph(c->exported_forward));float etas[4]={0,0,eta,eta};brk_handbrake(&d->brakes.car,etas,d->brakes.handbrake);
 CtGrip grip=ct_grip(d->tune.is_player,eta,sigma,c->exported_speed,fabsf(c->exported_heading[2]),c->blend,c->car.bonus[0],c->car.bonus[1],s->contact_event,d->tune.grip_q,d->tune.grip_loc_x,s->contact_v0,s->contact_s0);c->grip_front=grip.front;c->grip_rear=grip.rear;
 stage1_steering_frame(s,steer,&s->contact_pose.axes);
 float h;int steps=drv_substep_count(dt,&h);for(int step=0;step<steps;step++){
  ResDerivative derivative;res_derivative(b,&s->resistance_car,r,&s->contact_pose.axes,fabsf(s->yaw_rate),&derivative);
  float oldrate=s->yaw_rate;s->yaw_momentum=(float)(s->yaw_momentum+(double)h*(b->moment[2]-s->yaw_damping*s->yaw_momentum));s->yaw_rate=s->yaw_momentum*s->inverse_yaw_inertia;*heading=(float)(*heading+(double)h*oldrate);
  /* Position derivatives use pre-Euler velocity. Vertical pose is prescribed,
   * with full vertical force retained for diagnostics/external 3D consumers. */
  for(int k=0;k<2;k++)pos[k]=(float)(pos[k]+(double)h*b->velocity[k]);
  for(int i=0;i<4;i++)s->wheel_travel+=h*d->wheel[i].omega*.25f;
  res_integrate(b,&derivative,h);drv_integrate(d,h);res_post(b,1);
  b->velocity[2]=ride?ride->vz:0;b->momentum[2]=b->mass*b->velocity[2];pose(s,pos,*heading,ride,(step+1)*h);geometry(s,h);sample(s,&env);prepare(s,&env);
 }
 ct_export(c,&s->contact_pose);s->exported_forward=c->exported_forward;for(int k=0;k<2;k++)vel[k]=b->velocity[k]/PHYS_TICKRATE;*speed=c->exported_forward/PHYS_TICKRATE;return fabsf(vel[0]*sinf(*heading)-vel[1]*cosf(*heading));
}
