/* param-layout-test: the UG2 parameter block layout (src/render_params.c)
 * against the std140 rules and against the uniforms that src/shaders/ug2.*
 * actually declare. No GL context needed. */
#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render_params.h"
#include "generated_shaders.h"

static const RhiParamField *find(const char *name) {
    for (uint32_t i = 0; i < ug2_params_layout.field_count; i++)
        if (!strcmp(ug2_params_layout.fields[i].name, name)) return &ug2_params_layout.fields[i];
    return NULL;
}

/* Expected RhiParamType for a GLSL uniform type; 0 = not a parameter type. */
static RhiParamType type_of(const char *glsl) {
    if (!strcmp(glsl, "float")) return RHI_PARAM_FLOAT;
    if (!strcmp(glsl, "vec2"))  return RHI_PARAM_VEC2;
    if (!strcmp(glsl, "vec3"))  return RHI_PARAM_VEC3;
    if (!strcmp(glsl, "vec4"))  return RHI_PARAM_VEC4;
    if (!strcmp(glsl, "mat4"))  return RHI_PARAM_MAT4;
    if (!strcmp(glsl, "sampler2D") || !strcmp(glsl, "samplerCube")) return RHI_PARAM_INT;
    return (RhiParamType)0;
}

/* Every "uniform TYPE a, b[N];" in the source must have a layout field of the
 * same type and array length (sampler arrays are spelled name0, name1, ...). */
static int check_source(const unsigned char *src, unsigned len) {
    char *t = (char *)malloc(len + 1);
    assert(t);
    memcpy(t, src, len); t[len] = 0;
    for (char *c = strstr(t, "/*"); c; c = strstr(c, "/*")) {   /* drop comments */
        char *e = strstr(c + 2, "*/");
        assert(e);
        memset(c, ' ', (size_t)(e + 2 - c));
    }
    int n = 0;
    for (char *u = strstr(t, "uniform"); u; ) {
        if (u > t && (isalnum((unsigned char)u[-1]) || u[-1] == '_')) { u = strstr(u + 7, "uniform"); continue; }
        char type[32], *p = u + 7;
        assert(sscanf(p, " %31s", type) == 1);
        p = strstr(p, type) + strlen(type);
        char *end = strchr(p, ';');
        assert(end);
        *end = 0;
        for (char *v = strtok(p, ", \n\t"); v; v = strtok(NULL, ", \n\t")) {
            char name[64]; int count = 1;
            char *br = strchr(v, '[');
            if (br) { count = atoi(br + 1); *br = 0; }
            snprintf(name, sizeof name, "%s", v);
            const RhiParamField *f = find(name);
            if (!f) {
                printf("param-layout-test: %s %s is missing from ug2_params_layout\n", type, name);
                exit(1);
            }
            assert(f->type == type_of(type) && (int)f->count == count);
            n++;
        }
        u = strstr(end + 1, "uniform");
    }
    free(t);
    return n;
}

int main(void) {
    assert(rhi_param_layout_check(&ug2_params_layout) == RHI_OK);
    assert(ug2_params_layout.size == sizeof(UG2Params));
    int n = check_source(src_shaders_ug2_vert, src_shaders_ug2_vert_len);
    n += check_source(src_shaders_ug2_frag, src_shaders_ug2_frag_len);
    /* uMVP is declared in both stages; every field is used by the shader */
    assert(n == (int)ug2_params_layout.field_count + 1);

    /* the checker rejects what std140 forbids */
    typedef struct { float a; float v[3]; } Misaligned;
    RhiParamField misaligned[] = { {"a", RHI_PARAM_FLOAT, offsetof(Misaligned, a), 1},
                                   {"v", RHI_PARAM_VEC3, offsetof(Misaligned, v), 1} };
    RhiParamField float_array[] = { {"f", RHI_PARAM_FLOAT, 0, 4} };
    RhiParamField overlap[] = { {"a", RHI_PARAM_VEC4, 0, 1}, {"b", RHI_PARAM_FLOAT, 8, 1} };
    RhiParamField outside[] = { {"m", RHI_PARAM_MAT4, 0, 2} };
    RhiParamLayout bad[] = {
        { misaligned, 2, sizeof(Misaligned), "misaligned" },
        { float_array, 1, 16, "float array" },
        { overlap, 2, 16, "overlap" },
        { outside, 1, 64, "outside" },
    };
    printf("param-layout-test: the next 4 rejections are expected\n"); fflush(stdout);
    for (unsigned i = 0; i < sizeof bad / sizeof bad[0]; i++)
        assert(rhi_param_layout_check(&bad[i]) == RHI_ERR_INVALID);
    printf("param-layout-test: PASS (%u fields, %zu bytes, std140)\n",
           (unsigned)ug2_params_layout.field_count, sizeof(UG2Params));
    return 0;
}
