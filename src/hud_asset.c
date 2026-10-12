#include "hud_asset.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <limits.h>
static uint16_t u16(const unsigned char *p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t u32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static float f32(const unsigned char *p){uint32_t u=u32(p);float f;memcpy(&f,&u,4);return f;}
static int tag(const unsigned char*p,const char*s){return !memcmp(p,s,4);}
static char *namecopy(const unsigned char*p,size_t n){const unsigned char *z=memchr(p,0,n);if(!z)return NULL;size_t k=(size_t)(z-p);char *s=malloc(k+1);if(s)memcpy(s,p,k+1);return s;}
uint32_t ha_hash(const char*s){uint32_t h=UINT32_MAX;for(;*s;s++){unsigned char c=(unsigned char)*s;if(c>='a'&&c<='z')c-=32;h=33*h+c;}return h;}
int ha_object(const unsigned char*d,size_t n,HaNode*o){
 memset(o,0,sizeof *o);o->parent=-1;o->valid=1;int identity=0;size_t appearance_extent=0;
 if(n<8||memcmp(d,"FOb\xea",4)||u32(d+4)>n-8)return 0;
 size_t end=8+u32(d+4),at=8;
 while(at<end){if(end-at<8)return 0;size_t z=u32(d+at+4);if(z>end-at-8)return 0;
 if(tag(d+at,"ObjD")){size_t q=at+8,e=q+z;while(q<e){if(e-q<4)return 0;size_t v=u16(d+q+2);const unsigned char*b=d+q+4;if(v>e-q-4)return 0;
 #define FIELD(s) (!memcmp(d+q,s,2))
 if(FIELD("Ot")){if(v<4)return 0;o->kind=u32(b);}else if(FIELD("Oh")){if(v<4)return 0;o->name=u32(b);}else if(FIELD("OP")){if(v<16)return 0;o->id=u32(b);o->name=u32(b+4);o->resource=u16(b+12);identity=1;}
 else if(FIELD("PA")){if(v<4)return 0;o->has_parent=1;o->parent_id=u32(b);}
 else if(FIELD("Sj")){if(v<4)return 0;o->justification=(uint16_t)u32(b);}
 else if(FIELD("St")){if(v%2)return 0;size_t k;for(k=0;k<v;k+=2)if(!u16(b+k))break;if(k==v)return 0;free(o->sample);o->sample=calloc(k/2+1,1);if(!o->sample)return 0;for(size_t j=0;j<k/2;j++)o->sample[j]=u16(b+2*j)<128?(char)u16(b+2*j):'?';}
 else if(FIELD("SA")){appearance_extent=v;size_t need=o->kind==1?84:68;if(v<need)return 0;for(int j=0;j<4;j++)if(u32(b+4*j)>255)return 0;o->color[0]=(unsigned char)u32(b+8);o->color[1]=(unsigned char)u32(b+4);o->color[2]=(unsigned char)u32(b);o->color[3]=(unsigned char)u32(b+12);for(int j=0;j<3;j++){o->pivot[j]=f32(b+16+4*j);o->position[j]=f32(b+28+4*j);o->size[j]=f32(b+56+4*j);if(!isfinite(o->pivot[j])||!isfinite(o->position[j])||!isfinite(o->size[j]))return 0;}for(int j=0;j<4;j++){o->quaternion[j]=f32(b+40+4*j);if(!isfinite(o->quaternion[j]))return 0;o->uv[j]=v>=84?f32(b+68+4*j):0;if(!isfinite(o->uv[j]))return 0;}o->appearance=1;}
 q+=4+v;
 #undef FIELD
 }}at+=8+z;}if(o->kind==1&&o->appearance&&appearance_extent<84)return 0;return identity;
}
int ha_hierarchy(HaPackage*p){int ok=1;for(size_t i=0;i<p->count;i++){HaNode*n=p->nodes+i;n->parent=-1;for(size_t j=0;j<i;j++)if(p->nodes[j].id==n->id){n->valid=0;p->nodes[j].valid=0;ok=0;}if(n->has_parent){for(size_t j=0;j<p->count;j++)if(p->nodes[j].id==n->parent_id){n->parent=(int)j;break;}if(n->parent<0){n->valid=0;ok=0;}}}
 for(size_t i=0;i<p->count;i++){int j=(int)i;unsigned depth=0;while(j>=0){if(depth++>=HA_DEPTH||!p->nodes[j].valid){p->nodes[i].valid=0;ok=0;break;}j=p->nodes[j].parent;}}if(!ok)p->diagnostics++;return ok;}
static int blocks(const unsigned char*d,size_t n,HaPackage*p,int mode){size_t at=0;const unsigned char*pool=NULL,*req=NULL;size_t pooln=0,reqn=0;
 while(at<n){if(n-at<8)return 0;size_t z=u32(d+at+4);if(z>n-at-8)return 0;const unsigned char*b=d+at+8;
 if(mode==1){if(tag(d+at,"RsNm")){pool=b;pooln=z;}else if(tag(d+at,"RsRq")){req=b;reqn=z;}}
 else if(mode==2&&tag(d+at,"FOb\xea")){if(p->count>=HA_LIMIT)return 0;HaNode node;if(ha_object(d+at,z+8,&node)){HaNode*a=realloc(p->nodes,(p->count+1)*sizeof *a);if(!a){free(node.sample);return 0;}p->nodes=a;p->nodes[p->count++]=node;}else{free(node.sample);p->diagnostics++;}}
 else if(mode==0&&(tag(d+at,"Res\xcc")||tag(d+at,"Obj\xcc"))){if(!blocks(b,z,p,tag(d+at,"Res\xcc")?1:2))return 0;}
 at+=8+z;}
 if(mode==1&&req){if(p->resources)return 0;if(reqn<4)return 0;size_t count=u32(req);if(count>HA_LIMIT||count>(reqn-4)/24||!pool)return 0;p->resources=calloc(count,sizeof *p->resources);if(!p->resources)return 0;p->resource_count=count;for(size_t i=0;i<count;i++){const unsigned char*r=req+4+i*24;size_t off=u32(r+4);p->resources[i].id=u32(r);p->resources[i].kind=u32(r+8);if(off>=pooln||!(p->resources[i].name=namecopy(pool+off,pooln-off))){p->diagnostics++;p->resources[i].kind=UINT32_MAX;}}}return 1;}
int ha_header_name(const unsigned char*d,size_t n,const char*name){if(n<24)return -1;size_t names=u32(d+16),paths=u32(d+20);if(!names||names>n-24||paths>n-24-names)return -1;const unsigned char*z=memchr(d+24,0,names);if(!z)return -1;size_t k=(size_t)(z-d-24);return strlen(name)==k&&!memcmp(d+24,name,k);}
static int selected(const unsigned char*d,size_t n,const char*name){size_t at=0;int found=0;while(at<n){if(n-at<8)return -1;size_t z=u32(d+at+4);if(z>n-at-8)return -1;if(tag(d+at,"PkHd")){int r=ha_header_name(d+at+8,z,name);if(r<0)return -1;if(r)found++;}at+=8+z;}return found==1?1:found>1?-1:0;}
static int archives(const unsigned char*d,size_t n,const char*name,const unsigned char**match,size_t*extent,unsigned depth){if(depth>HA_DEPTH)return 0;size_t at=0;while(at<n){if(n-at<8)return 0;uint32_t t=u32(d+at);size_t z=u32(d+at+4);if(z>n-at-8)return 0;const unsigned char*b=d+at+8;if(t==0x30203){if(z<8||memcmp(b,"FEn\xe7",4)||u32(b+4)>z-8)return 0;int r=selected(b+8,u32(b+4),name);if(r<0)return 0;if(r){if(*match)return 0;*match=b+8;*extent=u32(b+4);}}else if(t&0x80000000u){if(!archives(b,z,name,match,extent,depth+1))return 0;}
 at+=8+z;}return 1;}
int ha_package_select(const unsigned char*d,size_t n,const char*name,HaPackage*p){memset(p,0,sizeof *p);const unsigned char*match=NULL;size_t extent=0;if(!archives(d,n,name,&match,&extent,0)||!match||!blocks(match,extent,p,0)){ha_free(p);return 0;}ha_hierarchy(p);return 1;}
int ha_package(const unsigned char*d,size_t n,HaPackage*p){return ha_package_select(d,n,"HUD_SingleRace.fng",p);}
uint32_t ha_texture_hash(const char*alias){if(!strcmp(alias,"Track4000_map.tga"))return ha_hash("TRACKMAP4000");size_t n=strlen(alias);if(n>4096)return 0;char stem[4097];memcpy(stem,alias,n+1);if(n>=4){char ext[5];for(int i=0;i<4;i++){unsigned char c=stem[n-4+i];ext[i]=(char)(c>='A'&&c<='Z'?c+32:c);}ext[4]=0;if(!strcmp(ext,".tga")||!strcmp(ext,".bmp"))stem[n-4]=0;}return ha_hash(stem);}
void ha_free(HaPackage*p){for(size_t i=0;i<p->count;i++)free(p->nodes[i].sample);for(size_t i=0;i<p->resource_count;i++)free(p->resources[i].name);free(p->nodes);free(p->resources);memset(p,0,sizeof *p);}
int ha_font(const unsigned char*d,size_t n,HaFont*f){memset(f,0,sizeof *f);if(n<640||!memchr(d,0,256)||!memchr(d+256,0,256)||memcmp(d+512,"FNTF",4))return 0;memcpy(f->name,d,256);memcpy(f->texture,d+256,256);const unsigned char*h=d+512;size_t count=u16(h+10);if(count>(n-640)/16||u32(h+24)!=128+16*count||u32(h+28)!=n-512)return 0;f->inv_w=f32(h+64);f->inv_h=f32(h+68);f->line=u32(h+36);if(!isfinite(f->inv_w)||!isfinite(f->inv_h)||f->inv_w<=0||f->inv_h<=0)return 0;f->glyphs=calloc(count,sizeof *f->glyphs);if(!f->glyphs)return 0;f->count=count;for(size_t i=0;i<count;i++){const unsigned char*g=h+128+i*16;f->glyphs[i]=(HaGlyph){u16(g),u16(g+4),u16(g+6),u16(g+14),g[2],g[3],g[10],(int8_t)g[9]};}return 1;}
void ha_point(const HaPackage*p,size_t index,const float in[3],float out[3],float angle){memcpy(out,in,12);int i=(int)index;for(unsigned depth=0;i>=0&&depth<HA_DEPTH;depth++,i=p->nodes[i].parent){const HaNode*n=p->nodes+i;float v[3];for(int j=0;j<3;j++)v[j]=(out[j]-n->pivot[j])*(n->kind==5?n->size[j]:1);float x=n->quaternion[0],y=n->quaternion[1],z=n->quaternion[2],w=n->quaternion[3],norm=x*x+y*y+z*z+w*w;float a=norm>0?2/norm:0;float r[3]={ (1-a*(y*y+z*z))*v[0]+a*(x*y-z*w)*v[1]+a*(x*z+y*w)*v[2],a*(x*y+z*w)*v[0]+(1-a*(x*x+z*z))*v[1]+a*(y*z-x*w)*v[2],a*(x*z-y*w)*v[0]+a*(y*z+x*w)*v[1]+(1-a*(x*x+y*y))*v[2]};if(depth==0&&angle){float c=cosf(angle),sn=sinf(angle),rx=r[0];r[0]=c*rx-sn*r[1];r[1]=sn*rx+c*r[1];}for(int j=0;j<3;j++)out[j]=n->position[j]+n->pivot[j]+r[j];}}
int ha_speed(double v,int metric,char out[32]){out[0]=0;v=fabs(v)*(metric?3.6:2.237);if(!isfinite(v)||v>INT_MAX)return 0;snprintf(out,32,"%d",(int)v);return 1;}
int ha_scale(double m){int scales[]={7500,8500,9500,10500};if(isfinite(m)&&m>0)for(int i=0;i<4;i++)if(m<=scales[i])return scales[i];return 10500;}
double ha_rpm_angle(double rpm,double maximum,double initial){int scale=ha_scale(maximum);double bound=isfinite(maximum)&&maximum>0?maximum:scale;if(!isfinite(rpm))rpm=0;(void)initial;return 235*fmin(bound,fmax(0,rpm))/scale;}
int ha_skin(int s){return s>=0&&s<=10?s:0;}
int ha_cash(int career,int supplied,int suppressed,int64_t b,char sep,char out[64]){out[0]=0;if(!career||!supplied||suppressed||b<0)return 0;char raw[32];snprintf(raw,sizeof raw,"%lld",(long long)b);size_t n=strlen(raw),k=0;for(size_t i=0;i<n;i++){out[k++]=raw[i];if(sep&&i+1<n&&(n-i-1)%3==0)out[k++]=sep;}out[k]=0;return 1;}
int ha_map(double ox,double oy,double e,double x,double y,double h,double window,double out[3]){if(!isfinite(ox)||!isfinite(oy)||!isfinite(e)||e<=0||!isfinite(x)||!isfinite(y)||!isfinite(h)||!isfinite(window)||window<=0)return 0;out[0]=(x-ox)/e;out[1]=(oy-y)/e;out[2]=window/e;return isfinite(out[0])&&isfinite(out[1])&&isfinite(out[2]);}
float ha_text_start(float a,float s,unsigned b,float w){return a-w*s*((b&2)?1:(b&1)?.5f:0);}

int ha_gear(int gear,char out[32]){out[0]=0;if(gear==-1)strcpy(out,"R");else if(gear==0)strcpy(out,"N");else if(gear>0)snprintf(out,32,"%d",gear);else return 0;return 1;}
int ha_fit(int w,int h,float x,float y,float out[3]){if(w<=0||h<=0)return 0;out[0]=fminf(w/640.0f,h/480.0f);out[1]=w/2.0f+x*out[0];out[2]=h/2.0f+y*out[0];return 1;}

int ha_map_pick(const HaMapFrame*f,double x,double y,double coverage,double out[2]){
 if(!f||!f->enabled||f->width<=0||f->height<=0||!isfinite(x)||!isfinite(y)||!isfinite(coverage)||coverage<=0||f->size[0]<=0||f->size[1]<=0||f->window<=0)return 0;
 double scale=fmin(f->width/640.0,f->height/480.0),dx=(x-f->width*.5)/scale-f->center[0],dy=(y-f->height*.5)/scale-f->center[1],det=f->basis[0]*f->basis[3]-f->basis[1]*f->basis[2];
 if(!isfinite(det)||fabs(det)<1e-12)return 0;
 double lx=(f->basis[3]*dx-f->basis[2]*dy)/det/f->size[0],ly=(-f->basis[1]*dx+f->basis[0]*dy)/det/f->size[1];
 if(fabs(lx)>.5||fabs(ly)>.5)return 0;
 double c=cos(f->heading),s=sin(f->heading);out[0]=f->world[0]+(c*lx-s*ly)*f->window;out[1]=f->world[1]-(s*lx+c*ly)*f->window;return isfinite(out[0])&&isfinite(out[1]);
}

int ha_needle_calibrate(HaNode*n){
 float x=n->quaternion[0],y=n->quaternion[1],z=n->quaternion[2],w=n->quaternion[3],norm=sqrtf(x*x+y*y+z*z+w*w);if(!isfinite(norm)||norm<1e-8f)return 0;
 x/=norm;y/=norm;z/=norm;w/=norm;
 if(fabsf(x)<1e-5f&&fabsf(y)<1e-5f){n->quaternion[0]=n->quaternion[1]=n->quaternion[2]=0;n->quaternion[3]=1;return 1;}
 if(fabsf(y)<1e-5f&&fabsf(z)<1e-5f&&fabsf(w)<1e-5f){n->quaternion[0]=1;n->quaternion[1]=n->quaternion[2]=n->quaternion[3]=0;return 1;}
 if(fabsf(x)<1e-5f&&fabsf(z)<1e-5f&&fabsf(w)<1e-5f){n->quaternion[1]=1;n->quaternion[0]=n->quaternion[2]=n->quaternion[3]=0;return 1;}
 return 0;
}

/* ---- Runtime capture amendments, capture sets 1-3 ------------------------ */
const char ha_bank_label[6]="Bank:";
void ha_text_tint(int role,int gear,int change,double speed_mps,double standstill,const unsigned char a[4],const unsigned char g[3],unsigned char out[4]){
 memcpy(out,a,4);
 if(role!=HA_TEXT_GEAR||gear<1)return; /* F1: N and R keep the authored tint */
 int replace=change!=0;                          /* F1 item 2a */
 if(!replace&&gear==1)replace=isfinite(speed_mps)&&isfinite(standstill)&&standstill>0&&fabs(speed_mps)<standstill; /* F1 item 2b, Q7 */
 if(replace)memcpy(out,g,3);
}
void ha_setting_rgba(const unsigned char rgba[4],unsigned char out[4]){memcpy(out,rgba,4);}
void ha_blend_over(const unsigned char texel[4],const unsigned char tint[4],const unsigned char bg[3],double out[3]){
 double a=texel[3]/255.0*tint[3]/255.0;
 for(int j=0;j<3;j++)out[j]=texel[j]*tint[j]/255.0*a+bg[j]*(1-a);
}
int ha_inherited_message_visuals(int race_path,int widget_drawn,int pending){
 if(race_path)return pending!=0; /* baseline race path unchanged */
 return !widget_drawn&&pending;  /* D4: the authored widget suppresses inherited visuals */
}
void ha_setting_tint(const unsigned char a[4],const unsigned char rgb[3],unsigned char out[4]){memcpy(out,rgb,3);out[3]=a[3];}
double ha_dial_angle(double dx,double dy){
 double d=atan2(-dx,dy)*180.0/3.14159265358979323846;
 if(d<0)d+=360;
 if(d>=360)d-=360;
 return d;
}
double ha_indicated_rpm(double rpm,double maximum){
 int scale=ha_scale(maximum);double bound=isfinite(maximum)&&maximum>0?maximum:scale;
 if(!isfinite(rpm))rpm=0;
 return fmin(bound,fmax(0,rpm));
}
int ha_redline(int enabled,int supplied,double r,int scale,double*start){
 *start=0;
 if(!enabled||!supplied)return 0;
 if(!isfinite(r)||scale<=0)return -1;
 if(r<0)r=0;
 if(r>=scale)return 0;
 *start=235.0*r/scale;
 return 1;
}
int ha_shift(int enabled,int supplied,double shift_rpm,int gear,double rpm,double maximum){
 if(!enabled||!supplied)return 0;
 if(!isfinite(shift_rpm)||shift_rpm<=0)return -1; /* D5: diagnose invalid threshold */
 if(gear<1)return 0;
 return ha_indicated_rpm(rpm,maximum)>=shift_rpm;
}
int ha_nos_level(int enabled,int installed,int supplied,double level,double a0,double a1,double*limit){
 *limit=0;
 if(!enabled||!installed||!supplied)return 0;
 if(!isfinite(level)||!isfinite(a0)||!isfinite(a1))return -1;
 level=fmin(1,fmax(0,level));
 if(level<=0)return 1;
 *limit=a0+level*(a1-a0);
 return 2;
}
int ha_bank(int career,int supplied,int suppressed,int64_t balance,char separator,int resolved,char value[64]){
 value[0]=0;
 if(!career||!supplied||suppressed)return 0;
 if(balance<0||!resolved)return -1;
 if(!ha_cash(career,supplied,suppressed,balance,separator,value))return -1;
 return 1;
}
void ha_widget(int career,int pending,int resolved,int centre_present,HaWidget*w){
 memset(w,0,sizeof *w);
 if(!career)return;
 if(!resolved){w->diagnostic=1;return;}
 w->visible=w->backing=w->dpad=1;
 if(centre_present)w->centre=1;
 else w->diagnostic=1;
 if(pending)w->mail=w->highlight_right=1;
}
static int alias_equal(const char*a,const char*b){
 for(;*a&&*b;a++,b++){unsigned char x=(unsigned char)*a,y=(unsigned char)*b;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return 0;}
 return !*a&&!*b;
}
int ha_lookup(const HaPackage*p,const HaKey*k){
 int found=-1;
 if(!p||!k)return -1;
 for(size_t i=0;i<p->count;i++){
  if(p->nodes[i].name!=k->name)continue;
  if(found>=0)return -1;
  found=(int)i;
 }
 if(found<0)return -1;
 const HaNode*n=p->nodes+found;
 if(!n->valid||n->kind!=k->kind)return -1;
 if(k->parent_root){if(n->has_parent||n->parent>=0)return -1;}
 else if(n->parent<0||(size_t)n->parent>=p->count||p->nodes[n->parent].name!=k->parent_name)return -1;
 if(k->kind==HA_KIND_IMAGE&&k->alias){
  if(n->resource>=p->resource_count||!p->resources[n->resource].name||p->resources[n->resource].kind!=1)return -1;
  if(!alias_equal(p->resources[n->resource].name,k->alias))return -1;
 }
 return found;
}

/* ---- Capture set 5: N2O glow label (E3, opt-in, inferred) ---------------- */
static double lerp_keys(double t,const double k[][2],int n){
 if(t<=k[0][0])return k[0][1];
 for(int i=1;i<n;i++)if(t<=k[i][0])return k[i-1][1]+(k[i][1]-k[i-1][1])*(t-k[i-1][0])/(k[i][0]-k[i-1][0]);
 return k[n-1][1];
}
int ha_glow(double t,HaGlow*o){
 static const double alpha_keys[][2]={{100,0},{340,255},{360,500},{480,500},{620,0}};
 static const double size_keys[][2]={{340,0},{360,1},{460,1},{480,0}}; /* 0 = base size, 1 = grown */
 memset(o,0,sizeof *o);o->width=64;o->height=32;
 if(!isfinite(t)||t<HA_GLOW_START||t>=HA_GLOW_END)return 0;
 double raw=t>=620?0:lerp_keys(t,alpha_keys,5),k=lerp_keys(t,size_keys,4);
 o->alpha_byte=(unsigned char)((long)floor(raw)%256); /* values above 255 wrap modulo 256 (inferred) */
 o->alpha=o->alpha_byte/255.0;
 o->width=64+10*k;o->height=32+10*k;o->rotation_ccw_deg=2*k;
 o->visible=o->alpha_byte>0;
 return o->visible;
}
double ha_glow_units(double ms,double ms_per_unit){
 if(!isfinite(ms)||!isfinite(ms_per_unit)||ms_per_unit<=0)return NAN;
 return ms/ms_per_unit;
}
int ha_glow_trigger(int enabled,double now_ms,int have_start,double start_ms,double ms_per_unit){
 if(!enabled||!isfinite(now_ms)||!isfinite(ms_per_unit)||ms_per_unit<=0)return 0;
 if(!have_start||!isfinite(start_ms))return 1;
 double t=ha_glow_units(now_ms-start_ms,ms_per_unit);
 return t<0||t>=HA_GLOW_END; /* a new event while running is ignored (guessed) */
}

/* ---- Capture set 7 G3: glyph quad from the stored cell -------------------- */
int ha_glyph_quad(const HaGlyph*g,float inv_w,float inv_h,double pen,double anchor_y,double sx,double sy,int inset,HaGlyphQuad*o){
 memset(o,0,sizeof *o);
 if(!g||!g->width||!g->height)return 0; /* empty glyph: pen still advances */
 o->x0=pen+g->bearing*sx;o->x1=o->x0+g->width*sx; /* width from the cell, never the advance */
 o->y0=anchor_y+g->top*sy;o->y1=o->y0+g->height*sy;
 double ix=inset&&g->width>1?.5:0,iy=inset&&g->height>1?.5:0;
 o->u0=(g->x+ix)*(double)inv_w;o->u1=(g->x+g->width-ix)*(double)inv_w;
 o->v0=(g->y+iy)*(double)inv_h;o->v1=(g->y+g->height-iy)*(double)inv_h;
 return 1;
}

/* ---- Set 8 G5-G7: glyph sampling ------------------------------------------ */
int ha_glyph_atlas(const unsigned char*rgba,unsigned w,unsigned h,const HaGlyph*glyphs,size_t count,HaGlyphAtlas*o){
 memset(o,0,sizeof *o);
 if(!rgba||!w||!h||w>HA_LIMIT||h>HA_LIMIT||(count&&!glyphs)||count>HA_LIMIT*4)return 0;
 const unsigned G=HA_GLYPH_GUTTER;
 HaGlyphCell*cells=calloc(count?count:1,sizeof *cells);if(!cells)return 0;
 /* Shelf layout in glyph order; each slot is the cell plus the gutter. */
 unsigned width=w,x=0,y=0,shelf=0;
 for(size_t i=0;i<count;i++){const HaGlyph*g=glyphs+i;if(g->width&&g->width+2*G>width)width=g->width+2*G;}
 for(size_t i=0;i<count;i++){const HaGlyph*g=glyphs+i;
  if(!g->width||!g->height){cells[i].ok=1;continue;}
  if((unsigned)g->x+g->width>w||(unsigned)g->y+g->height>h)continue; /* ok = 0 */
  unsigned sw=g->width+2*G,sh=g->height+2*G;
  if(x+sw>width){y+=shelf;x=0;shelf=0;}
  cells[i].x=(uint16_t)(x+G);cells[i].y=(uint16_t)(y+G);cells[i].ok=1;x+=sw;if(sh>shelf)shelf=sh;
  if(y+shelf>HA_LIMIT){free(cells);return 0;}
 }
 unsigned height=y+shelf?y+shelf:1;
 unsigned char*px=calloc((size_t)width*height,4);if(!px){free(cells);return 0;}
 for(size_t i=0;i<count;i++){const HaGlyph*g=glyphs+i;if(!cells[i].ok||!g->width||!g->height)continue;
  for(int dy=-(int)G;dy<(int)(g->height+G);dy++)for(int dx=-(int)G;dx<(int)(g->width+G);dx++){
   int cx=dx<0?0:dx>=g->width?g->width-1:dx,cy=dy<0?0:dy>=g->height?g->height-1:dy; /* nearest cell texel */
   const unsigned char*s=rgba+(((size_t)g->y+cy)*w+g->x+cx)*4;unsigned char*d=px+(((size_t)cells[i].y+dy)*width+cells[i].x+dx)*4;
   memcpy(d,s,4);if(dx!=cx||dy!=cy)d[3]=0; /* outside the cell: alpha 0 (G6 item 2) */
  }
 }
 o->rgba=px;o->width=width;o->height=height;o->cells=cells;o->count=count;return 1;
}
void ha_glyph_atlas_free(HaGlyphAtlas*a){if(!a)return;free(a->rgba);free(a->cells);memset(a,0,sizeof *a);}
void ha_sample_linear(const unsigned char*rgba,unsigned w,unsigned h,double u,double v,double out[4]){
 memset(out,0,4*sizeof *out);if(!rgba||!w||!h||!isfinite(u)||!isfinite(v))return;
 double fx=u*w-.5,fy=v*h-.5,x0=floor(fx),y0=floor(fy),ax=fx-x0,ay=fy-y0;
 long xs[2]={(long)x0,(long)x0+1},ys[2]={(long)y0,(long)y0+1};
 for(int k=0;k<2;k++){if(xs[k]<0)xs[k]=0;if(xs[k]>(long)w-1)xs[k]=(long)w-1;if(ys[k]<0)ys[k]=0;if(ys[k]>(long)h-1)ys[k]=(long)h-1;} /* clamp to edge */
 for(int c=0;c<4;c++){double t[2][2];for(int j=0;j<2;j++)for(int k=0;k<2;k++)t[j][k]=rgba[((size_t)ys[j]*w+(size_t)xs[k])*4+c]/255.0;
  out[c]=(1-ay)*((1-ax)*t[0][0]+ax*t[0][1])+ay*((1-ax)*t[1][0]+ax*t[1][1]);}
}
double ha_glyph_coverage(double alpha,double tint_alpha){return alpha*tint_alpha/255.0;}
double ha_glyph_over(double texel,double alpha,double tint,double tint_alpha,double dst){double a=ha_glyph_coverage(alpha,tint_alpha);return texel*tint*a+dst*(1-a);}

/* ---- Capture set 7 H: shift-up icon visibility, flash and colour ---------- */
int ha_shift_mode(int enabled,int supplied,double shift_rpm,int gear,double rpm,double maximum,int transmission,int automatic_setting){
 if(transmission==HA_TRANSMISSION_AUTOMATIC&&!automatic_setting)return 0; /* H4 (guessed default on) */
 return ha_shift(enabled,supplied,shift_rpm,gear,rpm,maximum);
}
double ha_shift_flash(int flash_enabled,double elapsed_ms,double ms_per_unit,int*diagnostic){
 *diagnostic=0;
 if(!flash_enabled)return 1;
 if(!isfinite(elapsed_ms)||elapsed_ms<0||!isfinite(ms_per_unit)||ms_per_unit<=0){*diagnostic=1;return 1;}
 double u=fmod(elapsed_ms/ms_per_unit,HA_SHIFT_CYCLE_UNITS);
 if(u<HA_SHIFT_LIT_UNITS)return 1;
 if(u<HA_SHIFT_DARK_UNITS)return (HA_SHIFT_DARK_UNITS-u)/(HA_SHIFT_DARK_UNITS-HA_SHIFT_LIT_UNITS);
 return 0;
}
double ha_shift_elapsed(HaShiftPhase*p,int visible,double now_ms){
 if(!visible){p->shown=0;p->start_ms=0;return 0;}
 if(!isfinite(now_ms)){p->shown=1;p->start_ms=NAN;return NAN;}
 if(!p->shown||!isfinite(p->start_ms)||now_ms<p->start_ms){p->shown=1;p->start_ms=now_ms;}
 return now_ms-p->start_ms;
}
void ha_shift_default_colour(unsigned char out[3]){out[0]=202;out[1]=244;out[2]=138;}
