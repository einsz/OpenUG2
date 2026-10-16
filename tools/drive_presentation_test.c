/* Independent render-time sampling: never feeds interpolation into physics. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "drive_presentation.h"
int main(void) {
 Stage1Drive s={0};s.dynamic_initialized=1;s.motion.axes=res_basis_level(0);
 s.motion.position[0]=10;s.contacts.car.centre[0]=1;
 for(int i=0;i<4;i++){s.contacts.car.mount[i][0]=i;s.contacts.wheel[i].travel=.1f*i;s.drivetrain.wheel[i].phi=i;}
 DrivePresentation a=drive_presentation_capture(&s),b=a;
 b.origin[0]+=2;b.axes=res_basis_level(.2f);
 for(int i=0;i<4;i++){b.hub[i][2]+=.2f;b.phase[i]+=i;}
 Stage1Drive unchanged=s;DrivePresentation r=drive_presentation_mix(&a,&b,.25f);
 assert(fabsf(r.origin[0]-9.5f)<1e-6f);
 for(int i=0;i<4;i++){assert(fabsf(r.hub[i][2]-(.1f*i+.05f))<1e-6f);assert(fabsf(r.phase[i]-1.25f*i)<1e-6f);}
 assert(fabs(res_dot(r.axes.forward,r.axes.lateral))<1e-6);
 assert(fabs(res_dot(r.axes.forward,r.axes.forward)-1)<1e-6);
 DrivePresentation exact=drive_presentation_mix(&a,&b,1);assert(!memcmp(&exact,&b,sizeof b));
 assert(drive_presentation_side(&a,0)==1);
 a.hub[2][1]=-.7f;a.hub[3][1]=.7f;
 assert(drive_presentation_side(&a,2)==-1&&drive_presentation_side(&a,3)==1);
 float m[16];drive_presentation_model(&a,0,m);assert(m[12]==9&&m[15]==1);
 assert(!memcmp(&s,&unchanged,sizeof s));
 /* Cadence check: a camera using alpha sees the body at the same sample time.
  * At 4 subframes per tick the former current-state body had 1.5m peak lead. */
 for(int k=0;k<4;k++){float t=k*.25f;DrivePresentation q=drive_presentation_mix(&a,&b,t);assert(fabsf(q.origin[0]-(9+2*t))<1e-6f);}
 puts("drive presentation: common sample time, independent hub/spin, exact endpoints and immutable physics passed");
}
