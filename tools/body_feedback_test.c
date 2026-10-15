/* Independent BR-7 host feedback acceptance. Invented car, flat world, unit
 * grip, c=1,p=1,race=0, no aero and bottoming suppressed, fixed 60 Hz entry. */
#define STAGE5_TEST_NO_MAIN
#include "drive_stage5_test.c"
typedef struct {BodyMotion motion;ResBody body;CtWheel wheel[4];float forces[4][3],moments[4][3];int preparation;} Snapshot;
static Snapshot snapshots[256];static int nsnapshot;static FILE *trace_file;
static void observe(void*u,const Stage1Drive*s,float h,int prepared){(void)u;assert(nsnapshot<256);Snapshot*q=snapshots+nsnapshot++;q->motion=s->motion;q->body=s->body;memcpy(q->wheel,s->contacts.wheel,sizeof q->wheel);memcpy(q->forces,s->contact_force,sizeof q->forces);memcpy(q->moments,s->contact_moment,sizeof q->moments);q->preparation=prepared;
 fprintf(trace_file,"%d,%s,%.9g",nsnapshot,prepared?"preparation":"derivative",h);
#define LOG(v) fprintf(trace_file,",%.9g",(double)(v))
 for(int k=0;k<3;k++)LOG(s->motion.position[k]);
 const float *axes[3]={s->motion.axes.forward,s->motion.axes.lateral,s->motion.axes.up};for(int a=0;a<3;a++)for(int k=0;k<3;k++)LOG(axes[a][k]);
 for(int k=0;k<3;k++)LOG(s->body.momentum[k]);
 for(int k=0;k<3;k++)LOG(s->motion.angular_momentum[k]);
 for(int k=0;k<3;k++)LOG(s->body.velocity[k]);
 for(int k=0;k<3;k++){LOG(s->motion.omega[k]);}
 LOG(s->body.z);LOG(s->motion.adjusted);
 for(int k=0;k<3;k++){LOG(s->body.force[k]);}
 for(int k=0;k<3;k++){LOG(s->body.moment[k]);}
 LOG(s->contacts.exported_mask);
 for(int i=0;i<4;i++){LOG(s->contacts.wheel[i].contact);LOG(s->contacts.wheel[i].travel);LOG(s->contacts.wheel[i].rate);LOG(s->contacts.wheel[i].load);}
 fputc('\n',trace_file);
#undef LOG
}
static void dynamic_init(Trace*t,float speed){contact_init(t,speed);t->drive.resistance_car=(ResCar){0};BodyCar body={.yaw_scale=t->drive.contacts.car.yaw_scale,.limit={0,0}};float adjusted[3];ct_inertia(&t->drive.contacts.car,body.inertia,adjusted);assert(!stage5_drive_body(&t->drive,&body,1,0,1));t->drive.body_observe=observe;nsnapshot=0;}
static void tick_body(Trace*t,float pedal,float steer){tick5(t,pedal,steer,0,NULL);}
static void pitch_feedback(void){Trace t;dynamic_init(&t,10);tick_body(&t,-1,0);
 /* First preparation makes braking force; next derivative consumes it, next
  * orientation derivative consumes its angular velocity. */
 assert(nsnapshot==8);Snapshot*p=snapshots+1,*d=snapshots+2,*after=snapshots+3,*pose=snapshots+5;
 float fx=0,my=0;for(int i=0;i<4;i++){fx+=p->forces[i][0];my+=p->moments[i][1];}assert(fx<0&&my>0);
 assert(d->motion.angular_momentum[1]==p->motion.angular_momentum[1]);assert(after->motion.angular_momentum[1]>d->motion.angular_momentum[1]);assert(after->motion.axes.forward[2]==d->motion.axes.forward[2]);assert(pose->motion.axes.forward[2]<after->motion.axes.forward[2]);
 assert(pose->wheel[0].travel!=p->wheel[0].travel&&pose->wheel[0].rate!=p->wheel[0].rate&&pose->wheel[0].load!=p->wheel[0].load);assert(pose->forces[0][2]!=p->forces[0][2]);
 float previous=t.drive.motion.axes.forward[2];nsnapshot=0;tick_body(&t,0,0);assert(t.drive.motion.axes.forward[2]!=previous); /* No prescribed reset on pedal release. */
 printf("BR pitch feedback: wheel Fx %.6g My %.6g f.z %.6g\n",fx,my,t.drive.motion.axes.forward[2]);
}
static void roll_feedback(void){Trace t;dynamic_init(&t,10);tick_body(&t,0,.5f);Snapshot*p=snapshots+1;float fy=0,mx=0;for(int i=0;i<4;i++){fy+=p->forces[i][1];mx+=p->moments[i][0];}assert(fy>0&&mx>0);assert(t.drive.motion.angular_momentum[0]>0);assert(t.drive.motion.axes.lateral[2]>0&&t.drive.motion.axes.up[1]<0);assert(t.drive.contacts.wheel[0].travel!=t.drive.contacts.wheel[1].travel);assert(t.drive.contacts.wheel[0].load!=t.drive.contacts.wheel[1].load);printf("BR roll feedback: wheel Fy %.6g Mx %.6g l.z %.6g\n",fy,mx,t.drive.motion.axes.lateral[2]);}
static CtGround raised_query(void*u,const float*q,int wheel){CtGround g=query(u,q,wheel);if(wheel==0)g.height+=.015f;return g;}
static void heave_feedback(void){Trace t;dynamic_init(&t,0);tick_body(&t,0,0);t.drive.contact_query=raised_query;nsnapshot=0;float oldz=t.drive.motion.position[2];tick_body(&t,0,0);Snapshot*p=snapshots+1,*after=snapshots+3,*pose=snapshots+5;assert(p->body.force[2]>0);assert(after->body.momentum[2]>p->body.momentum[2]);assert(pose->motion.position[2]!=after->motion.position[2]);assert(t.drive.motion.position[2]>oldz);assert(pose->wheel[0].travel!=p->wheel[0].travel&&pose->wheel[0].load!=p->wheel[0].load);printf("BR heave feedback: P.z %.6g X.z %.6g\n",t.drive.body.momentum[2],t.drive.motion.position[2]);}
static void impulses_and_restore(void){Trace t;dynamic_init(&t,0);t.drive.impulses_suppressed=0;ground_z=.24f;tick_body(&t,0,0);assert(t.drive.contacts.zero_sweeps>1&&t.drive.body.velocity[2]>0);Stage1Drive saved=t.drive;float old=t.drive.motion.position[0];float origin[3]={t.p[0]+1,t.p[1],t.p[2]};float vel[2]={0,.1f};stage5_body_host_correction(&t.drive,origin,vel);assert(fabsf(t.drive.motion.position[0]-old-1)<1e-6f&&t.drive.body.velocity[1]==6);assert(t.drive.drivetrain.wheel[0].omega==saved.drivetrain.wheel[0].omega);t.drive=saved;assert(!memcmp(&t.drive,&saved,sizeof saved));stage1_drive_reset(&t.drive,t.v,t.h);assert(t.drive.dynamic_body&&!t.drive.dynamic_initialized&&!t.drive.prepared&&t.drive.motion.adjusted==1);}
static void limits_parser(void){
 unsigned char bytes[16+0x890]={0};uint32_t tag=0x34600,size=8+0x890;memcpy(bytes,&tag,4);memcpy(bytes+4,&size,4);memset(bytes+8,0x11,8);unsigned char*row=bytes+16;strcpy((char*)row,"SYNTH");strcpy((char*)row+0x40,"CARS\\SYNTH\\GEOMETRY.BIN");CtCar c=contact_car();BodyCar out;
#define PUT(f,o) memcpy(row+(o),&(f),sizeof(f))
 PUT(c.centre,0x110);PUT(c.mass,0x220);PUT(c.dimensions,0x224);PUT(c.yaw_scale,0x270);
 for(int i=0;i<4;i++){PUT(c.mount[i],0x120+0x30*i);PUT(c.radius[i],0x130+0x30*i);}
 for(int i=0;i<2;i++){PUT(c.axle[i],0x280+0x20*i);}
 float limits[2]={7,11};PUT(limits,0x398);assert(stage5_body_car_read(bytes,sizeof bytes,"SYNTH",&out));assert(out.limit[0]==7&&out.limit[1]==11&&out.yaw_scale==c.yaw_scale);float invalid=-1;PUT(invalid,0x398);assert(!stage5_body_car_read(bytes,sizeof bytes,"SYNTH",&out));invalid=NAN;PUT(invalid,0x398);assert(!stage5_body_car_read(bytes,sizeof bytes,"SYNTH",&out));
#undef PUT
}
static void dynamic_clock(void){Trace ref,t;dynamic_init(&ref,10);ref.drive.body_observe=NULL;for(int i=0;i<60;i++)tick_body(&ref,-.3f,.1f);assert(isfinite(ref.drive.motion.position[2]));for(int rate=30;rate<=144;rate+=114){dynamic_init(&t,10);t.drive.body_observe=NULL;PhysClock clock={0};for(int frame=0;frame<rate;frame++){int ticks=phys_clock_steps(&clock,1.0/rate);for(int k=0;k<ticks;k++)tick_body(&t,-.3f,.1f);}assert(t.tick==60&&!memcmp(&ref.drive,&t.drive,sizeof t.drive));}}
int main(void){trace_file=fopen("build/body_feedback.csv","w");assert(trace_file);fputs("sample,boundary,h,Xx,Xy,Xz,fx,fy,fz,lx,ly,lz,ux,uy,uz,Px,Py,Pz,Lx,Ly,Lz,Vx,Vy,Vz,Ox,Oy,Oz,z,s,Fx,Fy,Fz,Tx,Ty,Tz,previous_contacts",trace_file);for(int i=0;i<4;i++)fprintf(trace_file,",w%d_contact,w%d_travel,w%d_rate,w%d_load",i,i,i,i);fputc('\n',trace_file);pitch_feedback();roll_feedback();heave_feedback();impulses_and_restore();limits_parser();dynamic_clock();fclose(trace_file);puts("BR-7 dynamic feedback acceptance passed; trace: build/body_feedback.csv");return 0;}
