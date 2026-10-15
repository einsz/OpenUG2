/* Independent narrow PS2 controller decoder and ISO loader; SPEC 20.4.
 * ISO directory layout: ECMA-119 sections 8.4 and 9.1. */
#include "ugui_controller.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
static const char *names[19]={"PS2_TRIANGLE","PS2_RIGHTANALOG","PS2_DPADUPDOWN","PS2_L1","PS2_L2","PS2_L3","PS2_R1","PS2_R2","PS2_R3","PS2_LEFTANALOG","PS2_CIRCLE","PS2_CROSS","PS2_DPADD","PS2_DPADL","PS2_DPADR","PS2_DPADU","PS2_START","PS2_SELECT","PS2_SQUARE"};
static const uint32_t keys[19]={0x1ac94ca9,0x59efa983,0x5e800209,0x739d7070,0x739d7071,0x739d7072,0x739d7136,0x739d7137,0x739d7138,0xbb9b93f0,0xd9a22505,0xe795717d,0xe7a633b0,0xe7a633b8,0xe7a633be,0xe7a633c1,0xe8b7d6a1,0xfea90a33,0xff871084};
static uint32_t u32(const uint8_t *b){return b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;}
static unsigned u16(const uint8_t *b){return b[0]|(unsigned)b[1]<<8;}
const char *ug_controller_icon_name(int i){return i>=0&&i<19?names[i]:NULL;}
uint32_t ug_controller_icon_key(int i){return i>=0&&i<19?keys[i]:0;}
static const char *xbox_names[23]={"black button","direction pad up","start","left stick","left stick left/right","white button","right trigger","left trigger","back","direction pad","A","B","X","Y","direction pad up/down","direction pad down","direction pad left","right stick up","left stick up","right stick down","direction pad left/right","right stick","direction pad right"};
static const uint32_t xbox_keys[23]={0x09b6db77,0x0e7b631c,0x285718ed,0x3c5c25bc,0x3fd3b7a3,0x4b47bc3b,0x523452d0,0x68fdf2fd,0x85104670,0x85119eb8,0x8522e0a0,0x8522e0a1,0x8522e0b7,0x8522e0b8,0x8a3f71d3,0x9ad74eef,0x9adb8542,0xad32efd4,0xc3fc9001,0xc5a4e9a7,0xd0be1b1f,0xf2c274cf,0xf6baf515};
static const uint32_t localized_keys[24]={0x1c4bc1c4u,0x1e9510b6u,0x2fffeef1u,0x3909c9a6u,0x40496c1eu,0x495346d3u,0x7b118611u,0x8ba30e18u,0x91139ff0u,0x9579b4a5u,0x9a1d7aa5u,0xbcc1c032u,0xc891edddu,0xc8ef539cu,0xcc3d4ff5u,0xd19bc892u,0xd2296cc8u,0xd5472aaau,0xdb33477du,0xdc86cd22u,0xe590a7d7u,0xeb5476c4u,0xf45e5179u,0xf74d0529u};
static const char *localized_names[24]={"french 1c4bc1c4","french 1e9510b6","italian 2fffeef1","swedish 3909c9a6","italian 40496c1e","swedish 495346d3","french 7b118611","french 8ba30e18","italian 91139ff0","french 9579b4a5","swedish 9a1d7aa5","french bcc1c032","italian c891eddd","french c8ef539c","italian cc3d4ff5","swedish d19bc892","italian d2296cc8","swedish d5472aaa","swedish db33477d","italian dc86cd22","swedish e590a7d7","italian eb5476c4","swedish f45e5179","french f74d0529"};
int ug_controller_family_icon_count(UgControllerFamily f){return f==UG_CONTROLLER_PS2?19:f==UG_CONTROLLER_XBOX?47:0;}
const char *ug_controller_family_icon_name(UgControllerFamily f,int i){
    return i>=0&&i<ug_controller_family_icon_count(f)?(f==UG_CONTROLLER_PS2?names[i]:i<23?xbox_names[i]:localized_names[i-23]):NULL;
}
uint32_t ug_controller_family_icon_key(UgControllerFamily f,int i){
    return i>=0&&i<ug_controller_family_icon_count(f)?(f==UG_CONTROLLER_PS2?keys[i]:i<23?xbox_keys[i]:localized_keys[i-23]):0;
}
int ug_controller_family_icon_texture(const UgController *p,UgControllerFamily f,int i){
    if(!p||i<0||i>=ug_controller_family_icon_count(f))return 0;
    return f==UG_CONTROLLER_PS2?p->icons[i]:p->xbox_icons[i];
}
int ug_controller_family_available(const UgController *p,UgControllerFamily f){
    return p&&(f==UG_CONTROLLER_PC||(f==UG_CONTROLLER_PS2&&p->available)||(f==UG_CONTROLLER_XBOX&&p->xbox_available));
}
void ug_controller_init(UgController *p){memset(p,0,sizeof *p);ug_controller_select(p,UG_CONTROLLER_PC);}
/* SPEC 7.3 defines these alternate spellings of the same action. This
 * normalizes an explicit D binding; it never infers a resource key. */
static const char *binding_token(const char *token){
    if(!token)return NULL;
    if(!strncmp(token,"JOY_EVENT_FENG_",15))token+=15;
    return !strcmp(token,"SELECT")?"ACCEPT":token;
}
UgVisualStatus ug_controller_select(UgController *p,UgControllerFamily f){
    p->family=f;int available=ug_controller_family_available(p,f);
    if(f==UG_CONTROLLER_PC)snprintf(p->status,sizeof p->status,"PC text-only pills; original shared UI resources required");
    else snprintf(p->status,sizeof p->status,"%s %s",f==UG_CONTROLLER_XBOX?"Xbox":"PS2",available?"original icons available; action map is demo configuration":f==UG_CONTROLLER_XBOX?"source unavailable (provide --xbox-iso)":"source unavailable (provide --ps2-iso)");
    if(f!=UG_CONTROLLER_PC&&p->folder_active){
        int loaded=0,total=0;for(size_t i=0;i<p->folder_count;i++)if(p->folder_records[i].family==f){total++;if(p->folder_records[i].texture)loaded++;}
        snprintf(p->status,sizeof p->status,"%s TEST-ONLY PNG folder: %d / %d available%s",f==UG_CONTROLLER_XBOX?"Xbox":"PS2",loaded,total,p->folder_index_failed?"; index configuration failure":!loaded?"; source unavailable":"; explicit demo mappings");
    }
    return available?UG_VIS_OK:UG_VIS_UNAVAILABLE;
}
UgVisualStatus ug_controller_bind(UgController *p,const char *token,uint32_t key){
    token=binding_token(token);
    if(!p||!token||!*token||strlen(token)>=40)return UG_VIS_UNBOUND;
    /* Legacy callers can configure PS2 keys while PC is active. Console
     * callers bind only a key belonging to their explicitly selected family. */
    UgControllerFamily f=p->family==UG_CONTROLLER_PC?UG_CONTROLLER_PS2:p->family;
    int found=0;for(int i=0;i<ug_controller_family_icon_count(f);i++)if(ug_controller_family_icon_key(f,i)==key)found=1;

    for(int i=0;i<p->nbindings;i++)if(p->binding[i].family==f&&!strcmp(p->binding[i].token,token)){p->binding[i].key=key;return found?UG_VIS_OK:UG_VIS_UNAVAILABLE;}
    if(p->nbindings>=32)return UG_VIS_UNBOUND;
    UgControllerBinding *b=&p->binding[p->nbindings++];snprintf(b->token,sizeof b->token,"%s",token);b->key=key;b->family=f;return found?UG_VIS_OK:UG_VIS_UNAVAILABLE;
}
UgVisualStatus ug_controller_token(const UgController *p,const char *token,int *tex){
    token=binding_token(token);
    if(tex)*tex=0;
    if(!p||!token)return UG_VIS_UNBOUND;
    uint32_t key=0;int configured=0;
    for(int i=0;i<p->nbindings;i++)if(!strcmp(p->binding[i].token,token)){
        configured=1;if(p->binding[i].family==p->family)key=p->binding[i].key;
    }
    if(!configured)return UG_VIS_UNBOUND;
    if(!key||!ug_controller_family_available(p,p->family))return UG_VIS_UNAVAILABLE;
    for(int i=0;i<ug_controller_family_icon_count(p->family);i++)if(ug_controller_family_icon_key(p->family,i)==key){
        int handle=ug_controller_family_icon_texture(p,p->family,i);if(handle){if(tex)*tex=handle;return UG_VIS_OK;}
    }
    return UG_VIS_UNAVAILABLE;
}
unsigned ug_ps2_icon_address(unsigned x,unsigned y){
    return 32*(y&~15u)+2*(x&~15u)+64*((((y&~3u)>>1)+(y&1))&7)+4*((x+4*(((y+2)>>2)&1))&7)+((y>>1)&1)+((x>>2)&2);
}
int ug_ps2_icon_decode(const uint8_t *pixels,size_t plen,const uint8_t *pal,size_t clen,uint8_t rgba[4096]){
    if(!pixels||!pal||!rgba||plen!=512||clen!=64)return 0;
    for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++){
        unsigned a=ug_ps2_icon_address(x,y),k=(pixels[a/2]>>(4*(a&1)))&15;
        uint8_t *o=rgba+4*(y*32+x);memcpy(o,pal+4*k,3);o[3]=pal[4*k+3]>=128?255:2*pal[4*k+3];
    }
    return 1;
}
typedef struct {const uint8_t *info,*records,*keylist,*desc,*data;size_t nr,nk,nd,ds; int valid;} Pack;
static int walk_pack(Pack *p,const uint8_t *base,const uint8_t *data,size_t len,int depth,unsigned desc_stride){
    if(depth>8)return 0;
    for(size_t off=0;off<len;){
        if(len-off<8)return 0;
        uint32_t id=u32(data+off),n=u32(data+off+4);if(n>len-off-8)return 0;
        const uint8_t *b=data+off+8;
        if(id==0xB3310000u||id==0xB3320000u){if(!walk_pack(p,base,b,n,depth+1,desc_stride))return 0;}
        else if(id==0x33310001u){if(n!=124||p->info)return 0;p->info=b;}
        else if(id==0x33310002u){if(n%8||p->keylist)return 0;p->keylist=b;p->nk=n/8;}
        else if(id==0x33310004u){if(n%124||p->records)return 0;p->records=b;p->nr=n/124;}
        else if(id==0x33310005u){if(n%desc_stride||p->desc)return 0;p->desc=b;p->nd=n/desc_stride;}
        else if(id==0x33320002u){size_t gap=(128-((size_t)(b-base)&127))&127;if(gap>n||p->data)return 0;p->data=b+gap;p->ds=n-gap;}
        off+=8+n;
    }return 1;
}
int ug_controller_load_pack(UgController *p,UgTexLib *lib,const uint8_t *data,size_t len){
    Pack a={0};if(len<8||u32(data)!=0xB3300000u||u32(data+4)!=len-8||!walk_pack(&a,data,data+8,len-8,0,84))return 0;
    if(!a.info||u32(a.info)!=5||memcmp(a.info+32,"Global\\GlobalTexturesPSX2.tpk",30)||!a.data||!a.records||!a.keylist||!a.desc||a.nr!=a.nk||a.nr!=a.nd)return 0;
    const uint8_t *recs[19]={0};for(size_t i=0;i<a.nr;i++){
        const uint8_t *r=a.records+124*i;uint32_t key=u32(r+36);if(key!=u32(a.keylist+8*i))return 0;
        for(int j=0;j<19;j++)if(key==keys[j]){if(recs[j])return 0;recs[j]=r;}
    }
    uint8_t *rgba[19]={0};int ok=1;
    for(int i=0;i<19;i++){
        const uint8_t *r=recs[i];if(!r){ok=0;break;}
        uint32_t off=u32(r+48),po=u32(r+52);
        if(u16(r+68)!=32||u16(r+70)!=32||r[74]!=4||r[78]!=1||u32(r+56)!=512||u32(r+60)!=64||off>a.ds||512>a.ds-off||po>a.ds||64>a.ds-po){ok=0;break;}
        rgba[i]=malloc(4096);if(!rgba[i]||!ug_ps2_icon_decode(a.data+off,512,a.data+po,64,rgba[i])){ok=0;break;}
    }
    if(!ok){for(int i=0;i<19;i++)free(rgba[i]);return 0;}
    if(lib->n+19>lib->cap){int cap=lib->n+19;UgTexture *t=realloc(lib->t,(size_t)cap*sizeof *t);if(!t){for(int i=0;i<19;i++)free(rgba[i]);return 0;}lib->t=t;lib->cap=cap;}
    for(int i=0;i<19;i++){
        UgTexture *t=&lib->t[lib->n];memset(t,0,sizeof *t);t->key=keys[i];snprintf(t->name,sizeof t->name,"%s",names[i]);t->w=t->h=32;t->fmt=4;t->levels=1;t->rgba=rgba[i];
        snprintf(t->source,sizeof t->source,"%s:NFSUNDER/ZZDATA1.BIN",p->source[0]?p->source:"synthetic-test");
        p->icons[i]=++lib->n;
    }p->lib=lib;p->available=1;ug_texlib_index(lib);ug_controller_select(p,p->family);return 1;
}
/* Read only bounded extents, reject interleaved and multi-extent records. */
static int read_at(FILE *f,uint64_t off,void *buf,size_t n,uint64_t size){return off<=size&&n<=size-off&&!fseeko(f,(off_t)off,SEEK_SET)&&fread(buf,1,n,f)==n;}
static int directory_find(FILE *f,uint64_t size,unsigned block,uint32_t sector,uint32_t len,const char *name,int dir,uint32_t *so,uint32_t *lo){
    if(len>16*1024*1024)return 0;
    uint8_t *data=malloc(len);if(!data)return 0;int found=0;
    if(read_at(f,(uint64_t)sector*block,data,len,size))for(size_t i=0;i<len;){
        unsigned n=data[i];if(!n){i=((i/block)+1)*block;continue;}
        if(n<34||n>len-i)break;
        const uint8_t *r=data+i;unsigned nl=r[32];if(nl>n-33)break;
        char s[256];memcpy(s,r+33,nl);s[nl]=0;char *v=strchr(s,';');if(v)*v=0;
        if(!strcasecmp(s,name)&&!!(r[25]&2)==dir&&!r[1]&&!r[26]&&!r[27]&&!(r[25]&128)){*so=u32(r+2);*lo=u32(r+10);found=1;break;}
        i+=n;
    }
    free(data);return found;
}
int ug_controller_load_ps2(UgController *p,UgTexLib *lib,const char *path){
    FILE *f=fopen(path,"rb");if(!f){snprintf(p->status,sizeof p->status,"PS2 ISO unavailable: cannot open supplied path");return 0;}
    int ok=0;uint8_t vd[2048];uint64_t size=0;
    if(fseeko(f,0,SEEK_END)||ftello(f)<0)goto done;
    size=(uint64_t)ftello(f);
    int found=0;for(unsigned s=16;s<256;s++){
        if(!read_at(f,(uint64_t)s*2048,vd,sizeof vd,size)||memcmp(vd+1,"CD001",5)||vd[6]!=1)goto done;
        if(vd[0]==1){found=1;break;}if(vd[0]==255)break;
    }if(!found)goto done;
    unsigned block=u16(vd+128);if(block!=2048)goto done;
    uint32_t sector=u32(vd+158),len=u32(vd+166);
    if(!directory_find(f,size,block,sector,len,"NFSUNDER",1,&sector,&len)||!directory_find(f,size,block,sector,len,"ZZDATA1.BIN",0,&sector,&len))goto done;
    uint64_t start=(uint64_t)sector*block;if(start>size||len>size-start)goto done;
    snprintf(p->source,sizeof p->source,"%s",path);
    uint8_t buf[65536+7];for(uint64_t pos=0;pos+8<=len;){
        size_t n=(size_t)((len-pos)<sizeof buf?(len-pos):sizeof buf);if(!read_at(f,start+pos,buf,n,size))break;
        for(size_t i=0;i+8<=n;i++)if(u32(buf+i)==0xB3300000u){
            uint64_t clen=8ull+u32(buf+i+4);if(clen>16*1024*1024||clen>len-pos-i)continue;
            uint8_t *pk=malloc((size_t)clen);if(!pk)goto done;
            int got=read_at(f,start+pos+i,pk,(size_t)clen,size)&&ug_controller_load_pack(p,lib,pk,(size_t)clen);free(pk);
            if(got){ok=1;goto done;}
        }
        if(n<=7)break;
        pos+=n-7;
    }
done:fclose(f);if(!ok)snprintf(p->status,sizeof p->status,"PS2 ISO unavailable: no validated original controller pack in NFSUNDER/ZZDATA1.BIN");return ok;
}

/* Xbox resource validation, XBOX_CONTROLLER X2-X4. Only verified base keys
 * are exposed; opaque descriptors are shape-checked, never interpreted. */
int ug_xbox_icon_decode(int fmt,int w,int h,const uint8_t *src,size_t len,uint8_t *out){
    if(!src||!out||w<=0||h<=0||w>32||h>32||(fmt!=UG_PF_DXT1&&fmt!=UG_PF_DXT3))return 0;
    return ug_tex_decode(fmt,w,h,src,len,NULL,out);
}
int ug_controller_load_xbox_pack(UgController *p,UgTexLib *lib,const uint8_t *data,size_t len){
    Pack a={0};
    if(!p||!lib||!data||len<8||u32(data)!=0xb3300000u||u32(data+4)!=(uint64_t)len-8||!walk_pack(&a,data,data+8,len-8,0,60))return 0;
    if(!a.info||u32(a.info)!=5||!a.data||!a.records||!a.keylist||!a.desc||a.nr!=a.nk||a.nr!=a.nd)return 0;
    const uint8_t *records[23]={0};
    for(size_t i=0;i<a.nr;i++){
        const uint8_t *r=a.records+124*i;uint32_t key=u32(r+36);
        if(key!=u32(a.keylist+8*i))return 0;
        for(int j=0;j<23;j++)if(key==xbox_keys[j]){if(records[j])return 0;records[j]=r;}
    }
    uint8_t *rgba[23]={0};int count=0;
    for(int i=0;i<23;i++){
        const uint8_t *r=records[i];if(!r)continue;
        size_t off=u32(r+48),bytes=u32(r+56),needed=i==7?512:1024;
        int fmt=i==7?UG_PF_DXT1:UG_PF_DXT3;
        if(u16(r+68)!=32||u16(r+70)!=32||r[74]!=fmt||r[78]!=1||r[77]||u32(r+60)||bytes!=needed||u32(r+64)!=needed||off%128||off>a.ds||bytes>a.ds-off)continue;
        rgba[i]=malloc(4096);if(!rgba[i])goto failed;
        if(!ug_xbox_icon_decode(fmt,32,32,a.data+off,bytes,rgba[i])){free(rgba[i]);rgba[i]=NULL;continue;}count++;
    }
    if(!count)goto failed;
    if(lib->n+count>lib->cap){int cap=lib->n+count;UgTexture *t=realloc(lib->t,(size_t)cap*sizeof *t);if(!t)goto failed;lib->t=t;lib->cap=cap;}
    memset(p->xbox_icons,0,sizeof p->xbox_icons);
    for(int i=0;i<23;i++)if(rgba[i]){
        UgTexture *t=&lib->t[lib->n];memset(t,0,sizeof *t);t->key=xbox_keys[i];
        snprintf(t->name,sizeof t->name,"%s",xbox_names[i]);t->w=t->h=32;t->fmt=i==7?UG_PF_DXT1:UG_PF_DXT3;t->levels=1;t->rgba=rgba[i];
        snprintf(t->source,sizeof t->source,"%s:NFSUNDER/ZZDATA5.BIN",p->xbox_source[0]?p->xbox_source:"synthetic-test");p->xbox_icons[i]=++lib->n;
    }
    p->lib=lib;p->xbox_available=1;ug_texlib_index(lib);ug_controller_select(p,p->family);return 1;
failed:for(int i=0;i<23;i++)free(rgba[i]);return 0;
}

/* Independently authored bounded XDVDFS traversal, X1. One reader interface
 * serves real runtime files and fully synthetic memory fixtures. */
typedef struct {FILE *file;const uint8_t *memory;uint64_t size;} XboxReader;
static int xbox_read(const XboxReader *r,uint64_t off,void *out,size_t n){
    if(off>r->size||n>r->size-off)return 0;
    if(r->file)return read_at(r->file,off,out,n,r->size);
    memcpy(out,r->memory+(size_t)off,n);return 1;
}
static int xbox_directory(const XboxReader *r,uint32_t sector,uint32_t length,const char *name,size_t name_len,int directory,uint32_t *next_sector,uint32_t *next_length){
    if(length<14||length>16*1024*1024)return 0;
    uint8_t *table=malloc(length),*seen=calloc(length,1);
    size_t *stack=malloc(((size_t)length/4+1)*sizeof *stack);
    int found=0,ok=0;size_t pending=1;uint32_t so=0,lo=0;
    if(!table||!seen||!stack||!xbox_read(r,(uint64_t)sector*2048,table,length))goto done;
    stack[0]=0;
    while(pending){
        size_t at=stack[--pending];
        if(at>length||length-at<14||seen[at])goto done;
        seen[at]=1;const uint8_t *entry=table+at;size_t nl=entry[13];
        if(nl>length-at-14)goto done;
        uint32_t es=u32(entry+4),el=u32(entry+8);uint64_t start=(uint64_t)es*2048;
        if(start>r->size||el>r->size-start)goto done;
        if(nl==name_len&&!strncasecmp((const char *)entry+14,name,nl)&&!!(entry[12]&0x10)==directory){
            if(found)goto done;
            so=es;lo=el;found=1;
        }
        for(int i=0;i<2;i++){
            size_t child=(size_t)u16(entry+2*i)*4;if(!child)continue;
            if(child>=length||length-child<14||pending>=(size_t)length/4+1)goto done;
            stack[pending++]=child;
        }
    }
    if(found){*next_sector=so;*next_length=lo;ok=1;}
done:free(stack);free(seen);free(table);return ok;
}
static int xbox_find(const XboxReader *r,const char *path,uint64_t *offset,uint32_t *size){
    uint8_t descriptor[2048];if(!path||!*path||!xbox_read(r,32*2048,descriptor,sizeof descriptor))return 0;
    uint32_t sector=u32(descriptor+20),length=u32(descriptor+24),visited[64];int depth=0;
    const char *part=path;while(*part=='/')part++;
    while(*part){
        if(depth>=64)return 0;
        for(int i=0;i<depth;i++)if(visited[i]==sector)return 0;
        visited[depth++]=sector;
        const char *end=strchr(part,'/');size_t n=end?(size_t)(end-part):strlen(part);
        if(!n||n>255)return 0;
        int dir=end&&end[1];
        if(!xbox_directory(r,sector,length,part,n,dir,&sector,&length))return 0;
        if(!dir){if(offset)*offset=(uint64_t)sector*2048;if(size)*size=length;return 1;}
        part=end+1;
    }
    return 0;
}
int ug_xdvdfs_find(const uint8_t *source,size_t len,const char *path,uint64_t *offset,uint32_t *size){
    if(!source)return 0;
    XboxReader r={NULL,source,len};return xbox_find(&r,path,offset,size);
}
int ug_controller_load_xbox(UgController *p,UgTexLib *lib,const char *path){
    if(!p||!lib||!path)return 0;
    FILE *f=fopen(path,"rb");int ok=0;
    if(!f){snprintf(p->status,sizeof p->status,"Xbox ISO unavailable: cannot open supplied path");return 0;}
    if(fseeko(f,0,SEEK_END)||ftello(f)<0)goto done;
    XboxReader r={f,NULL,(uint64_t)ftello(f)};uint64_t start;uint32_t length;
    if(!xbox_find(&r,"/NFSUNDER/ZZDATA5.BIN",&start,&length))goto done;
    /* Scan only the named member; candidate lengths stay inside its extent. */
    uint8_t buffer[65536+7];
    for(uint64_t pos=0;pos+8<=length;){
        size_t n=(size_t)(((uint64_t)length-pos)<sizeof buffer?((uint64_t)length-pos):sizeof buffer);
        if(!xbox_read(&r,start+pos,buffer,n))break;
        for(size_t i=0;i+8<=n;i++)if(u32(buffer+i)==0xb3300000u){
            uint64_t bytes=8ull+u32(buffer+i+4);
            if(bytes>16*1024*1024||bytes>(uint64_t)length-pos-i)continue;
            uint8_t *pack=malloc((size_t)bytes);if(!pack)goto done;
            snprintf(p->xbox_source,sizeof p->xbox_source,"%s",path);
            int valid=xbox_read(&r,start+pos+i,pack,(size_t)bytes)&&ug_controller_load_xbox_pack(p,lib,pack,(size_t)bytes);free(pack);
            if(valid){ok=1;goto done;}
        }
        if(n<=7)break;
        pos+=n-7;
    }
done:fclose(f);if(!ok)snprintf(p->status,sizeof p->status,"Xbox ISO unavailable: no validated controller pack in NFSUNDER/ZZDATA5.BIN");return ok;
}
