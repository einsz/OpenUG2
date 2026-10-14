#include "ugui_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    UgProps p={0},saved={0};UgTrack tr={0};UgScript s={0};s.tracks=&tr;s.ntracks=1;
    tr.type=UG_VT_FLOAT;tr.has_base=1;tr.base.v[0]=7;
    tr.prop=UINT32_MAX;ug_script_apply(&s,0,&p);assert(!memcmp(&p,&saved,sizeof p));
    tr.prop=UG_NPROPS;ug_script_apply(&s,0,&p);assert(!memcmp(&p,&saved,sizeof p));
    tr.type=UG_VT_VEC3;tr.prop=UG_NPROPS-2;ug_script_apply(&s,0,&p);assert(!memcmp(&p,&saved,sizeof p));
    tr.type=UG_VT_FLOAT;tr.prop=UG_NPROPS-1;ug_script_apply(&s,0,&p);assert(p.w[UG_NPROPS-1]==7);
    puts("ugui_boundary_test: PASS (property bounds and valid final field)");return 0;
}
