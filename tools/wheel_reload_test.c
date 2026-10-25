/* Exercise the production transaction without a window or game assets.
 * Parser/upload boundaries inject failures; scene preparation and teardown run
 * normally. Include main.c to reach its shared static wheel loader. */
#include "render.h"
#include "rhi_gl.h"   /* before the stubs below rename its functions */
#include <assert.h>

static int test_stage, live_buffers, live_textures;
static int test_body_kit;
static int test_real_assets;
static GLuint next_handle = 1;
static unsigned char handles[65536];
static GLuint new_handle(int type) {
    assert(next_handle < sizeof handles);
    handles[next_handle] = (unsigned char)type;
    if (type == 1) live_buffers++; else live_textures++;
    return next_handle++;
}
static void drop_handle(GLuint id, int type) {
    if (!id) return;
    assert(id < next_handle && handles[id] == type);
    handles[id] = 0;
    if (type == 1) live_buffers--; else live_textures--;
}
static int test_load_car(const unsigned char *d, long len, N2Scene *s,
                         const uint32_t *keys, int nk, const N2CarConfig *cfg) {
    if(test_real_assets)return n2_load_car(d,len,s,keys,nk,cfg);
    (void)d; (void)len; (void)keys; (void)nk;
    s->count = s->cap = 2;
    s->meshes = calloc(2, sizeof *s->meshes);
    for (int i = 0; i < 2; i++) {
        N2Mesh *m = &s->meshes[i];
        const float v[] = {-1,0,-1,0,0, 1,0,-1,1,0, 1,0,1,1,1, -1,0,1,0,1};
        const uint16_t idx[] = {0,1,2,0,2,3};
        m->verts = malloc(sizeof v); memcpy(m->verts, v, sizeof v);
        m->idx = malloc(sizeof idx); memcpy(m->idx, idx, sizeof idx);
        m->nverts = 4; m->nidx = 6; m->texkey = 7;
        m->tierid = test_stage == 2 ? 0 : 1;
        m->vkind = test_stage == 3 ? 0 : 2; m->vnum = cfg->hood_style;
        if(test_body_kit) {
            m->cat=N2_CAR_BODY;m->texkey=8;m->famkey=test_stage==2?99:42;
            m->vkind=test_stage==3?0:1;m->vnum=cfg->body_kit;
            m->verts[1]=m->verts[6]=-.5f;m->verts[11]=m->verts[16]=.5f;
        }
        if (test_stage == 4) memset(m->verts, 0, sizeof v);
    }
    return test_stage == 1 ? 0 : s->count; /* partial parse must be released */
}
static int test_load_car_colored(const unsigned char *d, long len, N2Scene *s,
                                const uint32_t *keys, int nk,
                                const N2CarConfig *cfg, int prelight) {
    if (test_real_assets)
        return n2_load_car_colored(d,len,s,keys,nk,cfg,prelight);
    return test_load_car(d,len,s,keys,nk,cfg);
}
static int test_load_texture(const unsigned char *d, long len, uint32_t key, N2Tex *t) {
    if(test_real_assets)return n2_load_car_tex_by_key(d,len,key,t);
    (void)d; (void)len; (void)key;
    t->w = t->h = 1;
    t->rgb = malloc(3); memset(t->rgb, 127, 3);
    return test_stage != 5; /* partial decode must also be released */
}
static void test_free_scene_gpu(GpuMesh *gm, int n);
static GpuMesh *test_upload_scene(N2Scene *s) {
    if (test_stage == 6) return NULL;
    GpuMesh *gm = calloc(s->count, sizeof *gm);
    for (int i = 0; i < s->count; i++) {
        gm[i].vbo.id = new_handle(1); gm[i].nbo.id = new_handle(1);
        if (test_stage != 7) gm[i].ibo.id = new_handle(1);
    }
    /* The device runs out part-way: upload_scene releases what it made. */
    if (test_stage == 8) { test_free_scene_gpu(gm, s->count); return NULL; }
    return gm;
}
static void test_free_scene_gpu(GpuMesh *gm, int n) {
    if (!gm) return;
    for (int i = 0; i < n; i++) {
        drop_handle(gm[i].vbo.id, 1); drop_handle(gm[i].nbo.id, 1); drop_handle(gm[i].ibo.id, 1);
    }
    free(gm);
}
static RhiTexture test_upload_texture(const N2Tex *t) {
    (void)t;
    if (test_stage == 9) return (RhiTexture){0};
    RhiTexture made = {new_handle(2)};
    /* An upload the device rejects: the texture is released, {0} returned. */
    if (test_stage == 10) { drop_handle(made.id, 2); return (RhiTexture){0}; }
    return made;
}
static void test_destroy_texture(RhiTexture t) {
    drop_handle(t.id, 2);
}
static GLuint test_texture_name(RhiTexture t) {
    return t.id;   /* simulated: there is no GL texture behind the handle */
}
static void test_tex_parameter(GLenum target, GLenum name, GLint value) {
    (void)target; (void)name; (void)value;
}
#define n2_load_car test_load_car
#define n2_load_car_colored test_load_car_colored
#define n2_load_car_tex_by_key test_load_texture
#define upload_scene test_upload_scene
#define free_scene_gpu test_free_scene_gpu
#define upload_tpk_texture_to_gpu test_upload_texture
#define upload_tex test_upload_texture
#define upload_tpk_keyed(key, name, t) test_upload_texture(t)
#define upload_tex_keyed(key, name, t) test_upload_texture(t)
#define rhi_texture_destroy test_destroy_texture
#define rhi_gl_texture_name test_texture_name
#define render_texture_clamp(t) ((void)(t))   /* simulated textures have no sampler */
#define glTexParameteri test_tex_parameter
#define main openug_application_main
#include "../src/main.c"
#undef main

/* Optional local-data census; GPU handles remain simulated, no window needed. */
static void test_kit_archive(const char *root,const char *name) {
    char path[1024];long len=0,tlen=0;
    snprintf(path,sizeof path,"%s/CARS/%s/GEOMETRY.BIN",root,name);
    unsigned char *data=n2_read_file(path,&len);assert(data);
    snprintf(path,sizeof path,"%s/CARS/%s/TEXTURES.BIN",root,name);
    unsigned char *tex=n2_read_file(path,&tlen);assert(tex);
    uint32_t keys[512],cached[128];RhiTexture cached_tex[128];int ncached=0,first_cache=0;
    int nk=n2_car_tex_keys(tex,tlen,keys,512),kits[100];
    int n=n2_car_variant_numbers(data,len,1,kits,100);
    if(!n || kits[0]!=0) {
        assert(n<100);
        memmove(kits+1,kits,(size_t)n*sizeof *kits);kits[0]=0;n++;
    }
    test_real_assets=1;test_stage=0;
    BodyKitCandidate active={0};
    assert(n2_load_car(data,len,&active.scene,keys,nk,NULL)>0);
    for(int pass=0;pass<2;pass++) {
        for(int k=0;k<n;k++) {
            N2CarConfig cfg={.body_kit=kits[k]};BodyKitCandidate candidate={0};
            if(!prepare_body_kit(&candidate,data,len,keys,nk,&cfg,tex,tlen,
                                 &active.scene,cached,ncached,root)) {
                fprintf(stderr,"KIT CENSUS FAILED %s KIT%02d\n",name,kits[k]);abort();
            }
            for(int j=0;j<candidate.ntextures;j++) {
                cached[ncached]=candidate.keys[j];cached_tex[ncached++]=candidate.textures[j];
            }
            candidate.ntextures=0;
            body_kit_release(&active);active=candidate;
            assert(live_buffers==active.scene.count*3 && live_textures==ncached);
        }
        if(!pass)first_cache=ncached;else assert(ncached==first_cache);
    }
    body_kit_release(&active);for(int i=0;i<ncached;i++)test_destroy_texture(cached_tex[i]);
    assert(!live_buffers && !live_textures);
    free(data);free(tex);test_real_assets=0;
    printf("KIT CENSUS PASS %s: %d kits, two complete cycles, %d cached textures, no growing handle count\n",name,n,ncached);
}

static void test_parts_archive(const char *root,const char *name) {
    char path[1024];long len=0,tlen=0;
    snprintf(path,sizeof path,"%s/CARS/%s/GEOMETRY.BIN",root,name);
    unsigned char *data=n2_read_file(path,&len);assert(data);
    snprintf(path,sizeof path,"%s/CARS/%s/TEXTURES.BIN",root,name);
    unsigned char *tex=n2_read_file(path,&tlen);assert(tex);
    uint32_t keys[512];int nk=n2_car_tex_keys(tex,tlen,keys,512);
    test_real_assets=1;test_stage=0;
    N2Scene stock={0};assert(n2_load_car(data,len,&stock,keys,nk,NULL)>0);
    static N2PartMenu menus[N2_PART_COUNT];n2_mod_catalog(root,data,len,&stock,menus);
    N2CarConfig mixed={0};int count=0;
    for(int p=0;p<N2_PART_COUNT;p++)for(int o=1;o<menus[p].count;o++) {
        N2CarConfig cfg={0};cfg.parts[p]=menus[p].options[o].value;
        BodyKitCandidate candidate={0};next_handle=1;
        if(!prepare_body_kit(&candidate,data,len,keys,nk,&cfg,tex,tlen,&stock,NULL,0,root)) {
            fprintf(stderr,"PART CENSUS FAILED %s %s %s (%d)\n",name,n2_part_labels[p],menus[p].options[o].label,cfg.parts[p]);abort();
        }
        /* Every other local selection remains stock. Exhaust attachment may
         * move with a rear bumper, but its style must remain unchanged. */
        for(int i=0;i<candidate.scene.count;i++) {
            N2Mesh *m=candidate.scene.meshes+i;
            if(m->car_part && m->car_part!=p+1)assert(m->vnum==0);
        }
        mixed.parts[p]=cfg.parts[p];count++;
        body_kit_release(&candidate);assert(!live_buffers && !live_textures);
    }
    for(int pass=0;pass<2;pass++) {
        BodyKitCandidate candidate={0};next_handle=1;
        if(!prepare_body_kit(&candidate,data,len,keys,nk,&mixed,tex,tlen,&stock,NULL,0,root)) {
            fprintf(stderr,"MIXED PARTS FAILED %s\n",name);abort();
        }
        N2CarConfig reset={0};BodyKitCandidate restored={0};
        assert(prepare_body_kit(&restored,data,len,keys,nk,&reset,tex,tlen,
                               &candidate.scene,NULL,0,root));
        body_kit_release(&restored);
        body_kit_release(&candidate);assert(!live_buffers && !live_textures);
    }
    n2_free_scene(&stock);free(data);free(tex);test_real_assets=0;
    printf("PART CENSUS PASS %s: %d independent choices, mixed assembly and stock restoration\n",name,count);fflush(stdout);
}

static void test_car_switch_archive(const char *root) {
    char cars[64][64];int selected=0;
    int count=res_list_cars(root,cars,64,"",&selected);assert(count>0);
    long len=0;unsigned char *global=load_global_car_data(root,&len);assert(global && len>0);
    test_real_assets=1;test_stage=0;int factory=0,shared_windows=0,traffic=0;
    for(int i=0;i<count;i++) {
        next_handle=1;CarSwitchCandidate candidate={0};
        assert(prepare_car_switch(&candidate,root,cars[i]));
        int plates=0;
        for(int m=0;m<candidate.scene.count;m++)if(candidate.scene.meshes[m].car_material==N2_MAT_LICENSEPLATE) {
            N2Mesh *mesh=candidate.scene.meshes+m;int bound=0;
            for(int t=0;t<candidate.ntextures;t++)if(candidate.tex_keys[t]==mesh->texkey)bound=1;
            assert(mesh->texkey==N2_TEX_LICENSEPLATE && bound && mesh->car_mount==N2_MOUNT_BODY);
            for(int v=0;v<mesh->nverts;v++) {
                const float *p=mesh->verts+v*5;
                if(!strcmp(cars[i],"4DR_SEDAN02") || !strcmp(cars[i],"TAXI02"))assert(p[2]>.083f && p[2]<.229f);
                if(!strcmp(cars[i],"MINIVAN"))assert(p[2]>.279f && p[2]<.514f);
                if(!strcmp(cars[i],"PANELVAN"))assert(p[1]<-.068f && p[1]>-.518f);
            }
            plates++;
        }
        assert(plates==1);
        if(candidate.prelight)traffic++;
        if(candidate.prelight)for(int m=0;m<candidate.scene.count;m++)
            if(candidate.scene.meshes[m].cat==N2_CAR_GLASS){
                uint32_t key=candidate.scene.meshes[m].texkey;
                int bound=0;
                for(int t=0;t<candidate.ntextures;t++)if(candidate.tex_keys[t]==key)bound=1;
                assert(key==0x008a6835u && bound);
                shared_windows++;
            }
        if(candidate.prelight) {
            int skin=0,wheel_skin=0;
            for(int m=0;m<candidate.scene.count;m++) {
                N2Mesh *mesh=&candidate.scene.meshes[m];
                if(mesh->car_material!=0x19e0ecbeu && mesh->cat!=N2_CAR_TIRE)continue;
                assert(mesh->texkey && mesh->vcol);
                int bound=0;
                for(int t=0;t<candidate.ntextures;t++)if(candidate.tex_keys[t]==mesh->texkey)bound=1;
                assert(bound);
                if(mesh->cat==N2_CAR_TIRE)wheel_skin++;else skin++;
            }
            assert(skin && wheel_skin);
            BodyKitCandidate rebuilt={0};
            assert(prepare_body_kit(&rebuilt,candidate.data,candidate.len,
                candidate.keys,candidate.nkeys,&candidate.config,candidate.texdata,
                candidate.texlen,&candidate.scene,candidate.tex_keys,candidate.ntextures,root));
            assert(rebuilt.scene.count==candidate.scene.count && rebuilt.ntextures==0);
            for(int m=0;m<rebuilt.scene.count;m++) {
                assert(!!rebuilt.scene.meshes[m].vcol==!!candidate.scene.meshes[m].vcol);
                assert(rebuilt.scene.meshes[m].texkey==candidate.scene.meshes[m].texkey);
            }
            body_kit_release(&rebuilt);
        }
        int stock=candidate.stock_wheel;
        assert(stock>=0 && stock<candidate.scene.count);
        assert(candidate.scene.meshes[stock].car_mount==N2_MOUNT_WHEEL);
        int from_global=0;
        VehicleWheelConfig expected=wheel_config_for(cars[i],&candidate.profile,global,len,&from_global);
        factory+=from_global;
        printf("CAR SWITCH %s stance=%s\n",cars[i],from_global?"factory":"body fallback");
        assert(fabsf(candidate.wheel.front_axle-expected.front_axle)<1e-6f);
        assert(fabsf(candidate.wheel.rear_axle-expected.rear_axle)<1e-6f);
        assert(fabsf(candidate.wheel.front_track-expected.front_track)<1e-6f);
        assert(fabsf(candidate.wheel.rear_track-expected.rear_track)<1e-6f);
        N2CarShapeAttr shape;
        if(n2_global_car_shape(global,len,cars[i],&shape))for(int k=0;k<4;k++){
            assert(fabsf(candidate.profile.wheel_r*candidate.wheel.visual_radius_scale[k]-shape.wheel[k].radius)<1e-6f);
            assert(fabsf(candidate.profile.wheel_w*candidate.wheel.visual_width_scale[k]-shape.wheel[k].width)<1e-6f);
        }
        /* Same car, zero travel: AI body bottom vs the Stage5 player body
           bottom. Only the tyre-mesh vs source radius may separate them. */
        CtCar contact;
        if(stage5_car_read(global,len,cars[i],&contact)) {
            float floor=car_body_floor(&candidate.scene,candidate.bb[2]),bottom=0;
            for(int k=0;k<4;k++)bottom+=(contact.mount[k][2]-contact.radius[k])*.25f;
            float ride=candidate.profile.ride;
            float player=-bottom+body_ride_height(ride,floor,0)-ride+floor;
            float ai=ai_body_ride(&candidate.profile,floor,&candidate.wheel,1)+floor;
            printf("CAR SWITCH %s body bottom: AI %.3f player %.3f\n",cars[i],ai,player);
            assert(fabsf(ai-player)<.06f);
        }
        Stage1Drive untouched=g_stage1;
        publish_performance_source(candidate.has_physics?&candidate.physics:NULL,global,len,cars[i]);
        assert(!memcmp(&untouched,&g_stage1,sizeof untouched));
        if(!strcmp(cars[i],"CIVIC")||!strcmp(cars[i],"A3")){
            assert(g_dbg.perf_stock_available&&g_dbg.perf_mass==shape.mass_tonnes);
            assert(g_dbg.perf_radius[0]==shape.wheel[0].radius&&g_dbg.perf_width[2]==shape.wheel[2].width);
        }
        car_switch_release(&candidate);
        assert(!live_buffers && !live_textures);
    }
    printf("CAR SWITCH shared traffic windows: %d\n",shared_windows);
    assert(factory>0 && traffic>0 && shared_windows==traffic);test_real_assets=0;free(global);
    printf("CAR SWITCH PASS: all %d cars retain stock wheels and startup stance (%d factory); no leaked handles\n",count,factory);
}

static void test_vinyl_selection(void) {
    clear_car_vinyl();g_vinyl_data=calloc(1,1);g_vinyl_len=1;
    g_dbg.vinyl_count=2;g_dbg.vinyl_catalog_ready=1;
    g_vinyl_keys[0]=7;g_vinyl_keys[1]=8;test_stage=0;
    assert(select_car_vinyl(1));RhiTexture before=g_car_vinyl_tex;
    const int failures[]={5,9,10};
    for(int i=0;i<3;i++) {
        test_stage=failures[i];assert(!select_car_vinyl(2));
        assert(g_car_vinyl_tex.id==before.id && g_dbg.vinyl_current==1 && live_textures==1);
    }
    assert(!select_car_vinyl(-1) && !select_car_vinyl(3));
    test_stage=0;assert(select_car_vinyl(2));
    assert(g_car_vinyl_tex.id!=before.id && !handles[before.id] && live_textures==1);
    assert(select_car_vinyl(0) && !g_car_vinyl_tex.id && !live_textures);
    assert(select_car_vinyl(1));clear_car_vinyl();
    assert(!g_vinyl_data && !g_car_vinyl_tex.id && !g_dbg.vinyl_count &&
           !g_dbg.vinyl_catalog_ready && g_dbg.vinyl_request==-1 && !live_textures);
    puts("VINYL PASS: selection, replacement, removal, failed decode/upload rollback, car reset");
}

static void test_vinyl_archives(const char *root) {
    const char *cars[]={"MIATA","GOLF"};test_real_assets=1;test_stage=0;
    for(int c=0;c<2;c++) {
        clock_t start=clock();load_vinyl_catalog(root,cars[c]);
        assert(g_dbg.vinyl_count>1000 && g_dbg.vinyl_catalog_ready);
        for(int i=0;i<g_dbg.vinyl_count;i++)assert(g_vinyl_keys[i] && g_vinyl_names[i][0]);
        unsigned char *data=g_vinyl_data;load_vinyl_catalog(root,cars[c]);
        assert(data==g_vinyl_data); /* cached until the next car */
        assert(select_car_vinyl(1) && select_car_vinyl(g_dbg.vinyl_count));
        printf("VINYL ARCHIVE PASS %s: %d named entries, %.2f CPU seconds\n",
               cars[c],g_dbg.vinyl_count,(double)(clock()-start)/CLOCKS_PER_SEC);
        clear_car_vinyl();assert(!live_textures);
    }
    load_vinyl_catalog("/nonexistent-openug2-test-data","MISSING");
    assert(!g_dbg.vinyl_count && g_dbg.vinyl_catalog_ready && select_car_vinyl(0));
    clear_car_vinyl();test_real_assets=0;
}

/* Synthetic source sizes must affect only wheel geometry, never hubs/brakes. */
static void test_source_wheel_visual_fit(void) {
    unsigned char data[16+N2_GLOBAL_CAR_STRIDE]={0};
    uint32_t tag=0x34600,bytes=sizeof data-8;memcpy(data,&tag,4);memcpy(data+4,&bytes,4);memset(data+8,0x11,8);
    unsigned char *row=data+16;strcpy((char*)row,"FITCAR");strcpy((char*)row+0x40,"CARS\\FITCAR\\GEOMETRY.BIN");
    float value;
#define FIT_F(at,v) do{value=(v);memcpy(row+(at),&value,4);}while(0)
    FIT_F(0x220,1);FIT_F(0x224,4);FIT_F(0x228,2);FIT_F(0x22c,1.5f);
    FIT_F(0x230,1);FIT_F(0x244,1);FIT_F(0x258,1);
    for(uint32_t i=0;i<4;i++){int w=0x120+0x30*i;
        FIT_F(w,i<2?1.2f:-1.3f);FIT_F(w+4,(i&1)?-.8f:.8f);FIT_F(w+8,.15f);
        FIT_F(w+0x10,i<2?.3f:.32f);FIT_F(w+0x14,i<2?.24f:.26f);memcpy(row+w+0x18,&i,4);}
#undef FIT_F
    N2CarProfile profile={.wheel_r=.4f,.wheel_w=.2f,.ride=.4f};int source=0;
    VehicleWheelConfig wheel=wheel_config_for("FITCAR",&profile,data,sizeof data,&source);assert(source);
    PhysRideState ride={0};ride.compression[0]=.02f;
    float actual[4][16],brake[4][16];ai_wheel_matrices(&wheel,1,.3f,.2f,&ride,actual,brake);
    for(int i=0;i<4;i++){
        float radius=sqrtf(actual[i][0]*actual[i][0]+actual[i][1]*actual[i][1]+actual[i][2]*actual[i][2])*.4f;
        float width=sqrtf(actual[i][4]*actual[i][4]+actual[i][5]*actual[i][5]+actual[i][6]*actual[i][6])*.2f;
        assert(fabsf(radius-(i<2?.3f:.32f))<1e-6f);assert(fabsf(width-(i<2?.24f:.26f))<1e-6f);
        assert(actual[i][12]==brake[i][12]&&actual[i][13]==brake[i][13]&&actual[i][14]==brake[i][14]);
        assert(fabsf(sqrtf(brake[i][0]*brake[i][0]+brake[i][1]*brake[i][1])-1)<1e-6f);
    }
    assert(profile.wheel_r==.4f&&profile.wheel_w==.2f&&ride.compression[0]==.02f);
    /* AI body sits the source mount height below its rest hub, as the Stage5
       player body does; it must not take the tyre-mesh hub as its origin. */
    ai_wheel_matrices(&wheel,1,0,0,NULL,actual,brake);
    for(int i=0;i<4;i++)
        assert(fabsf(profile.ride+actual[i][14]-ai_body_ride(&profile,-.1f,&wheel,1)-.15f)<1e-6f);
    wheel=wheel_config_for("FITCAR",&profile,NULL,0,&source);assert(!source);
    assert(ai_body_ride(&profile,-.1f,&wheel,1)==body_ride_height(.4f,-.1f,0));
    ai_wheel_matrices(&wheel,1,0,0,NULL,actual,brake);assert(actual[0][0]==1&&actual[0][5]==1&&actual[0][10]==1);
    assert(g_dbg.body_drop==0);
    publish_performance_source(NULL,NULL,0,"missing");assert(!g_dbg.perf_source_available&&!g_dbg.perf_stock_available);
    puts("source wheel visual fit: per-wheel diameter/width, unchanged hubs/brakes/physics, missing-source fallback and zero default lowering PASS");
}
int main(int argc,char **argv) {
    test_source_wheel_visual_fit();
    RhiTexture roaming_tex={new_handle(2)}, player_tex={new_handle(2)};
    RhiTexture texmap[]={roaming_tex,player_tex};
    car_texture_map_clear(texmap,2,&roaming_tex,1);
    assert(handles[roaming_tex.id] && !handles[player_tex.id] && live_textures==1);
    car_texture_map_clear(&roaming_tex,1,NULL,0);
    assert(!live_textures);
    /* 2 cm of body-shell clearance is kept; the drop is clamped to what the
       model allows and never lifts the car. */
    assert(fabsf(body_ride_height(.35f,-.20f,.06f)-.29f)<1e-6f);   /* room to spare */
    assert(fabsf(body_ride_height(.35f,-.29f,.06f)-.31f)<1e-6f);   /* clamped to 4 cm */
    assert(fabsf(body_ride_height(.35f,-.33f,.06f)-.35f)<1e-6f);   /* already on the deck */
    assert(fabsf(body_ride_height(.35f,-.20f,-.1f)-.35f)<1e-6f);   /* negative drop ignored */
    assert(body_ride_height(.05f,.20f,.12f)>=.05f);
    const unsigned char archive[] = {0};
    N2Scene scene = {0}; GpuMesh *gm = NULL;
    int n = 0, mode = -1; RhiTexture tex = {0};
    assert(load_rim_style(archive, 1, NULL, 0, 1, &scene, &gm, &n,
                          archive, 1, &tex, &mode, .35f));
    for (int round = 0; round < 20; round++) {
        N2Scene before = scene; GpuMesh *old_gm = gm; RhiTexture old_tex = tex;
        int old_mode = mode;
        float vertices[20]; memcpy(vertices, scene.meshes[0].verts, sizeof vertices);
        for (test_stage = 1; test_stage <= 10; test_stage++) {
            assert(!load_rim_style(archive, 1, NULL, 0, 2, &scene, &gm, &n,
                                   archive, 1, &tex, &mode, .35f));
            assert(!memcmp(&scene, &before, sizeof scene));
            assert(!memcmp(vertices, scene.meshes[0].verts, sizeof vertices));
            assert(gm == old_gm && tex.id == old_tex.id && mode == old_mode && n == 2);
            assert(live_buffers == 6 && live_textures == 1);
        }
        assert(!load_rim_style(NULL, 0, NULL, 0, 2, &scene, &gm, &n,
                               archive, 1, &tex, &mode, .35f));
        assert(!load_rim_style(archive, 1, NULL, 0, 2, &scene, &gm, &n,
                               NULL, 0, &tex, &mode, .35f));
        test_stage = 0;
        assert(load_rim_style(archive, 1, NULL, 0, round % 2 + 1, &scene, &gm, &n,
                              archive, 1, &tex, &mode, .35f));
        assert(tex.id != old_tex.id && handles[old_tex.id] == 0);
        assert(live_buffers == 6 && live_textures == 1);
    }
    test_free_scene_gpu(gm, n); n2_free_scene(&scene); test_destroy_texture(tex);
    assert(!live_buffers && !live_textures);
    puts("wheel_reload_test: PASS (rollback at 10 boundaries; 20 repeated swaps)");
    test_body_kit=1;
    N2Mesh original={.famkey=42,.vkind=0,.texkey=7};
    N2Scene active={.meshes=&original,.count=1};
    N2CarConfig config={.body_kit=1};
    for(int round=0;round<20;round++) {
        for(test_stage=1;test_stage<=10;test_stage++) {
            BodyKitCandidate candidate={0};
            assert(!prepare_body_kit(&candidate,archive,1,NULL,0,&config,
                                     archive,1,&active,NULL,0,NULL));
            assert(!candidate.scene.meshes && !candidate.gpu && !candidate.ntextures);
            assert(!live_buffers && !live_textures && original.texkey==7);
        }
        test_stage=0;
        BodyKitCandidate candidate={0};
        assert(prepare_body_kit(&candidate,archive,1,NULL,0,&config,
                                archive,1,&active,NULL,0,NULL));
        assert(candidate.scene.count==2 && candidate.ntextures==1);
        assert(candidate.bb[0]==-1 && candidate.bb[3]==1 && candidate.bb[1]==-.5f && candidate.bb[4]==.5f);
        assert(live_buffers==6 && live_textures==1);
        body_kit_release(&candidate);
        uint32_t cached=8;
        assert(prepare_body_kit(&candidate,archive,1,NULL,0,&config,
                                archive,1,&active,&cached,1,NULL));
        assert(candidate.ntextures==0 && live_textures==0); /* preserves painted/vinyl texture */
        body_kit_release(&candidate);
        assert(!live_buffers && !live_textures);
    }
    puts("body_kit_reload_test: PASS (rollback at 10 boundaries; 20 repeated preparations; cached textures retained)");
    test_vinyl_selection();
    if(argc>2 && !strcmp(argv[2],"--vinyls")) {
        test_vinyl_archives(argv[1]);
    } else if(argc>2 && !strcmp(argv[2],"--car-switches")) {
        test_car_switch_archive(argv[1]);
    } else if(argc>2 && (!strcmp(argv[2],"--all-cars") || !strcmp(argv[2],"--all-parts"))) {
        char cars[64][64];int selected=0;
        int count=res_list_cars(argv[1],cars,64,"",&selected);assert(count>0);
        for(int i=0;i<count;i++) {
            next_handle=1;
            if(!strcmp(argv[2],"--all-parts"))test_parts_archive(argv[1],cars[i]);
            else test_kit_archive(argv[1],cars[i]);
        }
        printf("CENSUS PASS: all %d catalog cars\n",count);
    } else if(argc>1)test_kit_archive(argv[1],argc>2?argv[2]:"MIATA");
    return 0;
}
