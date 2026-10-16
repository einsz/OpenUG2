/* Trace schema and observational boundary; no game installation required. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drive_trace.h"

static const char *column(char *header,char *row,const char *name) {
    char *h=header,*r=row;
    while(h && r) {
        char *hn=strchr(h,','),*rn=strchr(r,',');
        size_t len=hn?(size_t)(hn-h):strcspn(h,"\n");
        if(strlen(name)==len && !strncmp(h,name,len))return r;
        h=hn?hn+1:NULL;r=rn?rn+1:NULL;
    }
    assert(!"missing trace column");return NULL;
}
int main(void) {
    FILE *f=tmpfile();assert(f);
    Stage1Drive s={0};
    s.drivetrain.omega_e=100;s.drivetrain.wheel[2].omega=45;
    s.drivetrain.wheel[2].tau=3;s.drivetrain.brakes.locked[0]=1;
    s.contacts.wheel[2].load=0;s.contacts.wheel[2].contact=1;
    s.body.velocity[0]=.2f;s.body.force[0]=8;
    DriveTraceSample sample={.tick=73,.active=1,.pedal=1,.position={10,20,30},
        .velocity={0,0},.prepared_force={-4,0,0},.ground_fraction=.5f,
        .walls=1,.rails=2,.vehicle_contact=3};
    Stage1Drive before=s;DriveTraceSample sample_before=sample;
    assert(drive_trace_header(f));assert(drive_trace_row(f,&s,&sample));
    assert(!memcmp(&s,&before,sizeof s));assert(!memcmp(&sample,&sample_before,sizeof sample));
    rewind(f);char header[16384],row[16384];
    assert(fgets(header,sizeof header,f)&&fgets(row,sizeof row,f));
    int hc=0,rc=0;for(char *p=header;*p;p++)hc+=*p==',';
    for(char *p=row;*p;p++)rc+=*p==',';
    assert(hc==rc && hc>100);
    assert(atoi(column(header,row,"tick"))==73);
    assert(atoi(column(header,row,"active"))==1);
    assert(atof(column(header,row,"host_vx"))==0);
    assert(fabs(atof(column(header,row,"body_vx"))-.2)<1e-7);
    assert(atof(column(header,row,"incoming_fx"))==-4);
    assert(atof(column(header,row,"prepared_fx"))==8);
    assert(atof(column(header,row,"wheel2_omega"))==45);
    assert(atof(column(header,row,"wheel2_torque"))==3);
    assert(atof(column(header,row,"wheel2_load"))==0);
    assert(atoi(column(header,row,"wheel2_contact"))==1);
    assert(atoi(column(header,row,"wheel0_locked"))==1);
    assert(atof(column(header,row,"ground_fraction"))==.5);
    assert(atoi(column(header,row,"walls"))==1);
    assert(atoi(column(header,row,"rails"))==2);
    assert(atof(column(header,row,"vehicle_contact"))==3);
    assert(fclose(f)==0);
    puts("drivetrain trace: schema, force/host distinctions and immutable state passed");
    return 0;
}
