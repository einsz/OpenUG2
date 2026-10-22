/* Independent folder source; CONTROLLER_GLYPH_FOLDER requirements 1-9.
 * Public libpng provides standard decoding; OpenSSL provides SHA-256. */
#include "ugui_glyph_folder.h"
#ifndef _WIN32
#include <png.h>
#include <openssl/sha.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ugui_glyph_folder_index.h"
#define FOLDER_INPUT_CAP 4096u

const UgGlyphFolderEntry *ug_glyph_folder_index(size_t *count){if(count)*count=66;return folder_index;}
const char *ug_glyph_folder_status(UgGlyphFolderStatus s) {
#ifdef _WIN32
    if(s==UG_FOLDER_UNSUPPORTED)return "folder source unsupported on Windows";
#endif
    static const char *labels[]={"available","source absent/unreadable","missing file","unreadable file","path containment rejected","PNG identity/hash mismatch","invalid PNG","unsupported PNG representation","wrong dimensions","decoded RGBA validation failed","input/allocation limit","index configuration failure"};
    return (unsigned)s<sizeof labels/sizeof labels[0]?labels[s]:"unknown";
}
static int terminated(const char *s,size_t n){return memchr(s,0,n)!=NULL;}
static int digest_valid(const char *s){if(!terminated(s,65)||strlen(s)!=64)return 0;for(int i=0;i<64;i++)if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')))return 0;return 1;}
#ifndef _WIN32
static int digest_matches(const void *bytes,size_t n,const char *expected) {
    uint8_t digest[32];char hex[65];if(!SHA256(bytes,n,digest))return 0;
    for(int i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",digest[i]);
    return !strcmp(hex,expected);
}
#endif
static int relative_valid(const char *s) {
    if(!terminated(s,160)||!*s||s[0]=='/'||strchr(s,'\\'))return 0;
    for(const char *p=s;*p;){const char *end=strchr(p,'/');size_t n=end?(size_t)(end-p):strlen(p);
        if(!n||(n==1&&p[0]=='.')||(n==2&&p[0]=='.'&&p[1]=='.'))return 0;
        if(!end)return 1;
        p=end+1;if(!*p)return 0;
    }return 0;
}
static int index_valid(const UgGlyphFolderEntry *e,size_t n) {
    if(!e||!n||n>66)return 0;
    for(size_t i=0;i<n;i++) {
        const UgGlyphFolderEntry *a=e+i;
        if((a->family!=UG_CONTROLLER_PS2&&a->family!=UG_CONTROLLER_XBOX)||!a->key||a->width!=32||a->height!=32||!relative_valid(a->file)||!digest_valid(a->png_sha256)||!digest_valid(a->rgba_sha256)||!terminated(a->category,sizeof a->category)||!terminated(a->language,sizeof a->language))return 0;
        if(!strcmp(a->category,"base")){if(*a->language)return 0;}
        else if(strcmp(a->category,"localized")||a->family!=UG_CONTROLLER_XBOX||(strcmp(a->language,"french")&&strcmp(a->language,"italian")&&strcmp(a->language,"swedish")))return 0;
        for(size_t j=0;j<i;j++)if((e[j].family==a->family&&e[j].key==a->key)||!strcmp(e[j].file,a->file))return 0;
    }return 1;
}
#ifndef _WIN32
/* Each component opens relative to an already-open directory, with no-follow.
 * Rejecting even internal symlinks makes escapes impossible without checking
 * a path and subsequently reopening it (which would race a symlink change). */
static int contained_open(int root,const char *path,UgGlyphFolderStatus *status) {
    char copy[160];snprintf(copy,sizeof copy,"%s",path);int dir=dup(root);if(dir<0){*status=UG_FOLDER_UNREADABLE;return -1;}
    char *part=copy;for(;;){char *slash=strchr(part,'/');if(slash)*slash=0;
        struct stat st;
        if(fstatat(dir,part,&st,AT_SYMLINK_NOFOLLOW)){*status=errno==ENOENT?UG_FOLDER_MISSING:UG_FOLDER_UNREADABLE;close(dir);return -1;}
        if(S_ISLNK(st.st_mode)){*status=UG_FOLDER_ESCAPE;close(dir);return -1;}
        int next=openat(dir,part,O_RDONLY|O_CLOEXEC|O_NOFOLLOW|(slash?O_DIRECTORY:O_NONBLOCK));int err=errno;close(dir);
        if(next<0){*status=err==ELOOP?UG_FOLDER_ESCAPE:err==ENOENT?UG_FOLDER_MISSING:UG_FOLDER_UNREADABLE;return -1;}
        if(!slash)return next;
        dir=next;part=slash+1;
    }
}
typedef struct {const uint8_t *bytes;size_t n,pos;} PngInput;
static void png_input(png_structp png,png_bytep out,png_size_t n) {
    PngInput *in=png_get_io_ptr(png);if(n>in->n-in->pos)png_error(png,"truncated");memcpy(out,in->bytes+in->pos,n);in->pos+=n;
}
static void png_failure(png_structp png,png_const_charp msg){(void)msg;png_longjmp(png,1);}
static void png_warning_ignored(png_structp png,png_const_charp msg){(void)png;(void)msg;}
static UgGlyphFolderStatus decode_png(const uint8_t *bytes,size_t n,uint8_t rgba[4096]) {
    if(n<33||png_sig_cmp(bytes,0,8))return UG_FOLDER_PNG;
    png_structp png=png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,png_failure,png_warning_ignored);if(!png)return UG_FOLDER_LIMIT;
    png_infop info=png_create_info_struct(png);if(!info){png_destroy_read_struct(&png,NULL,NULL);return UG_FOLDER_LIMIT;}
    if(setjmp(png_jmpbuf(png))){png_destroy_read_struct(&png,&info,NULL);return UG_FOLDER_PNG;}
    PngInput input={bytes,n,0};png_set_read_fn(png,&input,png_input);png_set_crc_action(png,PNG_CRC_ERROR_QUIT,PNG_CRC_ERROR_QUIT);
    png_set_user_limits(png,32,32);png_set_chunk_malloc_max(png,FOLDER_INPUT_CAP);png_set_chunk_cache_max(png,8);
    png_read_info(png,info);
    if(png_get_image_width(png,info)!=32||png_get_image_height(png,info)!=32){png_destroy_read_struct(&png,&info,NULL);return UG_FOLDER_DIMENSIONS;}
    if(png_get_bit_depth(png,info)!=8||png_get_color_type(png,info)!=PNG_COLOR_TYPE_RGBA||png_get_interlace_type(png,info)!=PNG_INTERLACE_NONE){png_destroy_read_struct(&png,&info,NULL);return UG_FOLDER_UNSUPPORTED;}
    if(png_get_rowbytes(png,info)!=128){png_destroy_read_struct(&png,&info,NULL);return UG_FOLDER_RGBA;}
    png_bytep rows[32];for(int y=0;y<32;y++)rows[y]=rgba+128*y;
    /* No alpha conversion, gamma/color transforms or premultiplication. */
    png_read_image(png,rows);png_read_end(png,info);png_destroy_read_struct(&png,&info,NULL);return UG_FOLDER_OK;
}
static UgGlyphFolderStatus read_record(int root,const UgGlyphFolderEntry *entry,uint8_t rgba[4096]) {
    UgGlyphFolderStatus status=UG_FOLDER_OK;int fd=contained_open(root,entry->file,&status);if(fd<0)return status;
    struct stat st;if(fstat(fd,&st)||!S_ISREG(st.st_mode)){close(fd);return UG_FOLDER_UNREADABLE;}
    if(st.st_size<=0||st.st_size>FOLDER_INPUT_CAP){close(fd);return UG_FOLDER_LIMIT;}
    uint8_t bytes[FOLDER_INPUT_CAP];size_t n=(size_t)st.st_size,pos=0;
    while(pos<n){ssize_t got=read(fd,bytes+pos,n-pos);if(got<0&&errno==EINTR)continue;if(got<=0){close(fd);return UG_FOLDER_UNREADABLE;}pos+=(size_t)got;}
    uint8_t extra;if(read(fd,&extra,1)!=0){close(fd);return UG_FOLDER_LIMIT;}close(fd);
    if(!digest_matches(bytes,n,entry->png_sha256))return UG_FOLDER_HASH;
    status=decode_png(bytes,n,rgba);if(status!=UG_FOLDER_OK)return status;
    return digest_matches(rgba,4096,entry->rgba_sha256)?UG_FOLDER_OK:UG_FOLDER_RGBA;
}
#endif
int ug_controller_load_folder_index(UgController *p,UgTexLib *lib,const char *root,const UgGlyphFolderEntry *entries,size_t count) {
    if(!p||!lib)return 0;
    p->folder_active=1;p->folder_count=0;p->folder_index_failed=0;p->available=p->xbox_available=0;
    memset(p->icons,0,sizeof p->icons);memset(p->xbox_icons,0,sizeof p->xbox_icons);memset(p->folder_records,0,sizeof p->folder_records);
    snprintf(p->folder_source,sizeof p->folder_source,"%s",root?root:"");p->lib=lib;
    if(!index_valid(entries,count)){p->folder_index_failed=1;snprintf(p->status,sizeof p->status,"TEST-ONLY PNG folder: index configuration failure");return 0;}
    p->folder_count=count;
    for(size_t i=0;i<count;i++){p->folder_records[i].family=entries[i].family;p->folder_records[i].key=entries[i].key;p->folder_records[i].status=UG_FOLDER_ABSENT;}
#ifdef _WIN32
    /* Fail closed until Windows has equivalent handle-relative no-reparse traversal. */
    for(size_t i=0;i<count;i++)p->folder_records[i].status=UG_FOLDER_UNSUPPORTED;
    snprintf(p->status,sizeof p->status,"TEST-ONLY PNG folder: unsupported on Windows");
    return 0;
#else
    int dir=root?open(root,O_RDONLY|O_DIRECTORY|O_CLOEXEC):-1;
    if(dir<0){snprintf(p->status,sizeof p->status,"TEST-ONLY PNG folder: source absent/unreadable");return 0;}
    int loaded=0;
    for(size_t i=0;i<count;i++) {
        UgGlyphFolderRecord *r=p->folder_records+i;uint8_t rgba[4096];r->status=read_record(dir,entries+i,rgba);
        if(r->status!=UG_FOLDER_OK)continue;
        if(lib->n==lib->cap){int cap=lib->cap+66;UgTexture *next=realloc(lib->t,(size_t)cap*sizeof *next);if(!next){r->status=UG_FOLDER_LIMIT;continue;}lib->t=next;lib->cap=cap;}
        uint8_t *pixels=malloc(4096);if(!pixels){r->status=UG_FOLDER_LIMIT;continue;}memcpy(pixels,rgba,4096);
        UgTexture *t=lib->t+lib->n;memset(t,0,sizeof *t);t->key=r->key;t->w=t->h=32;t->fmt=UG_PF_BGRA8;t->levels=1;t->rgba=pixels;
        snprintf(t->name,sizeof t->name,"%s %08x",entries[i].language[0]?entries[i].language:r->family==UG_CONTROLLER_PS2?"PS2":"Xbox",r->key);
        snprintf(t->source,sizeof t->source,"%s/%s",root,entries[i].file);r->texture=++lib->n;loaded++;
        for(int k=0;k<ug_controller_family_icon_count(r->family);k++)if(ug_controller_family_icon_key(r->family,k)==r->key){if(r->family==UG_CONTROLLER_PS2)p->icons[k]=r->texture;else p->xbox_icons[k]=r->texture;}
        if(r->family==UG_CONTROLLER_PS2)p->available=1;else p->xbox_available=1;
    }
    close(dir);ug_texlib_index(lib);ug_controller_select(p,p->family);return loaded>0;
#endif
}
int ug_controller_load_folder(UgController *p,UgTexLib *lib,const char *root){return ug_controller_load_folder_index(p,lib,root,folder_index,66);}
