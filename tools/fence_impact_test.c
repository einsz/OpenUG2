/* One asset-free check of marked panels, ordinary walls, AI and live GPU ranges. */
#include "world.h"
#include "ai.h"
#include "world_scenery.h"
#include "world_collision.h"
#include "rhi_gl.h"
#include <assert.h>

static void group_role_test(void) {
    unsigned char ov[8]={1,0,2,0,0,0,1,0},g[56]={0};
    memcpy(g+8,"SMOKEABLE",9);uint32_t hash=0xffffffffu;
    for(const char *p="SMOKEABLE";*p;p++)hash=hash*33+(unsigned char)*p;
    memcpy(g+40,&hash,4);uint32_t one=1;memcpy(g+48,&one,4);
    WGTable table;assert(wg_open(ov,sizeof ov,g,sizeof g,&table));
    for(int event=-1;event<=0;event++) {
        WGSelection selection;assert(wg_selection_open(&table,event,&selection));
        const WGSelected *role=wg_selection_find(&selection,1,2);
        assert(role && role->membership&WG_KNOCKDOWN);
        assert(wg_selection_visible(&selection,1,2));free(selection.items);
    }
}

static void road_sign_impact_test(void) {
    float road[]={-30,-10,0,0,0,100,-10,0,1,0,100,10,0,1,1,-30,10,0,0,1};
    /* The real NoPark pole extends about .3 m into the authored pavement. */
    float pole[]={25,-.06f,-.32f,0,0,25,.06f,-.32f,1,0,25,.06f,3.68f,1,1,25,-.06f,3.68f,0,1};
    float original[20];memcpy(original,pole,sizeof pole);uint16_t idx[]={0,1,2,0,2,3};
    N2Mesh m[3]={{.verts=road,.idx=idx,.nverts=4,.nidx=6,.cat=N2_ROAD,.scen=N2_SC_TERRAIN},
        {.verts=pole,.idx=idx,.nverts=4,.nidx=6,.cat=N2_OTHER,.scen=N2_SC_STRUCT,.prop_id=99}};
    strcpy(m[1].sname,"XS_NOPARKANYTIME_1A_00");N2Scene scene={m,2,2};
    assert(wce_roadside(NULL,&scene,0));assert(!n2_world_body_disabled(m+1));
    float bounds[][4]={{-30,-10,100,10},{25,-.06f,25,.06f}};WGroundGrid grid={0};
    assert(world_ground_grid_build(&grid,&scene,bounds));world_ground_grid_activate(&grid);
    float walls[2][4],z[2][2];int src[2];int n=phys_collect_walls(&scene,walls,src,z,2);assert(n==1);
    phys_wall_index_build(walls,z,n);float bb[]={-2,-.85f,0,2,.85f,1.5f},p[]={23.1f,0,0},v[]={.02f,0};
    assert(collide_body_walls(p,v,0,bb,walls,z,n,.05f,1.5f,&scene,src,NULL,0));
    assert(!m[1].prop_broken && v[0]==0);
    p[0]=23.1f;v[0]=.2f;
    assert(collide_body_walls(p,v,0,bb,walls,z,n,.05f,1.5f,&scene,src,NULL,0));
    assert(m[1].prop_broken && v[0]>0 && v[0]<.2f);
    int saw_sparks=0;WorldPropSpark sparks[64];
    assert(world_prop_sparks(sparks,64)==0);
    for(int tick=0;tick<120;tick++) {
        world_props_step(1/60.f);world_prop_update_mesh(m+1);
        int count=world_prop_sparks(sparks,64);saw_sparks|=count>0;
        for(int k=0;k<count;k++)assert(sparks[k].alpha>0 && sparks[k].alpha<=1 && sparks[k].pos[2]>=0);
    }
    assert(saw_sparks);
    float moved=pole[0]-original[0];assert(moved>2 && m[1].prop_angle>1.56f);
    float settled[20];memcpy(settled,pole,sizeof pole);
    for(int tick=0;tick<180;tick++){world_props_step(1/60.f);world_prop_update_mesh(m+1);}
    assert(!memcmp(settled,pole,sizeof pole));assert(world_prop_sparks(sparks,64)==0);
    for(int k=0;k<4;k++){assert(fabsf(pole[k*5+2])<.01f);assert(!memcmp(pole+k*5+3,original+k*5+3,8));}
    /* Rebuild after streaming: apply the same retained rotation AND translation. */
    memcpy(pole,original,sizeof pole);m[1].prop_angle=0;m[1].prop_revision=0;
    memset(m[1].prop_offset,0,sizeof m[1].prop_offset);
    assert(world_prop_update_mesh(m+1));
    for(int k=0;k<20;k++)assert(fabsf(pole[k]-settled[k])<.001f);
    world_props_reset();
    memcpy(pole,original,sizeof pole);m[1].prop_angle=0;m[1].prop_revision=0;m[1].prop_broken=0;
    memset(m[1].prop_offset,0,sizeof m[1].prop_offset);
    float wall[]={30,-6,0,0,0,30,6,0,1,0,30,6,5,1,1,30,-6,5,0,1};
    m[2]=(N2Mesh){.verts=wall,.idx=idx,.nverts=4,.nidx=6,.cat=N2_OTHER,.scen=N2_SC_WALL};
    scene.count=scene.cap=3;n=phys_collect_walls(&scene,walls,src,z,2);assert(n==2);
    phys_wall_index_build(walls,z,n);assert(g_phys_prop_impact_hook(&scene,1,(float[]){12,0},-1,0)==1);
    for(int tick=0;tick<300;tick++) {
        world_props_step_collisions(1/60.f,&scene,walls,z,src,n);world_prop_update_mesh(m+1);
    }
    for(int k=0;k<4;k++)assert(pole[k*5]<30.01f);
    assert(pole[0]>original[0]);
    world_props_reset();memcpy(pole,original,sizeof pole);
    for(int k=0;k<4;k++)pole[k*5+2]+=3; /* NP sign panel detached above pavement. */
    m[1].prop_angle=0;m[1].prop_revision=0;m[1].prop_broken=0;
    memset(m[1].prop_offset,0,sizeof m[1].prop_offset);
    assert(g_phys_prop_impact_hook(&scene,1,(float[]){12,0},-1,0)==1);
    for(int tick=0;tick<300;tick++) {
        world_props_step_collisions(1/60.f,&scene,walls,z,src,n);world_prop_update_mesh(m+1);
    }
    for(int k=0;k<4;k++){assert(pole[k*5]<30.01f);assert(fabsf(pole[k*5+2])<.01f);}
    world_props_reset();phys_wall_index_build(NULL,NULL,0);world_ground_grid_activate(NULL);world_ground_grid_free(&grid);
}

static void road_sign_direction_test(void) {
    const float incoming[][2]={{12,0},{-12,0},{10,8},{-10,-8},{8,-10}};
    for(int hit=0;hit<5;hit++) {
        float road[]={-100,-100,0,0,0,100,-100,0,1,0,100,100,0,1,1,-100,100,0,0,1};
        float pole[]={25,-2,0,0,0,25,2,0,1,0,25,2,4,1,1,25,-2,4,0,1};
        uint16_t ix[]={0,1,2,0,2,3};
        N2Mesh m[2]={{.verts=road,.idx=ix,.nverts=4,.nidx=6,.cat=N2_ROAD},
            {.verts=pole,.idx=ix,.nverts=4,.nidx=6,.cat=N2_OTHER,.scen=N2_SC_STRUCT,.prop_id=51}};
        strcpy(m[1].sname,"XS_SPEED35_1A_00");N2Scene scene={m,2,2};
        float bounds[][4]={{-100,-100,100,100},{25,-2,25,2}};WGroundGrid grid={0};
        assert(world_ground_grid_build(&grid,&scene,bounds));world_ground_grid_activate(&grid);
        float walls[2][4],z[2][2];int src[2],n=phys_collect_walls(&scene,walls,src,z,2);
        phys_wall_index_build(walls,z,n);
        float vx=incoming[hit][0],vy=incoming[hit][1],side=vx>0?1:-1;
        float heading=atan2f(vy,vx),reach=fabsf(cosf(heading))*1.15f+.85f;
        float p[]={25-side*(reach-.03f),0,0},v[]={vx/PHYS_TICKRATE,vy/PHYS_TICKRATE};
        float bb[]={-2,-.85f,0,2,.85f,1.5f};
        assert(collide_body_walls(p,v,heading,bb,walls,z,n,.05f,1.5f,&scene,src,NULL,0));
        assert(m[1].prop_broken);
        for(int tick=0;tick<120;tick++){world_props_step(1/60.f);world_prop_update_mesh(m+1);}
        float dx=0,dy=0;
        for(int k=0;k<4;k++){dx+=pole[k*5]-25;dy+=pole[k*5+1];}
        dx*=.25f;dy*=.25f;float distance=hypotf(dx,dy);
        assert(distance>.5f && dx*vx+dy*vy>0);
        assert(fabsf(dx*vy-dy*vx)<distance*hypotf(vx,vy)*.001f);
        for(int k=0;k<4;k++)assert(fabsf(pole[k*5+2])<.01f);
        world_props_reset();phys_wall_index_build(NULL,NULL,0);
        world_ground_grid_activate(NULL);world_ground_grid_free(&grid);
    }
}

int main(int argc,char **argv) {
    road_sign_direction_test();road_sign_impact_test();group_role_test();int gpu=argc>1 && !strcmp(argv[1],"--gpu");
    SDL_Window *window=NULL;SDL_GLContext context=NULL;
    if(gpu) {
        assert(SDL_Init(SDL_INIT_VIDEO)==0);
        window=SDL_CreateWindow("Fence impact check",0,0,64,64,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);assert(window);
        context=SDL_GL_CreateContext(window);assert(context);
    }
    float road[]={-30,-10,0,0,0, 100,-10,0,1,0, 100,10,0,1,1, -30,10,0,0,1};
    float panel[]={25,-8,0,0,0, 25,8,0,1,0, 25,8,4,1,1, 25,-8,4,0,1};
    float original[20];memcpy(original,panel,sizeof panel);
    uint16_t triangles[]={0,1,2,0,2,3};unsigned char colors[16]={10,20,30,255,40,50,60,255,70,80,90,255,100,110,120,255};
    N2Mesh meshes[2]={{.verts=road,.nverts=4,.idx=triangles,.nidx=6,.cat=N2_ROAD,.scen=N2_SC_TERRAIN},
        {.verts=panel,.nverts=4,.idx=triangles,.nidx=6,.cat=N2_OTHER,.scen=N2_SC_WALL,.vcol=colors,
         .prop_id=0x4152344c00010002ULL}};
    N2Scene scene={meshes,2,2};float bounds[][4]={{-30,-10,100,10},{25,-8,25,8}};
    WGroundGrid grid={0};assert(world_ground_grid_build(&grid,&scene,bounds));world_ground_grid_activate(&grid);
    float walls[2][4],heights[2][2];int sources[2],n=phys_collect_walls(&scene,walls,sources,heights,2);assert(n==1);
    phys_wall_index_build(walls,heights,n);
    float body[]={-2,-.85f,0,2,.85f,1.5f},pose[]={23.1f,0,0},vel[]={.02f,0};
    /* Planning is read-only. Low-speed contact still stops against the panel. */
    assert(!collide_body_walls_preview(pose,vel,0,body,walls,heights,n,.05f,1.5f,&scene,sources));
    assert(!meshes[1].prop_broken && !memcmp(panel,original,sizeof panel));
    assert(collide_body_walls(pose,vel,0,body,walls,heights,n,.05f,1.5f,&scene,sources,NULL,0));
    assert(!meshes[1].prop_broken && vel[0]==0);
    /* Even the invalid-body fallback cannot knock anything down while planning. */
    float invalid_body[6]={0},preview_pose[]={24,0,0},preview_vel[]={.2f,0};
    assert(!collide_body_walls_preview(preview_pose,preview_vel,0,invalid_body,walls,heights,n,.05f,1.5f,&scene,sources));
    assert(!meshes[1].prop_broken && preview_vel[0]==.2f);
    /* Same geometry without source membership remains an ordinary wall. */
    uint64_t identity=meshes[1].prop_id;meshes[1].prop_id=0;
    N2Mesh copy=meshes[1];copy.prop_id=identity;
    assert(!n2_mesh_same_content(&meshes[1],&copy));
    pose[0]=23.1f;vel[0]=.02f;
    assert(collide_body_walls_preview(pose,vel,0,body,walls,heights,n,.05f,1.5f,&scene,sources));
    meshes[1].prop_id=identity;
    /* A glancing corner hit must still lay this flat panel on the ground. */
    assert(g_phys_prop_impact_hook(&scene,1,(float[]){3,-9.539392f},-.3f,.9539392f)==1);
    world_props_step(.3f);assert(world_prop_update_mesh(meshes+1));
    for(int v=0;v<4;v++)assert(fabsf(panel[5*v+2])<.001f);
    world_props_reset();memcpy(panel,original,sizeof panel);
    meshes[1].prop_angle=0;meshes[1].prop_revision=0;meshes[1].prop_broken=0;
    N2Batch *batches=NULL;int count=0,map[2];BatchedVertex before[8],after[8];
    int panel_batch=-1,other_batch=-1;
    if(gpu) {
        RhiTexture textures[2]={{0}};count=upload_world_batches(&scene,bounds,textures,(RhiTexture){0},&batches,NULL,map,NULL);assert(count==2);
        /* upload returns count; the scene deliberately separates ROAD and OTHER. */
        assert(batches);panel_batch=map[1];other_batch=map[0];
        assert(panel_batch!=other_batch && batches[panel_batch].nprops==1);
        glBindBuffer(GL_ARRAY_BUFFER,rhi_gl_buffer_name(batches[panel_batch].vbo));glGetBufferSubData(GL_ARRAY_BUFFER,0,sizeof before[0]*4,before);
    }
    /* A normal physics-driven racer approaches, strikes and finishes. */
    float xy[]={0,0,20,0,40,0,60,0,80,0};N2Path path={xy,5};
    AiCar cars[N_RACE_AI]={{.pos={0,0,0},.half_length=2,.half_width=.85f,.height=1.5f}};
    AiRace drivers[N_RACE_AI]={0};
    drivers[0].progress=(WRace){.active=1,.kind=N2_RACE_SPRINT,.maxlaps=1,.ngate=3,
        .gate={{.x=0,.dx=1,.half=10},{.x=40,.dx=1,.half=10},{.x=70,.dx=1,.half=10}}};
    assert(world_race_bind_course(&drivers[0].progress,&path,12));
    assert(ai_drive_init(&drivers[0].drive,&path,cars[0].pos,0));
    AiTrafficWorld world={.scene=&scene,.obst=walls,.obstz=heights,.obstsrc=sources,.nobst=n};
    for(int frame=0;frame<2400 && !drivers[0].progress.finished;frame++) {
        ai_race_step(cars,drivers,0,1,0,&world,NULL);world_props_step(1/PHYS_TICKRATE);
        if(gpu)assert(render_world_prop_updates(&scene,batches,count,world_prop_update_mesh)>=0);
        else world_prop_update_mesh(meshes+1);
        assert(!drivers[0].drive.failed);
    }
    assert(drivers[0].progress.finished && cars[0].ride.contact_mask==15);
    assert(meshes[1].prop_broken && meshes[1].prop_angle>1.56f);
    assert(memcmp(panel,original,sizeof panel));
    for(int v=0;v<4;v++){assert(fabsf(panel[5*v+2])<.001f);assert(!memcmp(panel+5*v+3,original+5*v+3,8));}
    if(gpu) {
        assert(!render_world_prop_updates(&scene,batches,count,world_prop_update_mesh));
        glBindBuffer(GL_ARRAY_BUFFER,rhi_gl_buffer_name(batches[panel_batch].vbo));glGetBufferSubData(GL_ARRAY_BUFFER,0,sizeof after[0]*4,after);
        for(int v=0;v<4;v++) {
            assert(!memcmp(after[v].pos,panel+5*v,12));assert(!memcmp(after[v].uv,before[v].uv,8));
            assert(!memcmp(after[v].col,before[v].col,4));assert(fabsf(after[v].normal[2])>.99f);
        }
        glBindBuffer(GL_ARRAY_BUFFER,rhi_gl_buffer_name(batches[other_batch].vbo));glGetBufferSubData(GL_ARRAY_BUFFER,0,sizeof after[0]*4,after);
        for(int v=0;v<4;v++)assert(!memcmp(after[v].pos,road+5*v,12));
        assert(glGetError()==GL_NO_ERROR);
    }
    /* A reloaded copy keeps the same fall; another source identity does not. */
    memcpy(panel,original,sizeof panel);meshes[1].prop_angle=0;meshes[1].prop_revision=0;meshes[1].prop_broken=0;
    /* Freeing a failed map candidate must not reset the current world's props. */
    WorldCity candidate={0};world_city_free(&candidate);
    pose[0]=23.1f;vel[0]=.02f;
    assert(!collide_body_walls(pose,vel,0,body,walls,heights,n,.05f,1.5f,&scene,sources,NULL,0));
    assert(vel[0]==.02f && meshes[1].prop_broken);
    if(gpu) {
        render_batch_array_free(&batches,&count);
        RhiTexture textures[2]={{0}};count=upload_world_batches(&scene,bounds,textures,(RhiTexture){0},&batches,NULL,map,NULL);assert(count==2);
        assert(render_world_prop_updates(&scene,batches,count,world_prop_update_mesh)>0);
        glBindBuffer(GL_ARRAY_BUFFER,rhi_gl_buffer_name(batches[map[1]].vbo));glGetBufferSubData(GL_ARRAY_BUFFER,0,sizeof after[0]*4,after);
        for(int v=0;v<4;v++)assert(!memcmp(after[v].pos,panel+5*v,12));
        render_batch_array_free(&batches,&count);
    } else assert(world_prop_update_mesh(meshes+1));
    assert(meshes[1].prop_broken && meshes[1].prop_angle>1.56f);
    meshes[1].prop_id++;meshes[1].prop_broken=0;meshes[1].prop_revision=0;meshes[1].prop_angle=0;
    memcpy(panel,original,sizeof panel);assert(!world_prop_update_mesh(meshes+1));assert(!meshes[1].prop_broken);
    world_props_reset();meshes[1].prop_id=identity;
    assert(!world_prop_update_mesh(meshes+1));assert(!meshes[1].prop_broken);
    phys_wall_index_build(NULL,NULL,0);world_ground_grid_free(&grid);
    if(gpu){SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();}
    puts("fence_impact_test: PASS (source role, solid/slow contacts, AI finish, visible fall, reload/reset, optional GPU)");
}
