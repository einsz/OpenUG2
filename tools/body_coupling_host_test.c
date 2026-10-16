/* BC-1/2 adapter replay against an independent algebraic callback. Synthetic
 * Stage5 fixtures exercise candidate axes, centre offsets and all five passes. */
#define STAGE5_TEST_NO_MAIN
#include "drive_stage5_test.c"
static Stage1Drive *watched;
static Drivetrain before_coupling;
static ResBody before_aero;
static int checked_preparations,callback_passes;
static CtGround capture_query(void *u,const float q[3],int wheel){
 if(wheel==0){before_coupling=watched->drivetrain;before_aero=watched->body;}
 return query(u,q,wheel);
}
typedef struct {float mass,inverse,force[2],mount[4][2],angle[4];} Reference;
static void reference_accel(void *user,const float F[4],const float Y[4],float out[4]){
 const Reference *r=user;double x=r->force[0],y=r->force[1],moment=0;
 for(int i=0;i<4;i++){
  float c=cosf(r->angle[i]),s=sinf(r->angle[i]);
  double qx=(double)F[i]*c-(double)Y[i]*s,qy=(double)F[i]*s+(double)Y[i]*c;
  x+=qx;y+=qy;moment+=(double)r->mount[i][0]*qy-(double)r->mount[i][1]*qx;
 }
 double ax=x/r->mass,ay=y/r->mass,alpha=moment*r->inverse;
 for(int i=0;i<4;i++)out[i]=(float)((ax-alpha*r->mount[i][1])*cosf(r->angle[i])-(ay+alpha*r->mount[i][0])*sinf(r->angle[i]));
 callback_passes++;
}
static void replay(const Stage1Drive *s){
 ResBody prepared=before_aero;
 res_prepare_aero(&prepared,&s->resistance_car,&s->resistance_input,&s->contact_pose.axes,NULL);
 Reference r={.mass=s->body.mass,.inverse=s->inverse_yaw_inertia};
 if(s->dynamic_body){float yaw=s->body_car.inertia[2];if(s->motion.adjusted)yaw=(float)((double)yaw*s->body_car.yaw_scale);r.inverse=(float)(1/(double)yaw);}
 r.force[0]=(float)res_dot(prepared.force,s->contact_pose.axes.forward);r.force[1]=(float)res_dot(prepared.force,s->contact_pose.axes.lateral);
 for(int i=0;i<4;i++){
  for(int k=0;k<2;k++)r.mount[i][k]=s->contacts.car.mount[i][k]-s->contacts.car.centre[k];
  r.angle[i]=s->wheel_angle[i];
 }
 Drivetrain expected=before_coupling;callback_passes=0;
 drv_couple(&expected,&s->drivetrain.env,reference_accel,&r);
 assert(callback_passes==5);
 assert(!memcmp(&expected,&s->drivetrain,sizeof expected));
 checked_preparations++;
}
static void observe_coupling(void *u,const Stage1Drive *s,float h,int preparation){(void)u;(void)h;if(preparation)replay(s);}
static void fixture(Trace *t,int dynamic,unsigned selector,float scalar){
 contact_init(t,12);t->drive.inverse_yaw_inertia=scalar;
 CtCar contact=t->drive.contacts.car;contact.centre[0]=.2f;contact.centre[1]=-.1f;contact.centre[2]=.05f;
 assert(!stage5_drive_contacts(&t->drive,&contact,capture_query,NULL));
 if(dynamic){BodyCar body={.inertia={.5f,1.5f,2},.yaw_scale=3,.limit={0,0}};assert(!stage5_drive_body(&t->drive,&body,selector,0,1));t->drive.body_observe=observe_coupling;}
 watched=&t->drive;
}
static void host_replay(void){
 Trace t;
 for(int dynamic=0;dynamic<=1;dynamic++)for(unsigned selector=0;selector<=1;selector++){
  fixture(&t,dynamic,selector,.7f);
  PhysRideState ride={.pitch=.07f,.roll=-.03f,.pitch_rate=.1f,.roll_rate=.2f};
  for(int tick=0;tick<8;tick++){
   tick5(&t,tick<4?-.4f:.3f,.2f,0,&ride);
   if(!dynamic)replay(&t.drive);
  }
 }
 assert(checked_preparations==80);
 /* A supplied lower-stage fixture scalar cannot tune composed BR coupling,
  * including the adjusted-selection creation preparation before frame refresh. */
 Trace a,b;fixture(&a,1,0,.01f);for(int k=0;k<4;k++)tick5(&a,-.4f,.2f,0,NULL);
 fixture(&b,1,0,20);for(int k=0;k<4;k++)tick5(&b,-.4f,.2f,0,NULL);
 assert(!memcmp(&a.drive.drivetrain,&b.drive.drivetrain,sizeof a.drive.drivetrain));
 assert(!memcmp(&a.drive.motion,&b.drive.motion,sizeof a.drive.motion));
 assert(!memcmp(&a.drive.body,&b.drive.body,sizeof a.drive.body));
}
int main(void){host_replay();printf("BC host replay: %d preparations matched independent five-pass callback\n",checked_preparations);return 0;}
