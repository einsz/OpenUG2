/* Synthetic format fixtures only; no original game bytes. */
#include "race_ai_source.h"
#include <assert.h>
#include <stdio.h>

static void u16(unsigned char *p,unsigned v){p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);}
static void u32(unsigned char *p,unsigned v){for(int k=0;k<4;k++)p[k]=(unsigned char)(v>>(8*k));}
static void f32(unsigned char *p,float v){memcpy(p,&v,4);}
enum { TUNING=2784, POOL=64, RECORD=136, CAREER=8+8+POOL+8+RECORD,
       SIZE=8+TUNING+2*CAREER };
static void fixture(unsigned char *b){
    memset(b,0,SIZE);u32(b,0x34b00);u32(b+4,TUNING);
    for(int k=0;k<11;k++){f32(b+8+8*k,-500+100.0f*k);f32(b+12+8*k,.5f-.1f*k);}
    for(int k=0;k<10;k++)f32(b+8+0x520+104*k+24,.5f+.03f*k);
    for(int c=0;c<2;c++){
        unsigned char *p=b+8+TUNING+c*CAREER;
        u32(p,0x80034a10);u32(p+4,CAREER-8);
        u32(p+8,0x34a1d);u32(p+12,POOL);
        char *s=(char *)p+16;strcpy(s+1,c?"TEST_SECOND":"TEST_FIRST");strcpy(s+20,"Driver Alpha");strcpy(s+36,"Driver Beta");
        p+=16+POOL;u32(p,0x34a11);u32(p+4,RECORD);p+=8;
        u16(p,1);u32(p+8,race_ai_source_hash(c?"TEST_SECOND":"TEST_FIRST"));u16(p+24,1234);p[126]=1;
        p[124]=3;
        for(int k=0;k<3;k++){
            unsigned char *r=p+64+12*k;u16(r,k==1?36:20);u16(r+2,400+k);u32(r+4,race_ai_source_hash("RANDOM"));
            r[8]=1+c;r[9]=4+c;r[10]=8+c;r[11]=(unsigned char)(k!=1);
        }
        /* Poison unused descriptor bytes. The count is authoritative. */
        memset(p+100,0xff,24);
    }
}
static void rejected(const unsigned char *b,long size){
    RaceAiSource out={0};out.count=99;out.skill_scale[0]=42;
    assert(!race_ai_source_read(b,size,&out));assert(out.count==99 && out.skill_scale[0]==42);
}
int main(void){
    unsigned char b[SIZE];fixture(b);RaceAiSource out={0};
    assert(race_ai_source_read(b,SIZE,&out) && out.count==2);
    assert(!strcmp(out.event[0].name,"TEST_FIRST") && !strcmp(out.event[1].name,"TEST_SECOND"));
    assert(!strcmp(out.event[0].driver[1].name,"Driver Beta"));
    assert(out.event[0].driver[1].stats_multiplier==401 && !out.event[0].driver[1].catch_up);
    assert(out.event[1].driver[0].skill[2]==9 && out.event[0].track[0]==1234);
    assert(fabsf(out.skill_scale[4]-.62f)<1e-6f);
    assert(fabsf(race_ai_source_curve(&out,-250)-.25f)<1e-6f);
    assert(fabsf(race_ai_source_curve(&out,250)+.25f)<1e-6f);
    assert(race_ai_source_curve(&out,-10000)==out.catchup[0][1]);
    assert(race_ai_source_curve(&out,10000)==out.catchup[10][1]);
    assert(race_ai_source_curve(&out,NAN)==0);
    assert(race_ai_source_select(&out,1234,3,-1)==0);
    assert(race_ai_source_select(&out,1234,3,1)==1);
    assert(race_ai_source_select(&out,1234,5,-1)==-1);
    assert(race_ai_source_select(&out,5555,3,0)==-1);
    assert(race_ai_source_select(&out,1234,3,2)==-1);
    assert(race_ai_source_select(NULL,1234,3,-1)==-1);
    unsigned char duplicate[SIZE+8+TUNING];memcpy(duplicate,b,SIZE);memcpy(duplicate+SIZE,b,8+TUNING);
    rejected(duplicate,sizeof duplicate);
    size_t large_size=8+TUNING+(RACE_AI_SOURCE_EVENTS+1)*CAREER;
    unsigned char *large=(unsigned char *)malloc(large_size);assert(large);memcpy(large,b,8+TUNING);
    for(int j=0;j<=RACE_AI_SOURCE_EVENTS;j++)memcpy(large+8+TUNING+j*CAREER,b+8+TUNING,CAREER);
    rejected(large,(long)large_size);free(large);
    rejected(NULL,0);rejected(b,-1);
    for(int cut=0;cut<SIZE;cut++)if(cut!=8+TUNING+CAREER)rejected(b,cut);
    assert(race_ai_source_read(b,8+TUNING+CAREER,&out) && out.count==1);
    unsigned char *r=b+8+TUNING+16+POOL+8;
    r[124]=6;rejected(b,SIZE);fixture(b);
    r[64+8]=10;rejected(b,SIZE);fixture(b);
    r[64+11]=2;rejected(b,SIZE);fixture(b);
    u16(r+64,POOL);rejected(b,SIZE);fixture(b);
    u16(r+64,21);rejected(b,SIZE);fixture(b);
    r[8]^=1;rejected(b,SIZE);fixture(b);
    b[8+TUNING+16+POOL-1]='!'; /* no active name crosses this padding: valid */
    assert(race_ai_source_read(b,SIZE,&out));fixture(b);
    f32(b+8+8*3,NAN);rejected(b,SIZE);fixture(b);
    f32(b+8+8*3,-500);rejected(b,SIZE);fixture(b);
    f32(b+8+0x520+24,0);rejected(b,SIZE);fixture(b);
    f32(b+8+0x520+24,INFINITY);rejected(b,SIZE);fixture(b);
    u32(b+4,TUNING-4);rejected(b,SIZE);fixture(b);
    u32(b+8+TUNING+4,CAREER);rejected(b,SIZE);fixture(b);
    u32(b+8+TUNING+8,0x34a11);rejected(b,SIZE);fixture(b);
    u32(b,0x34b01);rejected(b,SIZE);
    puts("race AI source: transactional framing, scoped pools, difficulty/catch-up, inactive slots, selectors and curve sampling PASS");
    return 0;
}
