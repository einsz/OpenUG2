#ifndef HUD_TEXTURE_H
#define HUD_TEXTURE_H
#include <stdint.h>
#include <stddef.h>
typedef struct { unsigned char *rgba; unsigned width,height; uint32_t format; } HaPixels;
/* 1 decoded; 0 absent; -1 malformed; -2 unsupported. */
int ha_texture_find(const unsigned char *data,size_t len,uint32_t key,HaPixels *out);
int ha_texture_decode(uint32_t format,unsigned width,unsigned height,const unsigned char *image,size_t image_len,const unsigned char *palette,size_t palette_len,HaPixels *out);
#endif
