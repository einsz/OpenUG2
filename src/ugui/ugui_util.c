/* ugui_util.c -- colour helpers, name hash (SPEC 2), UTF-8 helpers. */
#include "ugui_core.h"
#include <math.h>

uint8_t ug_clamp8(float v) {
    float r = floorf(v + 0.5f);
    if (r < 0.0f) r = 0.0f;
    if (r > 255.0f) r = 255.0f;
    return (uint8_t)r;
}

UgColor ug_color_mul(UgColor a, UgColor b) {
    UgColor c;
    /* SPEC 1.4 (amendment 2): (a*b + 127) / 255, so 255 x 255 = 255 */
    c.r = (uint8_t)((a.r * b.r + 127) / 255);
    c.g = (uint8_t)((a.g * b.g + 127) / 255);
    c.b = (uint8_t)((a.b * b.b + 127) / 255);
    c.a = (uint8_t)((a.a * b.a + 127) / 255);
    return c;
}

UgColor ug_color_lerp(UgColor a, UgColor b, float t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    UgColor c;
    c.r = ug_clamp8(a.r + (b.r - a.r) * t);
    c.g = ug_clamp8(a.g + (b.g - a.g) * t);
    c.b = ug_clamp8(a.b + (b.b - a.b) * t);
    c.a = ug_clamp8(a.a + (b.a - a.a) * t);
    return c;
}

UgColor ug_color_alpha(UgColor c, float a01) {
    if (a01 < 0) a01 = 0;
    if (a01 > 1) a01 = 1;
    c.a = ug_clamp8(c.a * a01);
    return c;
}

uint32_t ug_hash_n(const char *s, size_t n) {
    uint32_t h = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) h = h * 33u + (uint8_t)s[i];
    return h;
}

uint32_t ug_hash(const char *s) {
    uint32_t h = 0xFFFFFFFFu;
    for (; *s; s++) h = h * 33u + (uint8_t)*s;
    return h;
}

uint32_t ug_hash_upper(const char *s) {
    uint32_t h = 0xFFFFFFFFu;
    for (; *s; s++) {
        uint8_t c = (uint8_t)*s;
        if (c >= 'a' && c <= 'z') c = (uint8_t)(c - 32);
        h = h * 33u + c;
    }
    return h;
}

uint32_t ug_texture_key(const char *file_name) {
    /* base name: strip directories and the last extension */
    const char *base = file_name, *p;
    for (p = file_name; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;
    size_t n = 0, dot = (size_t)-1;
    for (p = base; *p; p++, n++)
        if (*p == '.') dot = n;
    if (dot != (size_t)-1) n = dot;
    uint32_t h = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) {
        uint8_t c = (uint8_t)base[i];
        if (c >= 'a' && c <= 'z') c = (uint8_t)(c - 32);
        h = h * 33u + c;
    }
    return h;
}

size_t ug_utf8_encode(uint32_t cp, char out[4]) {
    if (cp < 0x80) { out[0] = (char)cp; return 1; }
    if (cp < 0x800) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    out[0] = (char)(0xF0 | (cp >> 18));
    out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[3] = (char)(0x80 | (cp & 0x3F));
    return 4;
}

uint32_t ug_utf8_next(const char **sp) {
    const uint8_t *s = (const uint8_t *)*sp;
    uint32_t c = *s;
    int extra;
    if (c == 0) return 0;
    if (c < 0x80) { *sp += 1; return c; }
    if ((c & 0xE0) == 0xC0) { extra = 1; c &= 0x1F; }
    else if ((c & 0xF0) == 0xE0) { extra = 2; c &= 0x0F; }
    else if ((c & 0xF8) == 0xF0) { extra = 3; c &= 0x07; }
    else { *sp += 1; return 0xFFFD; }
    for (int i = 1; i <= extra; i++) {
        if ((s[i] & 0xC0) != 0x80) { *sp += i; return 0xFFFD; }
        c = (c << 6) | (s[i] & 0x3F);
    }
    *sp += extra + 1;
    return c;
}
