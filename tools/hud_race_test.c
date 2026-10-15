#include "hud_race.h"
#include "race.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void timing(void) {
    HudRaceClock c={0};
    for(int i=0;i<180;i++)hud_race_clock_step(&c,4083,0,1./60);
    assert(c.elapsed==0);
    for(int i=0;i<600;i++)hud_race_clock_step(&c,4083,1,1./60);
    assert(fabs(c.elapsed-10)<1e-9);
    double saved=c.elapsed;
    for(int i=0;i<60;i++)hud_race_clock_step(&c,4083,2,1./60);
    assert(c.elapsed==saved);
    hud_race_clock_step(&c,4083,0,1./60);assert(c.elapsed==0);
    hud_race_clock_step(&c,4083,1,.25);assert(c.elapsed==.25);
    hud_race_clock_step(&c,4084,1,.125);assert(c.elapsed==.125);
    hud_race_clock_step(&c,4084,1,NAN);assert(c.elapsed==.125);
    hud_race_clock_step(&c,-1,3,.1);assert(!c.active && c.elapsed==0);
    char out[64];assert(hud_race_time(123.45,out)&&!strcmp(out,"02:03.45"));
    assert(hud_race_time(0,out)&&!strcmp(out,"00:00.00"));
    assert(!hud_race_time(NAN,out)&&!out[0]);assert(!hud_race_time(-1,out));
    assert(!hud_race_time(1e30,out));
}
static void fields(void) {
    assert(!strcmp(hud_race_package(N2_RACE_CIRCUIT),"HUD_SingleRace.fng"));
    assert(!strcmp(hud_race_package(N2_RACE_SPRINT),"HUD_SingleRace.fng"));
    assert(!strcmp(hud_race_package(N2_RACE_STREETX),"HUD_Short_Track.fng"));
    assert(!strcmp(hud_race_package(N2_RACE_DRAG),"HUD_Drag.fng"));
    assert(!strcmp(hud_race_package(N2_RACE_DRIFT),"HUD_Drift.fng"));
    HudState s={0};char out[64];assert(!hud_race_text(&s,HR_POSITION,out));
    s.racing=1;s.position=2;s.cars=4;
    assert(hud_race_text(&s,HR_POSITION,out)&&!strcmp(out,"2"));
    assert(hud_race_text(&s,HR_ORDINAL,out)&&!strcmp(out,"nd"));
    s.position=11;s.cars=12;assert(hud_race_text(&s,HR_ORDINAL,out)&&!strcmp(out,"th"));
    s.position=13;s.cars=12;assert(!hud_race_text(&s,HR_POSITION,out));
    s.circuit=1;s.lap=2;s.laps=3;assert(hud_race_text(&s,HR_LAP,out)&&!strcmp(out,"2"));
    s.lap=4;assert(hud_race_text(&s,HR_LAP,out)&&!strcmp(out,"3"));
    s.circuit=0;s.sprint_progress=.426f;
    assert(hud_race_text(&s,HR_LAP_LABEL,out)&&!strcmp(out,"Complete"));
    assert(hud_race_text(&s,HR_LAP,out)&&!strcmp(out,"42%"));
    assert(!hud_race_text(&s,HR_LAPS,out));
    s.sprint_progress=NAN;assert(!hud_race_text(&s,HR_LAP,out));
    s.race_kind=N2_RACE_DRIFT;s.drift_chain=123;s.drift_score=456;
    assert(hud_race_text(&s,HR_DRIFT_CHAIN,out)&&!strcmp(out,"123"));
    assert(hud_race_text(&s,HR_DRIFT_TOTAL,out)&&!strcmp(out,"456"));
    s.race_kind=N2_RACE_CIRCUIT;assert(!hud_race_text(&s,HR_DRIFT_CHAIN,out));
    s.race_phase=HUD_RACE_COUNTDOWN;s.countdown_seconds=2.1;
    assert(hud_race_text(&s,HR_BANNER,out)&&!strcmp(out,"3"));
    s.race_phase=HUD_RACE_RUNNING;s.have_race_time=1;s.elapsed_seconds=.1;
    assert(hud_race_text(&s,HR_BANNER,out)&&!strcmp(out,"GO!"));
    s.elapsed_seconds=1;assert(!hud_race_text(&s,HR_BANNER,out));
    s.race_phase=HUD_RACE_FINISHED;s.position=1;s.cars=4;
    assert(hud_race_text(&s,HR_BANNER,out)&&!strcmp(out,"WINNER"));
    s.race_phase=HUD_RACE_FAILED;s.engine_failed=1;
    assert(hud_race_text(&s,HR_BANNER,out)&&!strcmp(out,"ENGINE BLOWN"));
    s.have_race_time=0;assert(!hud_race_text(&s,HR_TIME,out));
}
static void put32(unsigned char*p,unsigned x){for(int i=0;i<4;i++)p[i]=(unsigned char)(x>>(8*i));}
static void packages(void){
 /* Entirely synthetic, empty named package: no retail bytes in fixtures. */
 unsigned char raw[128]={0};memcpy(raw,"FEn\xe7",4);memcpy(raw+8,"PkHd",4);
 const char name[]="SYNTH.fng";unsigned body=24+sizeof name,total=16+body;
 put32(raw+4,total-8);put32(raw+12,body);put32(raw+32,sizeof name);memcpy(raw+40,name,sizeof name);
 unsigned char chunk[512]={0};put32(chunk,0x30203);put32(chunk+4,total);memcpy(chunk+8,raw,total);
 HaPackage p={0};assert(hud_race_package_read(chunk,total+8,name,&p));ha_free(&p);
 assert(!hud_race_package_read(chunk,total+8,"OTHER.fng",&p));
 memcpy(chunk+total+8,chunk,total+8);assert(!hud_race_package_read(chunk,(total+8)*2,name,&p));
 /* Encode literals using the existing JDLZ format's two flag streams. */
 unsigned char encoded[256]={0};memcpy(encoded,"JDLZ",4);encoded[4]=2;encoded[5]=16;put32(encoded+8,total);
 unsigned at=16;for(unsigned i=0;i<total;i++){if(i%8==0)encoded[at++]=0;if(i==0)encoded[at++]=0;encoded[at++]=raw[i];}
 put32(encoded+12,at);memset(chunk,0,sizeof chunk);put32(chunk,0x30210);put32(chunk+4,at+4);memcpy(chunk+12,encoded,at);
 assert(hud_race_package_read(chunk,at+12,name,&p));ha_free(&p);
 assert(!hud_race_package_read(chunk,at+11,name,&p));
 encoded[8]=255;encoded[9]=255;encoded[10]=255;memcpy(chunk+12,encoded,at);
 assert(!hud_race_package_read(chunk,at+12,name,&p));
}
static void map_inverse(void){
 HaNode needle={0};needle.quaternion[2]=(float)sin(.35);needle.quaternion[3]=(float)cos(.35);
 assert(fabs(hud_race_drag_angle(&needle,0)+1.4)<1e-6);
 assert(fabs(hud_race_drag_angle(&needle,5000)+.7)<1e-6);
 assert(hud_race_drag_angle(&needle,10000)==0);
 assert(hud_race_drag_angle(&needle,12000)==0);

 for(int aspect=0;aspect<2;aspect++)for(int h=0;h<16;h++){
  HaMapFrame f={.enabled=1,.width=aspect?1280:960,.height=720,.center={-240,140},.basis={.9,.1,-.1,.9},.size={128,128},.world={-200,30},.window=500,.heading=h*.4};
  double point[2],world[2];assert(hud_race_map_point(&f,-190,45,point));
  double scale=fmin(f.width/640.,f.height/480.);
  assert(ha_map_pick(&f,f.width*.5+point[0]*scale,f.height*.5+point[1]*scale,1,world));
  assert(fabs(world[0]+190)<1e-8&&fabs(world[1]-45)<1e-8);
 }
 HaMapFrame f={0};double out[2];assert(!hud_race_map_point(&f,0,0,out));f.enabled=1;f.window=NAN;assert(!hud_race_map_point(&f,0,0,out));
}
static void package_rejections(void) {
    HaPackage p={0};unsigned char bytes[32]={0};
    for(size_t n=0;n<sizeof bytes;n++)assert(!hud_race_package_read(bytes,n,"SYNTH.fng",&p));
    unsigned char bad[]={0x10,2,3,0,20,0,0,0,0,0,0,0,'J','D','L','Z',2,0x10,0,0,255,255,255,127,16,0,0,0};
    assert(!hud_race_package_read(bad,sizeof bad,"SYNTH.fng",&p));
    memset(bad,0,sizeof bad);put32(bad,0x30210);put32(bad+4,20);memcpy(bad+12,"HUFF",4);
    assert(!hud_race_package_read(bad,sizeof bad,"SYNTH.fng",&p));
    unsigned char huff[36]={0};put32(huff,0x30210);put32(huff+4,28);memcpy(huff+12,"HUFF",4);
    huff[28]=0x30;huff[29]=0xfb;huff[30]=huff[31]=huff[32]=255;
    assert(!hud_race_package_read(huff,sizeof huff,"SYNTH.fng",&p));
    assert(!p.nodes && !p.resources);
}
int main(void) { timing();fields();packages();package_rejections();map_inverse();puts("hud_race_test: PASS"); }
