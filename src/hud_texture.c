/* Independent bounded collection reader and standard top-level pixel decoder. */
#include "hud_texture.h"
#include <stdlib.h>
#include <string.h>
#define FOURCC(a,b,c,d) ((uint32_t)(a)|(uint32_t)(b)<<8|(uint32_t)(c)<<16|(uint32_t)(d)<<24)
static uint16_t rd16(const unsigned char*p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t rd32(const unsigned char*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void endpoint(uint16_t x,unsigned char rgb[3]){unsigned r=x>>11,g=(x>>5)&63,b=x&31;rgb[0]=(unsigned char)((r<<3)|(r>>2));rgb[1]=(unsigned char)((g<<2)|(g>>4));rgb[2]=(unsigned char)((b<<3)|(b>>2));}
int ha_texture_decode(uint32_t format,unsigned w,unsigned h,const unsigned char*image,size_t n,const unsigned char*palette,size_t pn,HaPixels*out){
 memset(out,0,sizeof *out);
 if(!w||!h||w>4096||h>4096||!image)return -1;
 size_t pixels=(size_t)w*h;
 if(format==41){if(n<pixels||pn<1024||!palette)return -1;}
 else if(format==FOURCC('D','X','T','3')||format==FOURCC('D','X','T','1')){size_t need=((size_t)w+3)/4*((h+3)/4)*(format==FOURCC('D','X','T','3')?16:8);if(n<need)return -1;}
 else return -2;
 unsigned char*rgba=malloc(pixels*4);if(!rgba)return -1;
 if(format==41){for(size_t i=0;i<pixels;i++){const unsigned char*p=palette+image[i]*4;rgba[i*4]=p[2];rgba[i*4+1]=p[1];rgba[i*4+2]=p[0];rgba[i*4+3]=p[3];}}
 else{unsigned bw=(w+3)/4,bh=(h+3)/4;int explicit_alpha=format==FOURCC('D','X','T','3');for(unsigned by=0;by<bh;by++)for(unsigned bx=0;bx<bw;bx++){
  const unsigned char*b=image+((size_t)by*bw+bx)*(explicit_alpha?16:8),*color=b+(explicit_alpha?8:0);uint16_t c0=rd16(color),c1=rd16(color+2);unsigned char colors[4][3];endpoint(c0,colors[0]);endpoint(c1,colors[1]);int opaque=explicit_alpha||c0>c1;
  for(int k=0;k<3;k++){colors[2][k]=(unsigned char)(opaque?(2*colors[0][k]+colors[1][k])/3:(colors[0][k]+colors[1][k])/2);colors[3][k]=(unsigned char)(opaque?(colors[0][k]+2*colors[1][k])/3:0);}
  uint32_t selectors=rd32(color+4);for(unsigned py=0;py<4;py++)for(unsigned px=0;px<4;px++){unsigned x=bx*4+px,y=by*4+py,j=py*4+px,selector=(selectors>>(j*2))&3;if(x>=w||y>=h)continue;unsigned char*p=rgba+((size_t)y*w+x)*4;memcpy(p,colors[selector],3);p[3]=explicit_alpha?(unsigned char)(((b[j/2]>>((j%2)*4))&15)*17):(unsigned char)(!opaque&&selector==3?0:255);}
 }}
 out->rgba=rgba;out->width=w;out->height=h;out->format=format;return 1;
}
typedef struct {const unsigned char *catalog,*formats,*pixels;size_t catalog_len,formats_len,pixels_len;int malformed;} Collection;
static void leaves(const unsigned char*d,size_t n,Collection*c,unsigned depth){if(depth>64){c->malformed=1;return;}for(size_t at=0;at<n;){if(n-at<8){c->malformed=1;return;}uint32_t type=rd32(d+at);size_t z=rd32(d+at+4);if(z>n-at-8){c->malformed=1;return;}const unsigned char*b=d+at+8;if(type==0x33310004){if(c->catalog)c->malformed=1;c->catalog=b;c->catalog_len=z;}else if(type==0x33310005){if(c->formats)c->malformed=1;c->formats=b;c->formats_len=z;}else if(type==0x33320002){if(c->pixels)c->malformed=1;c->pixels=b;c->pixels_len=z;}else if(type&0x80000000u)leaves(b,z,c,depth+1);at+=8+z;}}
static int decode_collection(const unsigned char*base,const unsigned char*d,size_t n,uint32_t key,HaPixels*out){Collection c={0};leaves(d,n,&c,0);if(c.malformed)return -1;if(!c.catalog||!c.formats||!c.pixels)return 0;if(c.catalog_len%124||c.formats_len%32||c.catalog_len/124!=c.formats_len/32)return -1;
 size_t payload=(size_t)(c.pixels-base),padding=(128-payload%128)%128;if(padding>c.pixels_len)return -1;const unsigned char*data=c.pixels+padding;size_t available=c.pixels_len-padding;
 for(size_t i=0;i<c.catalog_len/124;i++){const unsigned char*r=c.catalog+i*124;if(rd32(r+36)!=key)continue;size_t io=rd32(r+48),po=rd32(r+52),stored=rd32(r+56),pal=rd32(r+60),top=rd32(r+64);unsigned w=rd16(r+68),h=rd16(r+70);uint32_t format=rd32(c.formats+i*32+20);
 if(!w||!h||top>stored||io>available||stored>available-io||po>available||pal>available-po)return -1;
 if(format==41&&top!=(size_t)w*h)return -1;
 return ha_texture_decode(format,w,h,data+io,top,data+po,pal,out);
 }return 0;}
static int scan(const unsigned char*base,const unsigned char*d,size_t n,uint32_t key,HaPixels*out,unsigned depth){if(depth>64)return -1;int failure=0;for(size_t at=0;at<n;){if(n-at<8)return failure?failure:-1;uint32_t type=rd32(d+at);size_t z=rd32(d+at+4);if(z>n-at-8)return -1;int r=0;if(type==0xb3300000u)r=decode_collection(base,d+at+8,z,key,out);else if(type&0x80000000u)r=scan(base,d+at+8,z,key,out,depth+1);if(r==1)return 1;if(r<0)failure=r;at+=8+z;}return failure;}
int ha_texture_find(const unsigned char*d,size_t n,uint32_t key,HaPixels*out){memset(out,0,sizeof *out);return scan(d,d,n,key,out,0);}
