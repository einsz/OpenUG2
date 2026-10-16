/* Synthetic host composition. Prescribed pitch/roll are fixture trajectories,
 * never presented as a recovered dynamic angular/body producer. */
#define main stage123_regressions
#include "drive_stage1_test.c"
#undef main
static LatCar lateral_car(void){return (LatCar){.mount={{1.25f,.75f,0},{1.25f,-.75f,0},{-1.25f,.75f,0},{-1.25f,-.75f,0}},.angle={12,12},.reference={1,1},.factor={1,1},.dimensions={4,1.8f,1.3f},.gain=1};}
static void lateral_init(Trace*t,float v){init(t,1.2f);LatCar c=lateral_car();assert(!stage4_drive_lateral(&t->drive,&c,1/lat_inertia(1.2f,c.dimensions),0));t->v[0]=v/60;stage1_drive_reset(&t->drive,t->v,0);t->drive.disable_stabilize=1;}
static int ground_found=1,surface_id=1;static float ground_z=0,normal[3]={0,0,1};
static CtGround query(void*u,const float q[3],int i){(void)u;(void)q;(void)i;CtGround g={.found=ground_found,.height=ground_z,.surface=surface_id};memcpy(g.normal,normal,sizeof normal);return g;}
static CtCar contact_car(void){LatCar l=lateral_car();CtCar c={.mass=1.2f,.dimensions={4,1.8f,1.3f},.yaw_scale=1.2f,.aero_preload=.4f,.bonus={.4f,.2f}};memcpy(c.mount,l.mount,sizeof c.mount);for(int i=0;i<4;i++)c.radius[i]=.32f;for(int i=0;i<2;i++)c.axle[i]=(CtAxle){.stiffness=18,.progression=2,.d_positive=1,.d_nonpositive=1.5f,.antiroll=4,.upper=.2f,.lower=-.2f,.reference=.13f};return c;}
static void contact_init(Trace*t,float v){ground_found=1;ground_z=0;surface_id=1;normal[0]=normal[1]=0;normal[2]=1;lateral_init(t,v);CtCar c=contact_car();assert(!stage5_drive_contacts(&t->drive,&c,query,NULL));t->drive.disable_stabilize=1;}
static void tick5(Trace*t,float pedal,float steer,int hb,const PhysRideState*r){stage1_drive_step(&t->drive,t->p,t->v,&t->h,&t->s,pedal,steer,hb,NULL,NULL,r);t->tick++;assert(isfinite(t->h)&&isfinite(t->s)&&isfinite(stage1_drive_rpm(&t->drive)));for(int i=0;i<4;i++)assert(isfinite(t->drive.contacts.wheel[i].load)&&t->drive.contacts.wheel[i].load>=0);}
static void components_extra(void){
 CtCar c=contact_car();float a[3],b[3];ct_inertia(&c,a,b);assert(fabsf(a[0]-1.2f*(1.8f*1.8f+1.3f*1.3f)/12)<1e-6f);assert(b[2]==a[2]*c.yaw_scale);
 float p[3]={1,2,3},old[3]={0},changed[3]={1,2,3};ResBasis basis=res_basis_level(0);ct_change_centre(p,&basis,old,changed);assert(p[0]==2&&p[1]==4&&p[2]==6);
 CtPreload pc=ct_preload(1,.2f,1.4f,-1.2f,0,&resistance,0,0,1);assert(fabsf(pc.rear_share-.5f*1.4f/2.6f)<1e-7f);
 CtGround g={.found=1,.height=0,.surface=8,.override_surface=1,.normal={0,0,1}};int surface;float q[3]={.5f,.5f,.3f};float bump=ct_bump(q[0],q[1],ct_surface(8)->frequency,ct_surface(8)->amplitude);assert(bump!=0);ct_ground_distance(q,basis.up,&g,bump,&surface);assert(surface==1);g.normal[2]=0;g.normal[0]=1;assert(ct_ground_distance(q,basis.up,&g,bump,&surface)==-100&&surface==0);
 float linear[3],angular[3];ct_impulse((float[3]){0,0,1},(float[3]){2,0,0},10,3,2,0,linear,angular);assert(linear[2]==3&&angular[1]==-2);ct_impulse((float[3]){0,0,1},(float[3]){2,0,0},10,3,2,1,linear,angular);assert(linear[2]==0&&angular[1]==0);
 CtPose pose={.axes={{1,0,0},{0,1,0},{0,0,1}}};CtWheel w={.contact=1,.load=4,.normal={.6f,0,.8f},.point={1,1,0}};
 for(int selector=1;selector<=4;selector++){float f[3]={0},m[3]={0};ct_apply(&pose,&w,&pose.axes,selector,2,1,f,m);assert(isfinite(f[0])&&isfinite(m[2]));if(selector==1)assert(f[0]==2&&f[1]==1&&f[2]==4);if(selector==4)assert(f[0]==4.4f&&f[1]==1&&f[2]==3.2f);if(selector==2)assert(fabsf(f[0]-3.68f)<1e-6f&&fabsf(f[2]-2.24f)<1e-6f);if(selector==3)assert(fabsf(f[0]-4)<1e-6f&&fabsf(f[2]-2)<1e-6f);}
}
static void parser5(void){
 unsigned char bytes[16+0x890]={0};uint32_t tag=0x34600,size=8+0x890;memcpy(bytes,&tag,4);memcpy(bytes+4,&size,4);memset(bytes+8,0x11,8);unsigned char*row=bytes+16;strcpy((char*)row,"SYNTH");strcpy((char*)row+0x40,"CARS\\SYNTH\\GEOMETRY.BIN");CtCar c=contact_car(),out;
#define PUT(f,o) memcpy(row+(o),&(f),sizeof(f))
 PUT(c.centre,0x110);PUT(c.mass,0x220);PUT(c.dimensions,0x224);PUT(c.yaw_scale,0x270);PUT(c.aero_preload,0x37c);PUT(c.bonus,0x390);
 for(int i=0;i<4;i++){PUT(c.mount[i],0x120+0x30*i);PUT(c.radius[i],0x130+0x30*i);}
 for(int i=0;i<2;i++){CtAxle*a=c.axle+i;int o=0x280+0x20*i;PUT(a->progression,o);PUT(a->stiffness,o+4);PUT(a->d_positive,o+8);PUT(a->d_nonpositive,o+12);PUT(a->antiroll,o+16);PUT(a->upper,o+20);PUT(a->lower,o+24);PUT(a->reference,o+28);}
 assert(stage5_car_read(bytes,sizeof bytes,"SYNTH",&out)&&!memcmp(&c,&out,sizeof c));assert(!stage5_car_read(bytes,sizeof bytes-1,"SYNTH",&out));assert(!stage5_car_read(bytes,sizeof bytes,"MISSING",&out));float invalid=NAN;PUT(invalid,0x290);assert(!stage5_car_read(bytes,sizeof bytes,"SYNTH",&out));
#undef PUT
}
static float first_velocity;static int observations,zero_sweep_gear;static Stage1Drive*observed;
static CtGround observed_query(void*u,const float q[3],int w){if(observations==4)zero_sweep_gear=observed->drivetrain.gear;if(observations++==0)first_velocity=observed->body.velocity[0];return query(u,q,w);}
static void accounting(const Trace*t){const Stage1Drive*s=&t->drive;float sum[3]={0,0,-s->body.mass*9.8f},m[3]={0};for(int k=0;k<3;k++){sum[k]+=s->aero.drag[k]+s->aero.front[k]+s->aero.rear[k];/* aero moment checked separately by fresh isolated preparation below */for(int i=0;i<4;i++)sum[k]+=s->contact_force[i][k];assert(fabsf(sum[k]-s->body.force[k])<3e-5f);}ResBody b={.mass=s->body.mass};memcpy(b.velocity,s->body.velocity,sizeof b.velocity);res_prepare_aero(&b,&s->resistance_car,&s->resistance_input,&s->contact_pose.axes,NULL);for(int k=0;k<3;k++){m[k]=b.moment[k];for(int i=0;i<4;i++)m[k]+=s->contact_moment[i][k];assert(fabsf(m[k]-s->body.moment[k])<3e-5f);}}
static void integration5(void){
 Trace a,b;PhysRideState ride={.contact_mask=15};contact_init(&a,12);b=a;
 for(int i=0;i<20;i++){tick5(&a,0,0,0,&ride);tick5(&b,0,.1f,1,&ride);}printf("Stage5 moving hb: coast %.5f hb %.5f blend %.5f\n",a.s*60,b.s*60,b.drive.contacts.blend);assert(b.s<a.s&&b.s>0&&b.drive.contacts.blend==.5f&&b.drive.drivetrain.brakes.handbrake[2]>0);assert(b.drive.drivetrain.brakes.handbrake[0]==0);float before=b.drive.contacts.blend;for(int i=0;i<30;i++)tick5(&b,.3f,0,0,&ride);assert(b.drive.contacts.blend>before&&b.drive.drivetrain.brakes.handbrake[2]==0);accounting(&b);assert(b.drive.contacts.blend_refreshes==50&&b.drive.contacts.frame_refreshes==51&&b.drive.contacts.post_sweeps==201);
 contact_init(&a,12);b=a;for(int i=0;i<30;i++){tick5(&a,-1,.15f,0,&ride);tick5(&b,0,.15f,0,&ride);}assert(a.s<b.s&&a.h>0);accounting(&a);
 contact_init(&a,10);b=a;for(int i=0;i<60;i++){tick5(&a,.3f,.25f,0,&ride);tick5(&b,.3f,-.25f,0,&ride);}assert(a.h>0&&b.h<0&&fabsf(a.h+b.h)<.005f);assert(fabsf(a.v[1]+b.v[1])<.005f);
 /* Prescribed pitch, versus flat ground with identical brake command. No
  * algebraic pedal/acceleration transfer operation is present. */
 contact_init(&a,10);b=a;PhysRideState pitch=ride;pitch.pitch=-.07f;tick5(&a,-1,0,0,&pitch);tick5(&b,-1,0,0,&ride);assert(a.drive.contacts.wheel[0].load>b.drive.contacts.wheel[0].load&&a.drive.contacts.wheel[2].load<b.drive.contacts.wheel[2].load);accounting(&a);
 Trace mirrored;contact_init(&mirrored,10);PhysRideState noseup=ride;noseup.pitch=.07f;tick5(&mirrored,1,0,0,&noseup);
 for(int i=0;i<2;i++){assert(fabsf(a.drive.contacts.wheel[i].load-mirrored.drive.contacts.wheel[i+2].load)<1e-4f);assert(fabsf(a.drive.contacts.wheel[i+2].load-mirrored.drive.contacts.wheel[i].load)<1e-4f);}
 contact_init(&a,10);PhysRideState roll=ride;roll.roll=.07f;tick5(&a,0,.2f,0,&roll);assert(a.drive.contacts.wheel[0].load<a.drive.contacts.wheel[1].load);accounting(&a);
 contact_init(&b,10);roll.roll=-.07f;tick5(&b,0,-.2f,0,&roll);assert(b.drive.contacts.wheel[0].load>b.drive.contacts.wheel[1].load);
 /* First derivative consumes the prior frame's prepared force under changed
  * handbrake and newly missing terrain. Observe at first post-Euler query. */
 contact_init(&a,10);tick5(&a,.3f,0,0,&ride);a.drive.contact_query=observed_query;observed=&a.drive;observations=0;float old=a.drive.body.velocity[0],f=a.drive.body.force[0],h;drv_substep_count(1/60.0f,&h);ground_found=0;tick5(&a,0,0,1,&ride);assert(fabsf(first_velocity-(old+h*f/a.drive.body.mass))<2e-6f);assert(a.drive.contacts.exported_mask==0);for(int i=0;i<4;i++){assert(a.drive.contacts.wheel[i].load==0&&a.drive.contacts.wheel[i].scalar==0);for(int k=0;k<3;k++)assert(a.drive.contact_force[i][k]==0);}accounting(&a);
 float oldtravel=a.drive.contacts.wheel[0].travel;ground_found=1;tick5(&a,0,0,0,&ride);assert(a.drive.contacts.exported_mask==15&&oldtravel==a.drive.contacts.car.axle[0].lower&&isfinite(a.drive.contacts.wheel[0].rate));
 /* Current mode changes only after preload reads preceding enable. */
 contact_init(&a,20);tick5(&a,0,0,0,&ride);a.drive.state_code=1;float exported=a.drive.contacts.exported_forward;tick5(&a,0,0,0,&ride);CtPreload pl=ct_preload(1.2f,0,1.25f,-1.25f,1,&resistance,.4f,exported,1);assert(a.drive.contacts.preload[0]==pl.front&&a.drive.resistance_input.enable==0);tick5(&a,0,0,0,&ride);assert(fabsf(a.drive.contacts.preload[0]-1.2f*9.8f/4)<1e-6f);
 /* Immediate tyre surfaces versus exported body-drag latency. */
 contact_init(&a,10);tick5(&a,0,0,0,&ride);surface_id=15;tick5(&a,0,0,0,&ride);assert(a.drive.resistance_input.surface==1&&a.drive.drivetrain.env.wheel[0].roll_drag==ct_surface(15)->spin&&a.drive.drivetrain.env.wheel[0].mu_peak_surface==ct_surface(15)->peak);tick5(&a,0,0,0,&ride);assert(a.drive.resistance_input.surface==5);
 /* Explicit suppression avoids guessed impulse caps, retains extra sweep. */
 contact_init(&a,0);ground_z=.24f;a.drive.contact_query=observed_query;observed=&a.drive;observations=0;zero_sweep_gear=-1;a.drive.drivetrain.mode=DRV_CLUTCH_OPEN;a.drive.drivetrain.target_gear=2;a.drive.drivetrain.pending_shift=1;tick5(&a,0,0,0,&ride);assert(zero_sweep_gear==2);assert(a.drive.contacts.zero_sweeps>1);assert(a.drive.contacts.wheel[0].travel==.2f);assert(a.drive.body.velocity[2]==0);
 contact_init(&a,-10);for(int i=0;i<10;i++)tick5(&a,-.2f,.2f,1,&ride);assert(a.drive.drivetrain.brakes.handbrake[2]==7.5f&&a.drive.contacts.exported_forward<0&&isfinite(a.h));
 /* Reset discards transient preparation/contact/blend and retains providers. */
 stage1_drive_reset(&a.drive,a.v,a.h);assert(a.drive.contact_ready&&a.drive.contact_query==query&&a.drive.contacts.blend==1&&!a.drive.prepared&&!a.drive.contacts.exported_mask);for(int i=0;i<4;i++)assert(a.drive.contacts.wheel[i].travel==0&&a.drive.contacts.wheel[i].rate==0);Stage1Drive saved=a.drive;PhysClock clock={0};assert(phys_clock_steps(&clock,0)==0&&!memcmp(&saved,&a.drive,sizeof saved));
 DrvCar car2=car(1.8f);assert(!configured_init(&a.drive,&car2,&resistance,0)&&!a.drive.contact_ready);
 /* Collision velocity changes are accepted without reassigning wheel spin. */
 contact_init(&a,10);tick5(&a,.2f,.1f,0,&ride);float spin=a.drive.drivetrain.wheel[0].omega;a.v[0]=0;a.v[1]=.04f;float beforewall[2]={.1f,.04f};assert(phys_ride_wall_response(&ride,beforewall,a.v)>0);tick5(&a,0,0,0,&ride);assert(isfinite(spin)&&a.drive.drivetrain.wheel[0].omega!=a.drive.drivetrain.env.wheel[0].vx/.32f);
}
static void clock5(void){Trace ref,a;PhysRideState ride={.contact_mask=15};contact_init(&ref,10);for(int k=0;k<180;k++)tick5(&ref,k<120?.4f:-1,k<60?.2f:k<120?-.2f:0,k>=75&&k<90,&ride);int rates[]={30,60,90,144,240};for(unsigned i=0;i<sizeof rates/sizeof*rates;i++){contact_init(&a,10);PhysClock c={0};for(int frame=0;frame<3*rates[i];frame++){int count=phys_clock_steps(&c,1.0/rates[i]);for(int k=0;k<count;k++)tick5(&a,a.tick<120?.4f:-1,a.tick<60?.2f:a.tick<120?-.2f:0,a.tick>=75&&a.tick<90,&ride);}assert(a.tick==180&&a.h==ref.h&&a.p[0]==ref.p[0]&&!memcmp(&a.drive,&ref.drive,sizeof a.drive));}}
/* Independent keyboard policy: a direction request is not an accelerator for
 * the opposite actual gear. Inspect each post-step query, not only tick exit:
 * an Open clutch can change gear in the middle of a host tick. */
static Stage1Drive *direction_observed;
static int direction_samples;
static float direction_first_velocity;
static int direction_frame_samples;
static CtGround direction_query(void *u,const float q[3],int wheel){
 Stage1Drive *s=direction_observed;Drivetrain *d=&s->drivetrain;
 int opposite=s->reverse?d->gear>DRV_GEAR_N:d->gear==DRV_GEAR_R;
 assert(!opposite||d->throttle==0);
 assert(s->resistance_input.throttle==d->throttle);
 if(!direction_frame_samples++)direction_first_velocity=s->body.velocity[0];
 direction_samples++;return query(u,q,wheel);
}
static void watch_direction(Trace *t){direction_observed=&t->drive;t->drive.contact_query=direction_query;}
static void direction_handoff5(void){
 PhysRideState ride={.contact_mask=15};
 Trace launch;contact_init(&launch,0);watch_direction(&launch);
 tick5(&launch,1,0,0,&ride);assert(launch.drive.drivetrain.throttle==1);
 for(int k=0;k<600;k++)tick5(&launch,1,0,0,&ride);
 assert(launch.s>.05f&&launch.drive.drivetrain.gear>DRV_GEAR_N);
 for(int reverse=0;reverse<=1;reverse++)for(int mode=0;mode<4;mode++){
  Trace t;contact_init(&t,reverse?.1f:-.1f);tick5(&t,0,0,0,&ride);
  Drivetrain *d=&t.drive.drivetrain;int actual=reverse?2:DRV_GEAR_R;
  d->gear=d->target_gear=actual;d->mode=mode;
  d->engage=mode==DRV_CLUTCH_OPEN?0:mode==DRV_CLUTCH_LOCKED?1:.6f;
  d->pending_shift=0;t.drive.reverse=!reverse;
  float pedal=reverse?-1:1;watch_direction(&t);
  float h;drv_substep_count(1/PHYS_TICKRATE,&h);
  float incoming=t.drive.body.velocity[0]+h*t.drive.body.force[0]/t.drive.body.mass;
  direction_frame_samples=0;
  tick5(&t,pedal,0,0,&ride);
  assert(fabsf(direction_first_velocity-incoming)<2e-6f);
  assert(d->throttle==0&&d->brake==0);
  assert(d->target_gear==(reverse?DRV_GEAR_R:2));
  for(int k=0;k<600;k++)tick5(&t,pedal,0,0,&ride);
  assert(reverse?t.s<-.02f:t.s>.02f);
  assert(reverse?d->gear==DRV_GEAR_R:d->gear>DRV_GEAR_N);
  assert(d->throttle==1&&stage1_drive_rpm(&t.drive)>=d->car.idle_rpm-.01f);
 }
 /* The safety hold does not alter ordinary in-gear demand or the existing
  * forward anti-stall floor. Neutral launches are exercised by integration. */
 for(int reverse=0;reverse<=1;reverse++)for(int mode=0;mode<4;mode++){
  Trace t;contact_init(&t,0);tick5(&t,0,0,0,&ride);
  Drivetrain *d=&t.drive.drivetrain;d->gear=d->target_gear=reverse?DRV_GEAR_R:2;
  d->mode=mode;d->engage=mode==DRV_CLUTCH_OPEN?0:mode==DRV_CLUTCH_LOCKED?1:.6f;
  t.drive.reverse=reverse;watch_direction(&t);
  tick5(&t,reverse?-.3f:.3f,0,0,&ride);
  assert(d->throttle==(reverse?.3f:.5f)&&d->brake==0);
 }
 /* Cancel/reissue a direction while an Open clutch still has the previous
  * target. This exercises gear changes within the tick and rapid W/S changes. */
 for(int reverse=0;reverse<=1;reverse++){
  Trace t;contact_init(&t,0);tick5(&t,0,0,0,&ride);
  Drivetrain *d=&t.drive.drivetrain;d->gear=reverse?DRV_GEAR_R:2;
  d->target_gear=reverse?2:DRV_GEAR_R;d->mode=DRV_CLUTCH_OPEN;
  d->engage=0;d->pending_shift=1;t.drive.reverse=reverse;watch_direction(&t);
  tick5(&t,reverse?-1:1,0,0,&ride);
  for(int k=0;k<240;k++)tick5(&t,k&1?-1:1,0,0,&ride);
  for(int k=0;k<900;k++)tick5(&t,1,0,0,&ride);
  assert(t.s>.05f&&d->gear>DRV_GEAR_N&&d->throttle==1);
 }
 /* Actual movement still determines service braking, including a direction
  * latch retained from an interrupted transition or external collision. */
 for(int reverse=0;reverse<=1;reverse++){
  Trace t;contact_init(&t,reverse?5:-5);tick5(&t,0,0,0,&ride);
  t.drive.reverse=reverse;watch_direction(&t);
  tick5(&t,reverse?-1:1,0,0,&ride);
  assert(t.drive.drivetrain.brake==1&&t.drive.drivetrain.throttle==0);
 }
 assert(direction_samples>10000);
 printf("Stage5 direction handoff: %d substep observations passed\n",direction_samples);
}
#ifndef STAGE5_TEST_NO_MAIN
int main(void){direction_handoff5();components_extra();parser5();integration5();clock5();puts("Stage5 adapter: contact lifecycle, force accounting, handbrake/recovery, host pose, surfaces, reset/collision/clock passed");return 0;}

#endif
