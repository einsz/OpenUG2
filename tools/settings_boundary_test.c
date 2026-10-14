/* Synthetic settings trust-boundary checks. Include the backend only to inject
 * allocation failures; no production hooks or external/retail files. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#endif
static long fail_after = -1;
static size_t allocation_limit = (size_t)-1;
static void *test_malloc(size_t n) {
    if (n > allocation_limit) return NULL;
    if (fail_after == 0) { fail_after = -1; return NULL; }
    if (fail_after > 0) fail_after--;
    return malloc(n);
}
#define malloc test_malloc
#include "../src/settings.c"
#undef malloc
#ifdef _WIN32
#define rmdir _rmdir
#endif
static const char *scratch = "build/settings_boundary_tmp";
static void put_file(const char *path, const char *text) {
    FILE *f = fopen(path,"wb");assert(f);assert(fwrite(text,1,strlen(text),f)==strlen(text));assert(!fclose(f));
}
static int intact(const char *path) {
    char data[16]={0};FILE *f=fopen(path,"rb");if(!f)return 0;size_t n=fread(data,1,15,f);fclose(f);return n==8&&!memcmp(data,"KEEPTHIS",8);
}
static void test_numeric_and_container(void) {
    StGraphics g;StGfxCaps c;st_gfx_defaults(&g);st_gfx_caps_full(&c,5);
    g.v[ST_GFX_RESOLUTION]=INT_MAX;assert(!st_resolution_step(&g,&c,1));assert(g.v[ST_GFX_RESOLUTION]==INT_MAX);
    g.v[ST_GFX_RESOLUTION]=INT_MIN;assert(!st_resolution_step(&g,&c,-1));
    uint8_t bytes[64];assert(!st_container_build(bytes,sizeof bytes,NULL,4,NULL,0));
    assert(!st_container_build(bytes,sizeof bytes,NULL,0,NULL,4));
    StContainerView view;memset(bytes,0,sizeof bytes);
    for(size_t n=0;n<28;n++)assert(st_container_parse(bytes,n,&view)!=ST_OK);
    memcpy(bytes,"20CM",4);wr32(bytes+4,28);wr32(bytes+8,UINT_MAX);wr32(bytes+12,UINT_MAX);wr32(bytes+24,st_checksum(bytes,24));
    assert(st_container_parse(bytes,28,&view)==ST_E_IO);
    StProfile *p=malloc(sizeof *p);assert(p);memset(p,0xff,sizeof *p);
    uint8_t *out=malloc(ST_PROFILE_FILE_SIZE);assert(out);assert(st_profile_serialize(p,out,ST_PROFILE_FILE_SIZE)==ST_PROFILE_FILE_SIZE);
    assert(!memcmp(out+36+53761,p->name,32));
    assert(st_profile_save(scratch,p)==ST_E_ARG);assert(!st_profile_name_valid(p->name));free(out);free(p);
}
static void test_memory_failure(int check_old) {
    char path[1100];snprintf(path,sizeof path,"%s/oom.ini",scratch);remove(path);
    StRegStore s={0};assert(st_reg_open_ini(&s,path)==ST_OK);assert(st_reg_set_string(&s,"Language","English US")==ST_OK);
    fail_after=0;assert(st_reg_set_string(&s,"Language","French")==ST_E_NOMEM);fail_after=-1;
    char text[32];if(check_old)assert(st_reg_get_string(&s,"Language",text,sizeof text)&&!strcmp(text,"English US"));
    assert(st_reg_set_dword(&s,"g_RainEnable",1)==ST_OK);st_reg_close(&s);
    fail_after=1;assert(st_reg_open_ini(&s,path)==ST_E_NOMEM);fail_after=-1;assert(!s.ops);st_reg_close(&s);
    assert(st_reg_open_ini(&s,scratch)==ST_E_IO);assert(!s.ops);
}
static void test_ini_strings(void) {
    char path[1100];snprintf(path,sizeof path,"%s/text.ini",scratch);remove(path);
    StRegStore s={0};assert(st_reg_open_ini(&s,path)==ST_OK);
    const char *value="path\\quoted\"\nInjected=dword:00000001\r\ttail";
    assert(st_reg_set_string(&s,"Install Dir",value)==ST_OK);
    assert(st_reg_set_dword(&s,"bad\nName",1)==ST_E_ARG);
    char longvalue[8192];memset(longvalue,'x',sizeof longvalue-1);longvalue[sizeof longvalue-1]=0;
    assert(st_reg_set_string(&s,"Long",longvalue)==ST_OK);st_reg_close(&s);
    assert(st_reg_open_ini(&s,path)==ST_OK);char text[8192];
    assert(st_reg_get_string(&s,"Install Dir",text,sizeof text)&&!strcmp(text,value));assert(!st_reg_exists(&s,"Injected"));
    assert(st_reg_get_string(&s,"Long",text,sizeof text)&&!strcmp(text,longvalue));st_reg_close(&s);
    put_file(path,"[Software\\EA Games\\Need for Speed Underground 2]\nBad=dword:100000000\nOpen=\"unterminated\n");
    assert(st_reg_open_ini(&s,path)==ST_E_CORRUPT);assert(!s.ops);remove(path);
}
#ifndef _WIN32
static void test_link_boundaries(const char *which) {
    char root[1100],victim[1100],dir[1100],file[1100],target[1100];
    snprintf(root,sizeof root,"%s/root",scratch);snprintf(victim,sizeof victim,"%s/victim",scratch);
    assert(make_dirs(root)==ST_OK&&make_dirs(victim)==ST_OK);
    snprintf(dir,sizeof dir,"%s/Linked",root);snprintf(target,sizeof target,"%s/Linked",victim);
    put_file(target,"KEEPTHIS");remove(dir);assert(!symlink("../victim",dir));
    if(!strcmp(which,"delete")) {assert(st_profile_delete(root,"Linked")!=ST_OK);assert(intact(target));}
    else if(!strcmp(which,"save-dir")) {static StProfile p;st_profile_defaults(&p,0);strcpy(p.name,"Linked");assert(st_profile_save(root,&p)!=ST_OK);assert(intact(target));}
    remove(dir);remove(target);
    snprintf(dir,sizeof dir,"%s/Real",root);assert(make_dir(dir)==ST_OK);
    snprintf(file,sizeof file,"%s/Real",dir);snprintf(target,sizeof target,"%s/file",victim);put_file(target,"KEEPTHIS");remove(file);
    assert(!symlink("../../victim/file",file));
    if(!strcmp(which,"save-file")) {static StProfile p;st_profile_defaults(&p,0);strcpy(p.name,"Real");assert(st_profile_save(root,&p)!=ST_OK);assert(intact(target));}
    remove(file);remove(target);rmdir(dir);
}
static void test_temp(void) {
    char path[1100],tmp[1100],victim[1100];snprintf(path,sizeof path,"%s/temp.ini",scratch);
    snprintf(tmp,sizeof tmp,"%s.tmp",path);snprintf(victim,sizeof victim,"%s/untouched",scratch);put_file(victim,"KEEPTHIS");remove(tmp);
    assert(!symlink("untouched",tmp));StRegStore s={0};assert(st_reg_open_ini(&s,path)==ST_OK);
    assert(st_reg_set_dword(&s,"g_RainEnable",1)==ST_OK);st_reg_close(&s);assert(intact(victim));remove(tmp);remove(victim);remove(path);
}
#endif
#ifndef _WIN32
static void test_default_path_bounds(void) {
    const char *old_home = getenv("HOME");char *saved = old_home ? dup_str(old_home) : NULL;
    char home[950], out[1200];home[0]='/';
    for(size_t i=1;i<sizeof home-1;i++)home[i]=i%9==0?'/':'a';home[sizeof home-1]=0;
    assert(setenv("HOME",home,1)==0);
    assert(st_default_config_path(out,sizeof out)==ST_E_ARG);
    assert(st_default_profile_root(out,sizeof out)==ST_E_ARG);
    if(saved){assert(setenv("HOME",saved,1)==0);free(saved);}else assert(unsetenv("HOME")==0);
}
#endif
static void test_large_profile(void) {
    char root[1100],dir[1100],path[1100];snprintf(root,sizeof root,"%s/large",scratch);
    snprintf(dir,sizeof dir,"%s/Large",root);assert(make_dirs(dir)==ST_OK);snprintf(path,sizeof path,"%s/Large",dir);
    FILE *f=fopen(path,"wb");assert(f);assert(!fseek(f,16*1024*1024,SEEK_SET));assert(fputc(0,f)!=EOF);assert(!fclose(f));
    static StProfile p;allocation_limit=ST_PROFILE_FILE_SIZE;int rc=st_profile_load(root,"Large",&p);allocation_limit=(size_t)-1;
    assert(rc==ST_E_CORRUPT);remove(path);
    /* Declared extra A bytes and trailing slack remain valid. */
    static uint8_t payload[ST_PAYLOAD_SIZE], bytes[ST_PROFILE_FILE_SIZE+8192+16];
    uint8_t a[8192]={0};a[4000]=0x57;wr32(a,ST_PROFILE_VERSION);wr32(a+4,ST_PAYLOAD_SIZE);
    st_profile_defaults(&p,0);strcpy(p.name,"Large");st_profile_pack_payload(&p,payload);
    size_t n=st_container_build(bytes,sizeof bytes,a,sizeof a,payload,sizeof payload);assert(n);
    memset(bytes+n,0,16);n+=16;wr32(bytes+4,(uint32_t)n);wr32(bytes+24,st_checksum(bytes,24));
    f=fopen(path,"wb");assert(f);assert(fwrite(bytes,1,n,f)==n);assert(!fclose(f));
    allocation_limit=ST_PROFILE_FILE_SIZE;assert(st_profile_load(root,"Large",&p)==ST_OK);allocation_limit=(size_t)-1;
    assert(!strcmp(p.name,"Large")&&p.audio.master==1.0f);remove(path);rmdir(dir);
}
int main(int argc,char **argv) {
    if(argc>2)scratch=argv[2];assert(make_dirs(scratch)==ST_OK);
    const char *mode=argc>1?argv[1]:"all";
    if(!strcmp(mode,"all")||!strcmp(mode,"numeric"))test_numeric_and_container();
    if(!strcmp(mode,"all")||!strcmp(mode,"oom"))test_memory_failure(1);
    if(!strcmp(mode,"oom-crash"))test_memory_failure(0);
    if(!strcmp(mode,"load-oom")||!strcmp(mode,"read-error")) {
        char path[1100];snprintf(path,sizeof path,"%s/load.ini",scratch);
        put_file(path,"[Software\\EA Games\\Need for Speed Underground 2]\nLanguage=\"English US\"\n");
        StRegStore s={0};
        if(!strcmp(mode,"load-oom")){fail_after=1;assert(st_reg_open_ini(&s,path)==ST_E_NOMEM);fail_after=-1;}
        else assert(st_reg_open_ini(&s,scratch)==ST_E_IO);
        assert(!s.ops);st_reg_close(&s);remove(path);
    }
    if(!strcmp(mode,"all")||!strcmp(mode,"ini"))test_ini_strings();
    if(!strcmp(mode,"roundtrip")) {
        char path[1100];snprintf(path,sizeof path,"%s/roundtrip.ini",scratch);StRegStore s={0};
        assert(st_reg_open_ini(&s,path)==ST_OK);const char *v="one\nInjected=dword:00000001\r\ttwo";
        assert(st_reg_set_string(&s,"Install Dir",v)==ST_OK);st_reg_close(&s);assert(st_reg_open_ini(&s,path)==ST_OK);
        char text[100];assert(st_reg_get_string(&s,"Install Dir",text,sizeof text)&&!strcmp(text,v));assert(!st_reg_exists(&s,"Injected"));st_reg_close(&s);remove(path);
    }
#ifndef _WIN32
    if(!strcmp(mode,"all")||!strcmp(mode,"delete"))test_link_boundaries("delete");
    if(!strcmp(mode,"all")||!strcmp(mode,"save-dir"))test_link_boundaries("save-dir");
    if(!strcmp(mode,"all")||!strcmp(mode,"save-file"))test_link_boundaries("save-file");
    if(!strcmp(mode,"all")||!strcmp(mode,"temp"))test_temp();
    if(!strcmp(mode,"all")||!strcmp(mode,"paths"))test_default_path_bounds();
#endif
    if(!strcmp(mode,"all")||!strcmp(mode,"large"))test_large_profile();
    puts("settings_boundary_test: PASS (parser, allocation rollback, INI round trip, paths)");return 0;
}
