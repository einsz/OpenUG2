/* Exact declarative data identities, reviewed pause BINDING 1-4. */
#ifndef OPENUG2_PAUSE_MENU_BINDING_H
#define OPENUG2_PAUSE_MENU_BINDING_H
#include "ugui/ugui_scene.h"
enum { PAUSE_ROLE_RIGHT,PAUSE_ROLE_LEFT,PAUSE_ROLE_MARKER,PAUSE_ROLE_RAIL,PAUSE_ROLE_SLOT,PAUSE_ROLE_PRESENTATION,PAUSE_ROLE_HEADING,PAUSE_ROLE_SELECTOR,PAUSE_ROLE_CAPTION_SURFACE,PAUSE_ROLE_CAPTION,PAUSE_ROLE_BACKING,PAUSE_ROLE_GLOW,PAUSE_ROLE_OUTER_FILL,PAUSE_ROLE_OUTER_BORDER,PAUSE_ROLE_COUNT };
#define PAUSE_POOL_COUNT 10
typedef struct { uint32_t hash,guid,parent; int type,resource; } PauseIdentity;
static const PauseIdentity pause_identities[PAUSE_ROLE_COUNT]={
 {0x444969FEu,0x000DE087u,0x00000000u,1,2},
 {0x444969FDu,0x000DE06Du,0x00000000u,1,2},
 {0x3FBB7A4Eu,0x000DDF2Bu,0x00000000u,1,3},
 {0x688A1551u,0x000DDF2Au,0x00000000u,1,4},
 {0xB04669E3u,0x000DB84Fu,0x00000000u,1,5},
 {0x8D7E0C53u,0x000DDF11u,0x00000000u,5,5},
 {0x42ADB44Cu,0x000DDF12u,0x000DDF11u,2,1},
 {0x371F1C93u,0x000DDF1Du,0x000DDF11u,5,5},
 {0xDE2F4E52u,0x000DDF25u,0x000DDF11u,5,5},
 {0x5E7B09C9u,0x000DDF29u,0x000DDF11u,2,1},
 {0x687FDA31u,0x000DE222u,0x000DDF11u,1,8},
 {0xDF422345u,0x000DF02Eu,0x000DDF11u,1,9},
 {0x0DE769B5u,0x000E2709u,0x000DDF11u,5,5},
 {0x52BDEE1Du,0x000E4FE3u,0x000DDF11u,5,5},
};
static const PauseIdentity pause_pool_identities[PAUSE_POOL_COUNT]={
 {0x4B4E3888u,0x000DB813u,0,1,0},
 {0x4B4E3889u,0x000E2524u,0,1,0},
 {0x4B4E388Au,0x000E2529u,0,1,0},
 {0x4B4E388Bu,0x000E252Eu,0,1,0},
 {0x4B4E388Cu,0x000E2533u,0,1,0},
 {0x4B4E388Du,0x000E2538u,0,1,0},
 {0x4B4E388Eu,0x000E253Du,0,1,0},
 {0x4B4E388Fu,0x000E2542u,0,1,0},
 {0x4B4E3890u,0x000E2547u,0,1,0},
 {0xB51549B8u,0x000E254Cu,0,1,0},
};
/* Reject reused identifiers or mismatching metadata instead of picking the first object. */
static int pause_identity_find(const UgScene *s,const PauseIdentity *key) {
    int found=-1;
    for(int i=0;i<s->n;i++) {
        const UgFngObject *o=s->nodes[i].obj;
        if(o->guid!=key->guid && o->name_hash!=key->hash)continue;
        if(found>=0 || o->guid!=key->guid || o->name_hash!=key->hash ||
           o->parent_guid!=key->parent || o->type!=(uint32_t)key->type || o->res_index!=key->resource)return -1;
        found=i;
    }
    return found;
}
#endif
