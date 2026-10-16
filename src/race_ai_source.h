#ifndef OPENUG2_RACE_AI_SOURCE_H
#define OPENUG2_RACE_AI_SOURCE_H
#include "asset_chunks.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define RACE_AI_SOURCE_EVENTS 256
#define RACE_AI_SOURCE_DRIVERS 5
#define RACE_AI_SOURCE_SKILLS 10
#define RACE_AI_SOURCE_POINTS 11

typedef struct {
    char name[32];
    uint16_t stats_multiplier; /* retained only; no verified conversion */
    uint32_t preset_hash;
    unsigned char skill[3],catch_up;
} RaceAiSourceDriver;
typedef struct {
    char name[64];
    uint32_t key;
    uint16_t track[4];
    int count;
    RaceAiSourceDriver driver[RACE_AI_SOURCE_DRIVERS];
} RaceAiSourceEvent;
typedef struct {
    float catchup[RACE_AI_SOURCE_POINTS][2],skill_scale[RACE_AI_SOURCE_SKILLS];
    RaceAiSourceEvent event[RACE_AI_SOURCE_EVENTS];
    int count;
} RaceAiSource;

static uint32_t race_ai_source_hash(const char *s){
    uint32_t h=0xffffffffu;for(;*s;s++)h=33*h+(unsigned char)*s;return h;
}
static unsigned race_ai_source_u16(const unsigned char *p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static float race_ai_source_float(const unsigned char *p){
    uint32_t u=asset_chunk_u32(p);float f;memcpy(&f,&u,4);return f;
}
static int race_ai_source_string(const unsigned char *d,long beg,long end,unsigned off,char *out,size_t cap){
    if(off>=(unsigned long)(end-beg) || (off && d[beg+off-1]))return 0;
    long at=beg+off;const unsigned char *stop=(const unsigned char *)memchr(d+at,0,(size_t)(end-at));
    if(!stop || stop==d+at || (size_t)(stop-d-at)>=cap)return 0;
    for(const unsigned char *s=d+at;s<stop;s++)if(*s<32 || *s>126)return 0;
    memcpy(out,d+at,(size_t)(stop-d-at));out[stop-d-at]=0;return 1;
}
typedef struct {long pool,end,records,stop;int pools,leaves;} RaceAiSourceParts;
static int race_ai_source_parts(const unsigned char *d,uint32_t tag,long beg,long end,void *context){
    (void)d;RaceAiSourceParts *p=(RaceAiSourceParts *)context;
    if(tag==0x34a1d){p->pool=beg;p->end=end;p->pools++;}
    if(tag==0x34a11){p->records=beg;p->stop=end;p->leaves++;}
    return 0;
}
typedef struct {RaceAiSource *out;int ok,tuning,careers;} RaceAiSourceRead;
static int race_ai_source_visit(const unsigned char *d,uint32_t tag,long beg,long end,void *context){
    RaceAiSourceRead *r=(RaceAiSourceRead *)context;RaceAiSource *out=r->out;
    if(!r->ok)return 0;
    if(tag==0x34b00){
        if(++r->tuning!=1 || end-beg!=2784){r->ok=0;return 0;}
        /* Adaptation: curve zero and row +0x18 supply pace adjustments.
           These roles are inferred; they are not a recovered retail solver. */
        for(int j=0;j<RACE_AI_SOURCE_POINTS;j++)for(int axis=0;axis<2;axis++){
            float f=race_ai_source_float(d+beg+8*j+4*axis);
            if(!isfinite(f) || (axis && (f<=-1 || f>1)) ||
               (!axis && j && f<=out->catchup[j-1][0]))r->ok=0;
            out->catchup[j][axis]=f;
        }
        for(int j=0;j<RACE_AI_SOURCE_SKILLS;j++){
            float f=race_ai_source_float(d+beg+0x520+104*j+24);
            if(!isfinite(f) || f<=0 || f>2)r->ok=0;
            out->skill_scale[j]=f;
        }
        return 0;
    }
    if(tag==0x80034a10u){
        RaceAiSourceParts p={0};r->careers++;
        if(!asset_chunks_walk(d,beg,end,race_ai_source_parts,&p) || p.pools!=1 || p.leaves!=1 ||
           (p.stop-p.records)%136){r->ok=0;return 0;}
        for(long at=p.records;at<p.stop;at+=136){
            if(out->count>=RACE_AI_SOURCE_EVENTS){r->ok=0;break;}
            RaceAiSourceEvent *e=out->event+out->count;
            e->count=d[at+124];e->key=asset_chunk_u32(d+at+8);
            if(e->count>RACE_AI_SOURCE_DRIVERS || d[at+126]>4 ||
               !race_ai_source_string(d,p.pool,p.end,race_ai_source_u16(d+at),e->name,sizeof e->name) ||
               e->key!=race_ai_source_hash(e->name)){r->ok=0;break;}
            for(int j=0;j<4;j++)e->track[j]=(uint16_t)race_ai_source_u16(d+at+24+4*j);
            for(int j=0;j<e->count;j++){
                const unsigned char *b=d+at+64+12*j;RaceAiSourceDriver *v=e->driver+j;
                if(!race_ai_source_string(d,p.pool,p.end,race_ai_source_u16(b),v->name,sizeof v->name))r->ok=0;
                v->stats_multiplier=(uint16_t)race_ai_source_u16(b+2);v->preset_hash=asset_chunk_u32(b+4);
                for(int k=0;k<3;k++){v->skill[k]=b[8+k];if(v->skill[k]>=RACE_AI_SOURCE_SKILLS)r->ok=0;}
                v->catch_up=b[11];if(v->catch_up>1)r->ok=0;
            }
            if(!r->ok)break;
            out->count++;
        }
        return 0;
    }
    return (tag>>28)==8;
}
/* Transactional: malformed/missing/oversized data leaves the caller unchanged. */
static int race_ai_source_read(const unsigned char *d,long len,RaceAiSource *out){
    if(!d || len<8 || !out)return 0;
    RaceAiSource *tmp=(RaceAiSource *)calloc(1,sizeof *tmp);if(!tmp)return 0;
    RaceAiSourceRead r={tmp,1,0,0};
    int ok=asset_chunks_walk(d,0,len,race_ai_source_visit,&r) && r.ok && r.tuning==1 && r.careers && tmp->count;
    if(ok)*out=*tmp;
    free(tmp);return ok;
}
static int race_ai_source_matches(const RaceAiSourceEvent *e,int track,int drivers){
    if(!e || track<=0 || e->count!=drivers || drivers<1 || drivers>RACE_AI_SOURCE_DRIVERS)return 0;
    for(int j=0;j<4;j++)if(e->track[j]==track)return 1;
    return 0;
}
/* Quick-race policy: first source-order match unless the user selects a variant. */
static int race_ai_source_select(const RaceAiSource *s,int track,int drivers,int requested){
    if(!s || s->count<1 || s->count>RACE_AI_SOURCE_EVENTS)return -1;
    if(requested>=0)return requested<s->count && race_ai_source_matches(s->event+requested,track,drivers)?requested:-1;
    for(int j=0;j<s->count;j++)if(race_ai_source_matches(s->event+j,track,drivers))return j;
    return -1;
}
static float race_ai_source_curve(const RaceAiSource *s,float gap){
    if(!s || !isfinite(gap))return 0;
    if(gap<=s->catchup[0][0])return s->catchup[0][1];
    for(int j=1;j<RACE_AI_SOURCE_POINTS;j++)if(gap<s->catchup[j][0]){
        float span=s->catchup[j][0]-s->catchup[j-1][0];if(span<=0)return 0;
        float t=(gap-s->catchup[j-1][0])/span;
        return s->catchup[j-1][1]+t*(s->catchup[j][1]-s->catchup[j-1][1]);
    }
    return s->catchup[RACE_AI_SOURCE_POINTS-1][1];
}
#endif
