/* Asset-free compiled collision corrections: unchanged draw/ground payload,
   all material slices, exact overrides and forward/reverse/airborne contacts. */
#include <assert.h>
#include "world.h"
#include "world_collision_rules.h"

static N2Mesh mesh(uint32_t key,uint64_t id,float x,int ground) {
    float verts[]={x,-4,0,0,0,x,4,0,0,0,x,4,1,0,0,x,-4,1,0,0,
                   x-5,-5,0,0,0,x+5,-5,0,0,0,x+5,5,0,0,0,x-5,5,0,0,0};
    uint16_t idx[]={0,1,2,0,2,3,4,5,6,4,6,7};
    N2Mesh m={0};m.verts=malloc(sizeof verts);m.idx=malloc(sizeof idx);assert(m.verts&&m.idx);
    memcpy(m.verts,verts,sizeof verts);memcpy(m.idx,idx,sizeof idx);
    m.nverts=8;m.nidx=ground?12:6;m.cat=ground?N2_TERRAIN:N2_OTHER;
    m.scen=ground?N2_SC_TERRAIN:N2_SC_WALL;m.world_model_key=key;m.placement_id=id;
    strcpy(m.sname,"TRUNCATED_SHARED_NAME");return m;
}
static int contact(const N2Mesh *m,float x,float z) {
    N2Scene s={(N2Mesh*)m,1,1};float pos[]={x,0,z},vel[]={.1f,0};
    float bb[]={-.8f,-.4f,0,.8f,.4f,1.4f};
    return collide_body_mesh_wall(pos,vel,0,bb,z+.05f,z+1.4f,&s,0,.30f,INFINITY,NULL);
}
static void roadside(void) {
    const char *names[]={"TRN_TEST_ROAD","TRN_TEST_TERRAIN","XB_TEST_BUILDING",
        "XO_TEST_TREE","XO_TEST_CONTAINER","PAN_OCEAN","TRN_TEST_WATERFALL",
        "XW_TEST_GUARDRAIL","XO_TRACKBARRIER_PLAYER_00"};
    N2Mesh meshes[9];N2Scene s={meshes,9,9};
    uint64_t ids[9];float verts[9][40];uint16_t indices[9][12];
    static const float boundary[][3]={{140,-4,0},{140,4,0}};
    const WCollisionRule rule={1,108,8,2,boundary},guard={0,107,8,0,NULL};
    const WCollisionEdits edits={"TEST",1,&guard};
    for(int n=0;n<9;n++) {
        meshes[n]=mesh(100+n,100+n,20*n,1);
        strcpy(meshes[n].sname,names[n]);meshes[n].scen=n2_scen_class(names[n]);
        if(n==0)meshes[n].cat=N2_ROAD;
        if(n==2 || n==3 || n==4 || n==7)meshes[n].cat=N2_OTHER;
        if(n==7)meshes[n].cat=N2_ROAD; /* barrier material is road, geometry is scenery */
        if(n==8)meshes[n].cat=N2_OTHER;
        ids[n]=n2_world_source_id(meshes+n);
        memcpy(verts[n],meshes[n].verts,sizeof verts[n]);
        memcpy(indices[n],meshes[n].idx,sizeof indices[n]);
        assert(contact(meshes+n,20*n+.1f,0));
    }
    meshes[7].wall_policy=2;meshes[7].wall_height=8;
    assert(wce_wall(meshes+7,&rule,0) && contact(meshes+7,140.1f,6));
    assert(wce_roadside(&edits,&s,0) && wce_roadside(&edits,&s,0) && s.count==9);
    for(int n=0;n<9;n++) {
        N2Mesh *m=meshes+n;float x=20*n;
        int wall=n<2 || n==7;
        assert(m->wall_policy==(n<2?0:n==7?2:1));
        assert(m->wall_height==(n==7?8:0) && n2_world_body_disabled(m)==!wall);
        assert((m->wall_nidx>0)==(n==7));
        assert(ids[n]==n2_world_source_id(m) && !memcmp(verts[n],m->verts,sizeof verts[n]));
        assert(!memcmp(indices[n],m->idx,sizeof indices[n]));
        assert(contact(m,x+.1f,0)==wall && contact(m,x-.1f,0)==wall);
        assert(contact(m,x+.1f,6)==(n==7) && !contact(m,x+.1f,9));
        float face[9];assert(phys_wall_debug_face(m,0,.3f,face)==wall);
        N2Scene one={m,1,1};float anchor[]={x-2,0,.5f},eye[]={x+2,0,.5f};
        assert((world_camera_clip(&one,NULL,anchor,eye,.1f)<1)==wall);
        float ground=99;int expected=n<2?(n==0?WSURF_ROAD:WSURF_TERRAIN):WSURF_NONE;
        assert(world_ground_at(&one,x-1,0,1,&ground)==expected);
        WGroundHit hit;int verdict;
        assert(world_wheel_support(&one,x-1,0,0,.25f,.18f,&hit,NULL,&verdict)==expected);
        assert(verdict==(n<2?WWS_CONTACT:WWS_NOCOVER));
        float from[]={x-1,0,1},to[]={x-1,0,-1};
        assert((world_ground_sweep(&one,from,to,NULL)<1)==(n<2));
        assert((world_camera_clip(&one,NULL,from,to,.1f)<1)==(n<2));
        if(n<2)assert(fabsf(ground)<1e-6f);
        else assert(n2_ground_z(&one,x-1,0,99)==99);
    }
    float obst[9][4],z[9][2];int src[9];int count=phys_collect_walls(&s,obst,src,z,9);assert(count==1);
    const float bb[]={-.8f,-.4f,0,.8f,.4f,1.4f};
    for(int side=-1;side<=1;side+=2) {
        float old[]={140+4*side,0,6},pos[]={140-4*side,0,6},vel[]={-side*2,0};
        assert(world_body_walls_move(&s,old,pos,vel,0,bb,6,7.4f,-.5f,
            obst,z,count,src,NULL,0,NULL,NULL)>0);
        assert(side*(pos[0]-140)>.79f);
        float anchor[]={140+2*side,0,6},eye[]={140-2*side,0,6};
        N2Scene one={meshes+7,1,1};assert(world_camera_clip(&one,NULL,anchor,eye,.1f)<1);
    }
    for(int n=0;n<9;n++){free(meshes[n].verts);free(meshes[n].idx);
        free(meshes[n].wall_verts);free(meshes[n].wall_idx);}
}
static void boundary_outline(void) {
    float verts[40]={-6,-6,0,0,0, -6,6,0,0,0, -6,6,.8f,0,0, -6,-6,.8f,0,0,
                     -6,-6,0,0,0, 6,-6,0,0,0, 6,-6,.8f,0,0, -6,-6,.8f,0,0};
    uint16_t left[]={0,1,2,0,2,3},bottom[]={4,5,6,4,6,7};
    N2Mesh m[2]={{.verts=verts,.nverts=8,.idx=left,.nidx=6,.scen=N2_SC_WALL,
                  .cat=N2_OTHER,.placement_id=900,.world_model_key=902},
                 {.verts=verts,.nverts=8,.idx=bottom,.nidx=6,.scen=N2_SC_WALL,
                  .cat=N2_OTHER,.placement_id=900,.world_model_key=902}};
    strcpy(m[0].sname,"XW_CURVED_WALL");strcpy(m[1].sname,m[0].sname);
    const WCollisionRule guard={0,902,8,0,NULL};const WCollisionEdits edits={"TEST",1,&guard};
    N2Scene scene={m,2,2};assert(wce_roadside(&edits,&scene,0));
    assert(m[0].wall_verts && !m[1].wall_verts);
    assert(!cw_probe_contact(&scene,0,0,0,1,.1f,1.5f)); /* do not close the opening */
    assert(cw_probe_contact(&scene,0,-5.9f,0,1,6,7.4f));
    assert(cw_probe_contact(&scene,0,0,-5.9f,1,6,7.4f)); /* second material owns this face */
    for(int n=0;n<2;n++){free(m[n].wall_verts);free(m[n].wall_idx);}
}
static void planter_materials(void) {
    float verts[60]={-1,-1,0,0,0, -1,1,0,0,0, -1,1,.5f,0,0, -1,-1,.5f,0,0,
                     1,-1,0,0,0, 1,1,0,0,0, 1,1,.5f,0,0, 1,-1,.5f,0,0,
                     -10,-10,6,0,0, 10,-10,6,0,0, 10,-10,9,0,0, -10,-10,9,0,0};
    uint16_t leaves[]={8,9,10,8,10,11},base[]={0,1,2,0,2,3,4,5,6,4,6,7};
    N2Mesh m[2]={{.verts=verts,.nverts=12,.idx=leaves,.nidx=6,.scen=N2_SC_TREE,
                  .cat=N2_OTHER,.placement_id=901},
                 {.verts=verts,.nverts=12,.idx=base,.nidx=12,.scen=N2_SC_TREE,
                  .cat=N2_OTHER,.placement_id=901}};
    strcpy(m[0].sname,"XT_PLANTERPALMTREE");strcpy(m[1].sname,m[0].sname);
    N2Scene scene={m,2,2};assert(wce_roadside(NULL,&scene,0));
    assert(m[0].wall_nidx==24 && !m[1].wall_verts);
    assert(cw_probe_contact(&scene,0,-.9f,0,.5f,.1f,1.5f));
    assert(cw_probe_contact(&scene,0,.9f,0,.5f,.1f,1.5f));
    assert(!cw_probe_contact(&scene,0,0,-9.9f,.5f,6,7.4f));
    for(int n=0;n<2;n++){free(m[n].wall_verts);free(m[n].wall_idx);}
}
static void sloped_boundary(void) {
    float verts[20]={0,-6,0,0,0, 0,6,3,0,0, 0,6,3.8f,0,0, 0,-6,.8f,0,0};
    uint16_t idx[]={0,1,2,0,2,3};
    N2Mesh m={.verts=verts,.nverts=4,.idx=idx,.nidx=6,.cat=N2_OTHER,.scen=N2_SC_WALL};
    strcpy(m.sname,"XW_GREENBARRIER_TEST");N2Scene scene={&m,1,1};
    assert(wce_roadside(NULL,&scene,0) && m.wall_nidx==12);
    assert(cw_probe_contact(&scene,0,.1f,5,.5f,3.2f,4.6f));
    float anchor[]={-2,5,3.2f},eye[]={2,5,3.2f};
    assert(world_camera_clip(&scene,NULL,anchor,eye,.1f)<1);
    free(m.wall_verts);free(m.wall_idx);
}
static void compiled_rules(void) {
    const WCollisionEdits *e=&city_collision_edits;
    assert(wce_map(e,"STREAML4RA")&&wce_map(e,"STREAML4RA.BUN")&&!wce_map(e,"STREAML4RB"));
    for (int n=0;n<e->count;n++) {
        const WCollisionRule *r=e->rules+n;
        assert(r->kind>=0&&r->kind<=2&&isfinite(r->height)&&r->height>=0&&r->height<=1000);
        assert(r->kind==2 ? !r->key : r->key!=0);
        if (r->kind==0) assert(r->key<=UINT32_MAX&&!r->count&&!r->points);
        else assert(r->height>0&&r->count>=2&&r->count<=16383&&r->points);
        for (int k=0;k<r->count;k++) for (int a=0;a<3;a++)
            assert(isfinite(r->points[k][a])&&fabsf(r->points[k][a])<100000);
        for (int k=0;k<n;k++) assert(r->kind==2||r->kind!=e->rules[k].kind||r->key!=e->rules[k].key);
    }
}
static void buildings(void) {
    const char *names[]={"XB_MODERNHOTELBASE_1A_00","XB_MODERNHOTELBASE_1A_00",
        "XO_IndustrialOfficesA_1a_CK","XO_INDUSTRIALSHEDB_1A_CK_00",
        "XO_GasstationTN_1a_00","XS_HOTELB_1A_00","XW_GUARDRAIL_A_1A_FWY_00"};
    N2Mesh meshes[8];N2Scene s={meshes,8,8};
    for(int n=0;n<7;n++) {
        meshes[n]=mesh(11,n<2?202:300+n,10*n,0);
        meshes[n].scen=n2_scen_class(names[n]);
        strcpy(meshes[n].sname,names[n]);
        assert(contact(meshes+n,10*n+.1f,0));
    }
    meshes[7]=mesh(20,400,80,1);
    static const float boundary[][3]={{0,-4,0},{0,4,0}};
    const WCollisionRule rules[]={{0,11,8,0,NULL},{1,202,8,2,boundary}};
    const WCollisionEdits edits={"TEST",2,rules};
    assert(wce_apply(&edits,&s,1));
    for(int n=0;n<5;n++) {
        assert(meshes[n].scen==N2_SC_BUILDING && meshes[n].wall_policy==1);
        assert(!meshes[n].wall_verts && !contact(meshes+n,10*n+.1f,0));
        float face[9];assert(!phys_wall_debug_face(meshes+n,0,.3f,face));
        N2Scene one={meshes+n,1,1};float anchor[]={10*n-2,0,.5f},eye[]={10*n+2,0,.5f};
        assert(world_camera_clip(&one,NULL,anchor,eye,.1f)==1);
    }
    assert(meshes[5].scen==N2_SC_STRUCT && meshes[6].scen==N2_SC_WALL);
    assert(contact(meshes+5,50.1f,6) && contact(meshes+6,60.1f,6));
    /* The same building policy applies without a matching map-specific export. */
    meshes[0].wall_policy=2;meshes[0].wall_height=8;
    assert(wce_wall(meshes,rules+1,0) && wce_apply(NULL,&s,0));
    assert(meshes[0].wall_policy==1 && !meshes[0].wall_verts && !contact(meshes,.1f,0));
    float obst[8][4],z[8][2];int src[8];assert(phys_collect_walls(&s,obst,src,z,8)==2);
    float ground=99;N2Scene terrain={meshes+7,1,1};
    assert(world_ground_at(&terrain,79,0,1,&ground)!=WSURF_NONE && fabsf(ground)<1e-6f);
    for(int n=0;n<8;n++) {free(meshes[n].verts);free(meshes[n].idx);
        free(meshes[n].wall_verts);free(meshes[n].wall_idx);}
}
static void repeated_models(void) {
    static const float local[][3]={{16,-4,0},{16,4,0}};
    const WCollisionRule rules[]={{0,10,0,0,NULL},{0,11,8,0,NULL},{0,99,8,0,NULL},
        {1,202,8,2,local},{2,0,8,2,local}};
    const WCollisionEdits edits={"HOME",5,rules};
    N2Scene s={0};s.count=s.cap=6;s.meshes=calloc(6,sizeof *s.meshes);assert(s.meshes);
    s.meshes[0]=mesh(10,101,0,0);
    s.meshes[1]=mesh(91,102,10,1);s.meshes[1].world_base_model_key=10;
    s.meshes[2]=mesh(11,103,30,0);
    s.meshes[3]=mesh(93,202,50,0);s.meshes[3].world_base_model_key=11;
    s.meshes[4]=mesh(30,104,70,0); /* same name, unrelated model */
    s.meshes[5]=mesh(99,105,90,0);s.meshes[5].world_base_model_key=10;
    float verts[6][40];uint16_t indices[6][12];
    for(int n=0;n<6;n++) {memcpy(verts[n],s.meshes[n].verts,sizeof verts[n]);
        memcpy(indices[n],s.meshes[n].idx,sizeof indices[n]);}
    assert(!wce_map(&edits,"OTHER"));assert(wce_apply(&edits,&s,0) && s.count==6);
    for(int n=0;n<6;n++) {
        assert(!memcmp(verts[n],s.meshes[n].verts,sizeof verts[n]));
        assert(!memcmp(indices[n],s.meshes[n].idx,sizeof indices[n]));
    }
    assert(!contact(s.meshes,.1f,0) && !contact(s.meshes+1,10.1f,0));
    assert(contact(s.meshes+2,30.1f,6) && contact(s.meshes+3,50.1f,6));
    assert(!contact(s.meshes+3,16.1f,6) && !contact(s.meshes+3,50.1f,9));
    assert(s.meshes[4].wall_policy==0 && contact(s.meshes+4,70.1f,0));
    assert(contact(s.meshes+5,90.1f,6)); /* selected-model edit wins over its family */
    float ground=99;N2Scene terrain={s.meshes+1,1,1};
    assert(world_ground_at(&terrain,9,0,1,&ground)!=WSURF_NONE && fabsf(ground)<1e-6f);
    for(int n=0;n<s.count;n++) {free(s.meshes[n].verts);free(s.meshes[n].idx);
        free(s.meshes[n].wall_verts);free(s.meshes[n].wall_idx);}
    free(s.meshes);
}
int main(void) {
    roadside();boundary_outline();planter_materials();sloped_boundary();compiled_rules();buildings();repeated_models();
    static const float replacement[][3]={{16,-4,0},{16,4,0}},standalone[][3]={{40,-4,0},{40,4,0}};
    static const WCollisionRule rules[]={
        {0,10,0,0,NULL},{0,11,8,0,NULL},{1,202,8,2,replacement},{2,0,8,2,standalone}
    };
    const WCollisionEdits e={"TEST",4,rules};
    N2Scene s={0};s.count=s.cap=6;s.meshes=calloc(6,sizeof *s.meshes);assert(s.meshes);
    s.meshes[0]=mesh(10,101,0,1);s.meshes[1]=mesh(10,102,4,1);
    s.meshes[2]=mesh(20,103,6,0); /* same name, different model: keep */
    s.meshes[3]=mesh(11,202,12,0);s.meshes[4]=mesh(11,202,12,0);
    s.meshes[5]=mesh(11,203,30,0);
    uint64_t ids[6];float saved[6][40];uint16_t indices[6][12];
    for (int n=0;n<6;n++) {ids[n]=n2_world_source_id(s.meshes+n);memcpy(saved[n],s.meshes[n].verts,sizeof saved[n]);
        memcpy(indices[n],s.meshes[n].idx,sizeof indices[n]);}
    assert(contact(s.meshes,.1f,0));assert(wce_apply(&e,&s,1)&&s.count==7);
    for (int n=0;n<6;n++) {
        assert(ids[n]==n2_world_source_id(s.meshes+n));
        assert(!memcmp(saved[n],s.meshes[n].verts,sizeof saved[n]));
        assert(!memcmp(indices[n],s.meshes[n].idx,sizeof indices[n]));
    }
    assert(!contact(s.meshes,.1f,0)&&!contact(s.meshes+1,4.1f,0));
    assert(contact(s.meshes+2,6.1f,0));assert(!contact(s.meshes+3,12.1f,0));
    assert(contact(s.meshes+3,16.1f,6)&&!contact(s.meshes+4,16.1f,6));
    assert(contact(s.meshes+5,30.1f,6)&&!contact(s.meshes+5,30.1f,9));
    assert(contact(s.meshes+6,40.1f,6)&&s.meshes[6].cat==N2_COLLISION);
    N2Scene invisible={s.meshes+6,1,1};float bounds[][4]={{40,-4,40,4}};RhiTexture textures[]={{0}};
    WorldBatchUpload *upload=upload_world_batches_begin(&invisible,bounds,textures,(RhiTexture){0},NULL,NULL);
    N2Batch *batches=NULL;int nbatch=0;assert(upload);
    assert(upload_world_batches_step(&upload,1,&batches,&nbatch,NULL)==1&&!upload&&nbatch==0);
    free(batches);
    float faces[9];assert(!phys_wall_debug_face(s.meshes,0,.3f,faces));
    float ground=99;N2Scene terrain={s.meshes,2,2};
    assert(world_ground_at(&terrain,-1,0,1,&ground)!=WSURF_NONE&&fabsf(ground)<1e-6f);
    float from[]={-1,0,1},to[]={-1,0,-1};assert(world_ground_sweep(&terrain,from,to,NULL)<1);
    float anchor[]={-2,0,.5f},eye[]={2,0,.5f};N2Scene one={s.meshes,1,1};
    assert(world_camera_clip(&one,NULL,anchor,eye,.1f)==1);
    one.meshes=s.meshes+3;anchor[0]=14;eye[0]=18;anchor[2]=eye[2]=6;
    assert(world_camera_clip(&one,NULL,anchor,eye,.1f)<1);
    float obst[8][4],z[8][2];int src[8],count=phys_collect_walls(&s,obst,src,z,8);
    assert(count==4);
    const float bb[]={-.8f,-.4f,0,.8f,.4f,1.4f};
    for (int side=-1;side<=1;side+=2) {
        float old[]={40+4*side,0,6},pos[]={40-4*side,0,6},vel[]={-side*2,0};
        assert(world_body_walls_move(&s,old,pos,vel,0,bb,6,7.4f,-.5f,
            obst,z,count,src,NULL,0,NULL,NULL)>0);
        assert(side*(pos[0]-40)>.79f);
    }
    for (int n=0;n<s.count;n++) {free(s.meshes[n].verts);free(s.meshes[n].idx);
        free(s.meshes[n].wall_verts);free(s.meshes[n].wall_idx);}
    free(s.meshes);puts("world_collision_test: PASS");return 0;
}
