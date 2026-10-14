#ifndef OPENUG2_UG2_TRACK_COLLISION_H
#define OPENUG2_UG2_TRACK_COLLISION_H
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "asset_chunks.h"

typedef struct {
    unsigned char records[256][32]; /* Preserve activation bytes and count-like fields. */
    size_t count;
    uint64_t source_offset;
} UG2TrackGroups;
typedef struct { UG2TrackGroups result; int seen,valid; } UG2TCGroupsRead;
static int ug2_tc_groups_visit(const unsigned char *bytes,uint32_t id,long begin,long end,void *context) {
    UG2TCGroupsRead *read=context;
    if(!read->valid)return 0;
    if(id!=0x3412f)return id>>28==8;
    if(read->seen++ || !(end-begin) || (end-begin)%32 || (end-begin)/32>256)goto invalid;
    read->result.count=(size_t)(end-begin)/32;read->result.source_offset=(uint64_t)begin;
    for(size_t k=0;k<read->result.count;k++) {
        const unsigned char *r=bytes+begin+32*k,*name=r+9;
        const unsigned char *stop=memchr(name,0,23);
        if(!stop || stop==name)goto invalid;
        uint32_t hash=0xffffffffu;
        for(const unsigned char *p=name;p<stop;p++) {
            if(*p<32 || *p>126)goto invalid;
            hash=hash*33u+*p;
        }
        if(k ? hash!=asset_chunk_u32(r) : (asset_chunk_u32(r) || strcmp((const char*)name,"NONE")))goto invalid;
        memcpy(read->result.records[k],r,32);
    }
    return 0;
invalid:
    read->valid=0;return 0;
}
/* Success copies owned rows; malformed input retains the previous table. */
static int ug2_track_groups_read(const unsigned char *bytes,size_t size,UG2TrackGroups *out) {
    if(!bytes || !out || size>LONG_MAX)return 0;
    UG2TCGroupsRead read={0};read.valid=1;
    if(!asset_chunks_walk(bytes,0,(long)size,ug2_tc_groups_visit,&read) || !read.valid || read.seen!=1)return 0;
    *out=read.result;return 1;
}

typedef struct {
    float corners[4][3];
    uint32_t section;
    uint64_t source_offset;
    unsigned char metadata[8]; /* +0 indexes activation; other contact/surface bytes unknown. */
} UG2TrackPolygon;
typedef struct {
    UG2TrackPolygon *polygons;
    size_t count, groups, total;
    UG2TrackGroups activation;
} UG2TrackCollision;
static void ug2_track_collision_free(UG2TrackCollision *data) {
    if(data){free(data->polygons);*data=(UG2TrackCollision){0};}
}
static unsigned ug2_tc_u16(const unsigned char *p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static int ug2_tc_s16(const unsigned char *p){unsigned n=ug2_tc_u16(p);return n>32767?(int)n-65536:(int)n;}
static float ug2_tc_f32(const unsigned char *p){uint32_t bits=asset_chunk_u32(p);float f;memcpy(&f,&bits,4);return f;}
typedef struct {
    UG2TrackCollision result;
    const float *bounds;
    size_t capacity;
    int valid;
} UG2TCRead;
static int ug2_tc_group(const unsigned char *bytes,long begin,long end,UG2TCRead *read) {
    long leaves[4]={0},sizes[4]={0};unsigned seen=0;
    for(long p=begin;p<end;) {
        if(end-p<8)return 0;
        uint32_t id=asset_chunk_u32(bytes+p),size=asset_chunk_u32(bytes+p+4);
        if((uint64_t)size>(uint64_t)(end-p-8))return 0;
        int slot=id==0x34131?0:id==0x34134?1:id==0x34136?2:id==0x34137?3:-1;
        if(slot>=0) {
            if(seen&(1u<<slot))return 0;
            seen|=1u<<slot;leaves[slot]=p+8;sizes[slot]=size;
        } else if(id)return 0;
        p+=8+(long)size;
    }
    if(seen!=15 || sizes[0]!=48 || sizes[1]%32 || sizes[2]%4 || sizes[3]%4)return 0;
    const unsigned char *header=bytes+leaves[0];
    size_t nx=ug2_tc_u16(header+4),ny=ug2_tc_u16(header+6),count=(size_t)sizes[1]/32,refs=(size_t)sizes[2]/4;
    if((uint64_t)nx*ny!=(uint64_t)sizes[3]/4)return 0;
    for(int k=0;k<2;k++)if(!isfinite(ug2_tc_f32(header+8+4*k)))return 0;
    float low[3],high[3];
    for(int k=0;k<3;k++) {
        low[k]=ug2_tc_f32(header+16+4*k);high[k]=ug2_tc_f32(header+32+4*k);
        /* Empty sections use +FLT_MAX/-FLT_MAX instead of geometric bounds. */
        if(!isfinite(low[k]) || !isfinite(high[k]) || (count && low[k]>high[k]))return 0;
    }
    size_t cursor=0;
    for(long p=leaves[3];p<leaves[3]+sizes[3];p+=4) {
        size_t start=ug2_tc_u16(bytes+p),n=ug2_tc_u16(bytes+p+2);
        if(start!=cursor || n>refs-cursor)return 0;
        cursor+=n;
    }
    if(cursor!=refs)return 0;
    for(long p=leaves[2];p<leaves[2]+sizes[2];p+=4)if(ug2_tc_u16(bytes+p)>=count)return 0;
    if(count>SIZE_MAX-read->result.total)return 0;
    read->result.groups++;read->result.total+=count;
    for(long p=leaves[1];p<leaves[1]+sizes[1];p+=32) {
        if(bytes[p]>=read->result.activation.count || ug2_tc_u16(bytes+p+6)!=65535)return 0;
        UG2TrackPolygon poly={0};poly.section=asset_chunk_u32(header);poly.source_offset=(uint64_t)p;
        memcpy(poly.metadata,bytes+p,8);int base=ug2_tc_s16(bytes+p+4);
        float x0=INFINITY,y0=INFINITY,x1=-INFINITY,y1=-INFINITY;
        for(int v=0;v<4;v++) {
            float *q=poly.corners[v];q[0]=ug2_tc_s16(bytes+p+8+2*v)/8.f;
            q[1]=ug2_tc_s16(bytes+p+16+2*v)/8.f;q[2]=base+ug2_tc_s16(bytes+p+24+2*v)/256.f;
            for(int k=0;k<3;k++)if(q[k]<low[k]-.001f || q[k]>high[k]+.001f)return 0;
            x0=fminf(x0,q[0]);x1=fmaxf(x1,q[0]);y0=fminf(y0,q[1]);y1=fmaxf(y1,q[1]);
        }
        if(read->bounds && (x1<read->bounds[0] || y1<read->bounds[1] || x0>read->bounds[2] || y0>read->bounds[3]))continue;
        if(read->result.count==read->capacity) {
            size_t cap=read->capacity?read->capacity*2:256;
            if(cap<read->capacity || cap>SIZE_MAX/sizeof poly)return 0;
            UG2TrackPolygon *grown=realloc(read->result.polygons,cap*sizeof poly);
            if(!grown)return 0;
            read->result.polygons=grown;read->capacity=cap;
        }
        read->result.polygons[read->result.count++]=poly;
    }
    return 1;
}
static int ug2_tc_visit(const unsigned char *bytes,uint32_t id,long begin,long end,void *context) {
    UG2TCRead *read=context;
    if(!read->valid)return 0;
    if(id==0x80034130){read->valid=ug2_tc_group(bytes,begin,end,read);return 0;}
    return id>>28==8;
}
/* bounds is optional XY min/max; success replaces an initialized owner.
   Failure retains the previous owner, including after a malformed valid prefix. */
static int ug2_track_collision_read(const unsigned char *bytes,size_t size,
                                    const UG2TrackGroups *activation,const float bounds[4],UG2TrackCollision *out) {
    if(!bytes || !out || !activation || !activation->count || activation->count>256 || size>LONG_MAX || sizeof(float)!=4)return 0;
    if(bounds) {
        for(int k=0;k<4;k++)if(!isfinite(bounds[k]))return 0;
        if(bounds[0]>bounds[2] || bounds[1]>bounds[3])return 0;
    }
    UG2TCRead read={0};read.bounds=bounds;read.valid=1;
    read.result.activation=*activation;
    int valid=asset_chunks_walk(bytes,0,(long)size,ug2_tc_visit,&read) && read.valid && read.result.groups;
    if(!valid){ug2_track_collision_free(&read.result);return 0;}
    ug2_track_collision_free(out);*out=read.result;return 1;
}
#endif
