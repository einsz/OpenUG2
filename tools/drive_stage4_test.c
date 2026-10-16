/* Reuse independent synthetic car/adapter fixtures, never original assets. */
#define main stage123_regressions
#include "drive_stage1_test.c"
#undef main
static LatCar lateral_car(void){
 LatCar c={.mount={{1.25f,.75f,0},{1.25f,-.75f,0},{-1.25f,.75f,0},{-1.25f,-.75f,0}},.angle={12,12},.reference={1,1},.factor={1,1},.dimensions={4,1.8f,1.3f},.gain=1};return c;
}
static void lateral_init(Trace*t,float v){
 init(t,1.2f);LatCar c=lateral_car();assert(!stage4_drive_lateral(&t->drive,&c,1/lat_inertia(1.2f,c.dimensions),0));
 t->v[0]=v/PHYS_TICKRATE;stage1_drive_reset(&t->drive,t->v,0);t->drive.disable_stabilize=1;
}
static void turn(Trace*t,float pedal,float steer,unsigned mask){
 PhysRideState r={.contact_mask=mask};stage1_drive_step(&t->drive,t->p,t->v,&t->h,&t->s,pedal,steer,0,NULL,NULL,&r);++t->tick;
 assert(isfinite(t->h)&&isfinite(t->s)&&isfinite(t->drive.yaw_rate));
}
static void parser_lateral(void){
 unsigned char bytes[16+0x890]={0};uint32_t tag=0x34600,size=8+0x890;
 memcpy(bytes,&tag,4);memcpy(bytes+4,&size,4);memset(bytes+8,0x11,8);
 unsigned char*row=bytes+16;strcpy((char*)row,"SYNTH");strcpy((char*)row+0x40,"CARS\\SYNTH\\GEOMETRY.BIN");LatCar c=lateral_car(),out;
 for(int k=0;k<4;k++)memcpy(row+0x120+0x30*k,c.mount[k],12);
 for(int a=0;a<2;a++){memcpy(row+0x1e0+0x20*a,&c.angle[a],4);memcpy(row+0x1e4+0x20*a,&c.reference[a],4);memcpy(row+0x1e8+0x20*a,&c.factor[a],4);}
 memcpy(row+0x224,c.dimensions,12);memcpy(row+0x380,&c.gain,4);
 assert(stage4_car_read(bytes,sizeof bytes,"SYNTH",&out)&&!memcmp(&c,&out,sizeof c));
 assert(!stage4_car_read(bytes,sizeof bytes-1,"SYNTH",&out));assert(!stage4_car_read(bytes,sizeof bytes,"MISSING",&out));
 float bad=NAN;memcpy(row+0x1e4,&bad,4);assert(!stage4_car_read(bytes,sizeof bytes,"SYNTH",&out));
}
static void coupling_boundaries(void){
 float bf[2]={0,1},v[2]={10,10};assert(lat_body_slip(10,bf,v)==45);
 assert(lat_body_slip(-10,bf,v)==0);v[0]=0;assert(lat_body_slip(10,bf,v)==0);
 DrvCar c=car(1.2f);DrvTune tune;drv_tune_default(&tune);Drivetrain d;assert(!drv_init(&d,&c,&tune));
 DrvEnv env;drv_env_default(&env);env.planar_tyres=1;env.signed_forward=10;env.body_speed=10;
 for(int i=0;i<4;i++){
  d.wheel[i].omega=10/c.wheel_radius[i];d.wheel[i].w_lat=.5f;
  env.wheel[i].vx=10;env.wheel[i].load=5;env.wheel[i].slip_angle=.04f;
  env.wheel[i].lateral_grip_force=lat_grip(5,12,1,1,.04f,10,1);env.wheel[i].mu_peak_surface=100;
 }
 drv_couple(&d,&env,NULL,NULL);
 for(int i=0;i<4;i++)assert(fabsf(d.force_lat[i]-lat_grip(5,12,1,1,.04f,10,.5f))<1e-6f);
 /* Zero peak input must saturate the finite combined force, exactly once. */
 for(int i=0;i<4;i++)env.wheel[i].peak_scale=0;
 drv_couple(&d,&env,NULL,NULL);for(int i=0;i<4;i++)assert(d.force_long[i]==0&&d.force_lat[i]==0);
 /* The low-forward-speed sliding rule is deliberately signed in reverse. */
 d.env=env;d.env.wheel[0].mu_slide_surface=.5f;d.env.wheel[0].vx=-10;
 d.env.wheel[0].vy=2;d.env.wheel[0].slip_angle=-.1f;d.env.signed_forward=-10;
 d.wheel[0].omega=-8/c.wheel_radius[0];d.wheel[0].w_lat=1;d.wheel[0].tau=0;
 drv_prepare_tyre(&d,0);float reverse_force=d.wheel[0].force_lat;
 d.env.signed_forward=10;drv_prepare_tyre(&d,0);
 assert(fabsf(reverse_force)>fabsf(d.wheel[0].force_lat));
 /* BC-2 supersedes the external-moment provisional acceleration fixture. */
 DrvBodyPlanar body={.mass=2,.inv_iz=.5f,.external_moment=4};body.pos[0][0]=1;body.head[0][1]=1;
 float f[4]={0},fy[4]={0},accel[4];drv_body_accel_planar(&body,f,fy,accel);assert(accel[0]==0);
}
static void integration(void){
 Trace a,b,zero;lateral_init(&a,10);b=a;zero=a;
 for(int k=0;k<60;k++){turn(&a,.3f,.4f,15);turn(&b,.3f,-.4f,15);turn(&zero,.3f,0,15);}
 printf("Stage4 mirror: h %.5f / %.5f vy %.5f / %.5f yaw %.5f / %.5f\n",a.h,b.h,a.v[1]*60,b.v[1]*60,a.drive.yaw_rate,b.drive.yaw_rate);
 assert(a.h>.02f&&b.h<-.02f&&a.v[1]>0&&b.v[1]<0);
 assert(fabsf(a.h+b.h)<.005f&&fabsf(a.drive.yaw_rate+b.drive.yaw_rate)<.02f);
 assert(zero.h==0&&zero.v[1]==0&&zero.drive.yaw_rate==0);
 assert(a.drive.wheel_angle[0]>0&&a.drive.wheel_angle[1]>a.drive.wheel_angle[0]);
 /* Prepared force/moment are final sums, without accumulating provisional passes. */
 float sum[2]={0},moment=0;
 for(int i=0;i<4;i++){for(int axis=0;axis<2;axis++)sum[axis]+=a.drive.prepared_wheel[i][axis];moment+=a.drive.prepared_moment[i];}
 for(int axis=0;axis<2;axis++)assert(fabs(a.drive.body.force[axis]-sum[axis]-a.drive.aero.drag[axis])<.00001f);
 assert(fabsf(moment-a.drive.body.moment[2])<.00001f);
 /* Brake while turning retains signed yaw and actual Stage3 brake request. */
 a=zero;b=zero;for(int k=0;k<30;k++){turn(&a,-1,.25f,15);turn(&b,0,.25f,15);}
 assert(a.s<b.s&&a.h>0&&a.drive.drivetrain.brake==1);
 /* Reverse geometry retains command sign, with reversed vehicle yaw response. */
 lateral_init(&a,-8);for(int k=0;k<40;k++)turn(&a,-.2f,.25f,15);
 assert(a.drive.wheel_angle[0]>0&&a.h<0&&a.drive.exported_forward<0);
 /* Contact loss removes carried tyre forces AND moments before derivative.
  * Supplied zero load also zeros prepared curves; angular momentum persists. */
 lateral_init(&a,10);for(int k=0;k<30;k++)turn(&a,.3f,.3f,15);
 float oldrate=a.drive.yaw_rate,oldh=a.h;for(int k=0;k<20;k++)turn(&a,0,-1,0);
 assert(a.h>oldh&&a.drive.yaw_rate==oldrate);
 for(int i=0;i<4;i++)assert(a.drive.drivetrain.load[i]==0&&a.drive.prepared_moment[i]==0&&a.drive.prepared_wheel[i][0]==0&&a.drive.prepared_wheel[i][1]==0);
 /* Collision velocity impulses enter the next contact sample, not rolling spin
  * enforcement or a direct yaw reset. Wall helper remains the production helper. */
 lateral_init(&a,10);for(int k=0;k<30;k++)turn(&a,.2f,.2f,15);
 a.v[0]=-.03f;a.v[1]=.08f;
 { /* Actual normal wall response changes only supported velocity; the next
    * drivetrain tick must consume it without wiping the shaft/angular state. */
  PhysRideState ride={.contact_mask=15};float before[2]={a.v[0],a.v[1]};
  a.v[0]=0;assert(phys_ride_wall_response(&ride,before,a.v)>0);
 }float omega=a.drive.drivetrain.wheel[0].omega;
 turn(&a,0,0,15);assert(a.drive.drivetrain.env.wheel[0].vy>1&&a.drive.drivetrain.wheel[0].omega!=a.drive.drivetrain.env.wheel[0].vx/.32f&&isfinite(omega));
 /* Reset/car replacement clears steering, histories, latch, prepared force and
  * angular state while retaining configured supplied chassis/tyre parameters. */
 stage1_drive_reset(&a.drive,a.v,a.h);assert(a.drive.lateral_ready&&!a.drive.steering.command&&!a.drive.steering.latch&&a.drive.steering.previous_limit==45);
 assert(!a.drive.prepared&&a.drive.yaw_rate==0&&a.drive.input_cursor==0&&a.drive.rate_history[0]==1);
 DrvCar c=car(1.8f);assert(!configured_init(&a.drive,&c,&resistance,0));assert(!a.drive.lateral_ready);LatCar lc=lateral_car();lc.gain=.8f;assert(!stage4_drive_lateral(&a.drive,&lc,1/lat_inertia(c.mass,lc.dimensions),0));assert(a.drive.lateral_car.gain==.8f);
 /* Host tunable steer/lat factors no longer add yaw or erase lateral velocity. */
 lateral_init(&a,10);b=a;PhysRideState r={.contact_mask=15};PhysSurface sf=PHYS_SURF_ROAD;sf.steer=20;sf.lat=.1f;PhysVehicle vh={.steer=30,.lat=.1f};
 stage1_drive_step(&a.drive,a.p,a.v,&a.h,&a.s,.2f,.3f,0,&sf,&vh,&r);
 turn(&b,.2f,.3f,15);assert(a.h==b.h&&a.v[1]==b.v[1]);
 /* Same fixed tick sequence across render rates, including moving steering and
  * brake input changes. No render callback mutates histories/angular state. */
 Trace reference;lateral_init(&reference,10);
 for(int k=0;k<180;k++)turn(&reference,k<120?.3f:-1,k<60?.2f:k<120?-.2f:0,15);
 int rates[]={30,60,90,144,240};for(unsigned rix=0;rix<sizeof rates/sizeof *rates;rix++){
  lateral_init(&a,10);PhysClock clock={0};for(int frame=0;frame<3*rates[rix];frame++){int count=phys_clock_steps(&clock,1.0/rates[rix]);for(int k=0;k<count;k++)turn(&a,a.tick<120?.3f:-1,a.tick<60?.2f:a.tick<120?-.2f:0,15);}
  assert(a.tick==180&&a.h==reference.h&&!memcmp(&a.drive,&reference.drive,sizeof a.drive));
 }
}
int main(void){parser_lateral();coupling_boundaries();integration();puts("Stage4 gameplay adapter tests passed");return 0;}
