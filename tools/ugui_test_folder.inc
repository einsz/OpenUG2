/* CONTROLLER_GLYPH_FOLDER F-1..F-14: synthetic PNGs only. The user corpus
 * is tested separately by the runtime loader, never copied into tests. */
#include "ugui_glyph_folder.h"
#include <png.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <unistd.h>

static void folder_digest(const void *data,size_t n,char hex[65]) {
    unsigned char digest[32];SHA256(data,n,digest);
    for(int i=0;i<32;i++)sprintf(hex+2*i,"%02x",digest[i]);
}
static void folder_fixture(const char *root,UgGlyphFolderEntry e[66]) {
    const UgGlyphFolderEntry *approved=ug_glyph_folder_index(NULL);
    uint8_t rgba[4096];for(int i=0;i<1024;i++){rgba[4*i]=19;rgba[4*i+1]=83;rgba[4*i+2]=141;rgba[4*i+3]=(uint8_t)(i%256);}
    char path[640];snprintf(path,sizeof path,"%s/ps2",root);CHECK(!mkdir(path,0700));
    snprintf(path,sizeof path,"%s/xbox",root);CHECK(!mkdir(path,0700));
    /* Every record gets independently identified synthetic bytes. Same pixels
     * intentionally verify that equal artwork does not merge identities. */
    for(int i=0;i<66;i++) {
        e[i]=approved[i];snprintf(path,sizeof path,"%s/%s",root,e[i].file);
        char *slash=strrchr(path,'/');*slash=0;if(access(path,F_OK))CHECK(!mkdir(path,0700));*slash='/';
        png_image image;memset(&image,0,sizeof image);image.version=PNG_IMAGE_VERSION;image.width=image.height=32;image.format=PNG_FORMAT_RGBA;
        CHECK(png_image_write_to_file(&image,path,0,rgba,0,NULL));
        FILE *f=fopen(path,"rb");CHECK(f!=NULL);if(!f)continue;uint8_t bytes[4096];size_t n=fread(bytes,1,sizeof bytes,f);fclose(f);
        folder_digest(bytes,n,e[i].png_sha256);folder_digest(rgba,sizeof rgba,e[i].rgba_sha256);
    }
}
static int folder_count(const UgController *p,UgControllerFamily f) {
    int n=0;for(int i=0;i<ug_controller_family_icon_count(f);i++)if(ug_controller_family_icon_texture(p,f,i))n++;return n;
}
static void test_folder_index_vectors(void) {
    char *txt=read_file("controller-glyph-folder-index.json",NULL);CHECK(txt!=NULL);if(!txt)return;
    jp=txt;J *v=jparse();CHECK(!strcmp(jstrv(v,"schema"),"controller-glyph-folder-v1"));
    size_t n;const UgGlyphFolderEntry *index=ug_glyph_folder_index(&n);const J *entries=jget(v,"entries");CHECK(n==66&&entries->n==66);
    for(size_t i=0;i<n;i++) {
        const J *e=entries->kids[i];const UgGlyphFolderEntry *a=index+i;
        CHECK(a->family==(!strcmp(jstrv(e,"family"),"ps2")?UG_CONTROLLER_PS2:UG_CONTROLLER_XBOX));
        CHECK(a->key==(uint32_t)strtoul(jstrv(e,"key"),NULL,16));CHECK(!strcmp(a->file,jstrv(e,"file")));
        CHECK(a->width==jnum(e,"width")&&a->height==jnum(e,"height"));
        CHECK(!strcmp(a->category,jstrv(e,"category")));
        CHECK(!strcmp(a->png_sha256,jstrv(e,"png_sha256")));CHECK(!strcmp(a->rgba_sha256,jstrv(e,"rgba_sha256")));
        const J *lang=jget(e,"language");CHECK(lang&&(!strcmp(a->language,lang->str?lang->str:"")));
    }
    jfree(v);free(txt);
}
static void test_folder_cases(void) {
    char *txt=read_file("controller-glyph-folder-cases.json",NULL);CHECK(txt!=NULL);if(!txt)return;
    jp=txt;J *v=jparse();const J *cases=jget(v,"cases");CHECK(cases&&cases->n==14);
    char root[]="/tmp/ugui_folder_XXXXXX";CHECK(mkdtemp(root)!=NULL);UgGlyphFolderEntry e[66];folder_fixture(root,e);
    int cross=-1,xa=-1;for(int i=0;i<66;i++){if(e[i].family==UG_CONTROLLER_PS2&&e[i].key==0xe795717d)cross=i;if(e[i].family==UG_CONTROLLER_XBOX&&e[i].key==0x8522e0a0)xa=i;}
    CHECK(cross>=0&&xa>=0);
    for(int c=0;c<14;c++) {
        const char *id=jstrv(cases->kids[c],"id");char expected[16];snprintf(expected,sizeof expected,"F-%d",c+1);CHECK(!strcmp(id,expected));
        UgGlyphFolderEntry mutated[66];memcpy(mutated,e,sizeof e);char path[640],backup[660];int altered=-1;
        if(c==5||c==6||c==11||c==13) {
            altered=c==5||c==11?cross:xa;snprintf(path,sizeof path,"%s/%s",root,e[altered].file);snprintf(backup,sizeof backup,"%s.saved",path);CHECK(!rename(path,backup));
            if(c==6||c==13){FILE *f=fopen(path,"wb");CHECK(f!=NULL);if(f){fwrite(c==6?"bad PNG":"wrong dimensions",1,c==6?7:16,f);fclose(f);}}
            if(c==11)CHECK(!symlink(backup,path)); /* outside root below replaces this link */
            if(c==11){CHECK(!unlink(path));CHECK(!symlink("/etc/passwd",path));}
        }
        if(c==12)mutated[1]=mutated[0];
        if(c==10){char extra[640];snprintf(extra,sizeof extra,"%s/unindexed.png",root);FILE *f=fopen(extra,"wb");CHECK(f!=NULL);if(f)fclose(f);}
        UgController p;UgTexLib lib;ug_controller_init(&p);ug_texlib_init(&lib);
        int loaded=ug_controller_load_folder_index(&p,&lib,c==4?"/tmp/ugui-no-such-folder-0123456789":root,mutated,66);
        if(c==4){CHECK(!loaded);CHECK(!folder_count(&p,UG_CONTROLLER_PS2));CHECK(!ug_controller_family_available(&p,UG_CONTROLLER_PS2));}
        else if(c==12){CHECK(!loaded);CHECK(p.folder_index_failed);CHECK(!lib.n);}
        else {
            CHECK(loaded);CHECK(p.family==UG_CONTROLLER_PC);CHECK(folder_count(&p,UG_CONTROLLER_PS2)==(c==5||c==11?18:19));CHECK(folder_count(&p,UG_CONTROLLER_XBOX)==(c==6||c==13?46:47));
            if(c==1){int languages[3]={0};for(int i=0;i<66;i++)if(e[i].family==UG_CONTROLLER_XBOX){if(!strcmp(e[i].language,"french"))languages[0]++;if(!strcmp(e[i].language,"italian"))languages[1]++;if(!strcmp(e[i].language,"swedish"))languages[2]++;}CHECK(languages[0]==8&&languages[1]==8&&languages[2]==8);}
            UgControllerFamily family=c==1||c==3||c==6||c==9||c==13?UG_CONTROLLER_XBOX:UG_CONTROLLER_PS2;
            CHECK(ug_controller_select(&p,family)==UG_VIS_OK);
            CHECK(ug_controller_token(&p,"ACCEPT",NULL)==UG_VIS_UNBOUND);
            if(c==2||c==3||c==5||c==6){int tex=0;CHECK(ug_controller_bind(&p,"ACCEPT",family==UG_CONTROLLER_PS2?0xe795717d:0x8522e0a0)==UG_VIS_OK);CHECK(ug_controller_bind(&p,"CANCEL",family==UG_CONTROLLER_PS2?0xd9a22505:0x8522e0a1)==UG_VIS_OK);CHECK(ug_controller_token(&p,"CANCEL",&tex)==UG_VIS_OK&&tex>0);CHECK(ug_controller_token(&p,"ACCEPT",&tex)==(c==5||c==6?UG_VIS_UNAVAILABLE:UG_VIS_OK));}
            if(c==7){CHECK(ug_controller_bind(&p,"ACCEPT",0x8522e0a0)==UG_VIS_UNAVAILABLE);CHECK(ug_controller_token(&p,"ACCEPT",NULL)==UG_VIS_UNAVAILABLE);}
            if(altered>=0)CHECK(p.folder_records[altered].status==(c==5?UG_FOLDER_MISSING:c==11?UG_FOLDER_ESCAPE:UG_FOLDER_HASH));
        }
        ug_texlib_free(&lib);
        if(altered>=0){if(c!=5)CHECK(!unlink(path));CHECK(!rename(backup,path));}
    }
    /* Fixtures deliberately contain neither a manifest nor a README. */
    for(int i=0;i<66;i++){char path[640];snprintf(path,sizeof path,"%s/%s",root,e[i].file);CHECK(!unlink(path));}
    const char *dirs[]={"xbox/french","xbox/italian","xbox/swedish","ps2","xbox"};
    for(size_t i=0;i<sizeof dirs/sizeof dirs[0];i++){char path[640];snprintf(path,sizeof path,"%s/%s",root,dirs[i]);rmdir(path);}
    char extra[640];snprintf(extra,sizeof extra,"%s/unindexed.png",root);unlink(extra);CHECK(!rmdir(root));jfree(v);free(txt);
}
static void test_folder_rejections(void) {
    char root[]="/tmp/ugui_folder_bad_XXXXXX";CHECK(mkdtemp(root)!=NULL);UgGlyphFolderEntry e[66];folder_fixture(root,e);
    UgGlyphFolderEntry bad[66];UgController p;UgTexLib lib;
    const char *badpaths[]={"/etc/passwd","../outside.png","ps2/../../outside.png","ps2//a.png","ps2/./a.png","ps2/"};
    for(size_t k=0;k<sizeof badpaths/sizeof badpaths[0];k++) {
        memcpy(bad,e,sizeof bad);snprintf(bad[0].file,sizeof bad[0].file,"%s",badpaths[k]);ug_controller_init(&p);ug_texlib_init(&lib);
        CHECK(!ug_controller_load_folder_index(&p,&lib,root,bad,66));CHECK(p.folder_index_failed&&!lib.n);ug_texlib_free(&lib);
    }
    char file[640];snprintf(file,sizeof file,"%s/%s",root,e[0].file);uint8_t rgba[4096];memset(rgba,0x67,sizeof rgba);
    for(int k=0;k<6;k++) {
        memcpy(bad,e,sizeof bad);png_image image;memset(&image,0,sizeof image);image.version=PNG_IMAGE_VERSION;image.width=k==0?31:32;image.height=32;image.format=k==1?PNG_FORMAT_RGB:PNG_FORMAT_RGBA;
        CHECK(png_image_write_to_file(&image,file,0,rgba,0,NULL));
        FILE *f=fopen(file,"rb");CHECK(f!=NULL);uint8_t bytes[8192];size_t n=fread(bytes,1,sizeof bytes,f);fclose(f);
        if(k==2)bytes[29]^=1; /* invalid IHDR CRC, with resealed file identity */
        if(k==3)n-=6; /* truncation past image data */
        if(k==4)memset(bad[0].rgba_sha256,'0',64);
        if(k==5){memset(bytes,0,sizeof bytes);n=sizeof bytes;}
        f=fopen(file,"wb");CHECK(f!=NULL);CHECK(fwrite(bytes,1,n,f)==n);fclose(f);folder_digest(bytes,n,bad[0].png_sha256);
        ug_controller_init(&p);ug_texlib_init(&lib);CHECK(ug_controller_load_folder_index(&p,&lib,root,bad,66));
        CHECK(!p.folder_records[0].texture);CHECK(folder_count(&p,UG_CONTROLLER_PS2)==18);CHECK(folder_count(&p,UG_CONTROLLER_XBOX)==47);
        CHECK(p.folder_records[0].status==(k==0?UG_FOLDER_DIMENSIONS:k==1?UG_FOLDER_UNSUPPORTED:k==2||k==3?UG_FOLDER_PNG:k==4?UG_FOLDER_RGBA:UG_FOLDER_LIMIT));
        ug_texlib_free(&lib);
    }
    for(int i=0;i<66;i++){snprintf(file,sizeof file,"%s/%s",root,e[i].file);CHECK(!unlink(file));}
    snprintf(file,sizeof file,"%s/ps2",root);CHECK(!rmdir(file));snprintf(file,sizeof file,"%s/xbox",root);CHECK(!rmdir(file));CHECK(!rmdir(root));
}
/* Explicit opt-in through this recreated loader only. No art export or goldens. */
static void runtime_test_glyph_folder(const char *root) {
    UgController p;UgTexLib lib;ug_controller_init(&p);ug_texlib_init(&lib);
    CHECK(ug_controller_load_folder(&p,&lib,root));CHECK(p.family==UG_CONTROLLER_PC);CHECK(p.folder_count==66);
    for(size_t i=0;i<p.folder_count;i++) {
        printf("TEST-ONLY PNG loader: family=%d key=%08x status=%s\n",p.folder_records[i].family,p.folder_records[i].key,ug_glyph_folder_status(p.folder_records[i].status));
        CHECK(p.folder_records[i].status==UG_FOLDER_OK);CHECK(p.folder_records[i].texture>0);
    }
    CHECK(folder_count(&p,UG_CONTROLLER_PS2)==19);CHECK(folder_count(&p,UG_CONTROLLER_XBOX)==47);
    ug_texlib_free(&lib);
}
static void run_folder_tests(void){RUN(test_folder_index_vectors);RUN(test_folder_cases);RUN(test_folder_rejections);}
