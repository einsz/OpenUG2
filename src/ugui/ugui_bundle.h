/* ugui_bundle.h -- resource bundles: chunk walking and package extraction
 * (SPEC 3.2, 14). GL-free. */
#ifndef UGUI_BUNDLE_H
#define UGUI_BUNDLE_H

#include "ugui_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- owned byte buffer ---- */
typedef struct { uint8_t *data; size_t len; } UgBuf;
void ug_buf_free(UgBuf *b);
/* Reads a whole file. */
int  ug_file_read(const char *path, UgBuf *out, char *err, size_t errlen);
/* Reads a file and decompresses it when it is a JDLZ/HUFF stream (14.3). */
int  ug_bundle_read(const char *path, UgBuf *out, char *err, size_t errlen);

/* ---- chunk walk (14.1) ---- */
typedef struct {
    uint32_t id;
    const uint8_t *payload;   /* inside the walked buffer */
    uint32_t size;
    int depth;
    const uint8_t *base;      /* start of the whole buffer (file offsets are payload - base) */
    size_t base_len;          /* length of the whole buffer */
    const uint8_t *hdr;       /* start of the chunk header */
} UgChunkRef;

/* Ids with top nibble 8 nest chunks; so do the three texture-pack ids (15.1). */
int ug_chunk_is_container(uint32_t id);

/* Visitor return value: 0 = continue (descend into containers), 1 = handled,
 * do not descend. Padding chunks (id 0) are skipped. Returns 0 with a message
 * when a chunk runs past its parent; chunks seen before that were visited. */
typedef int (*UgChunkFn)(void *user, const UgChunkRef *c);
int ug_chunks_walk(const uint8_t *buf, size_t len, UgChunkFn fn, void *user,
                   char *err, size_t errlen);
/* Same, for a sub-range [buf, buf+len) of a larger buffer starting at `base`. */
int ug_chunks_walk_in(const uint8_t *base, size_t base_len, const uint8_t *buf, size_t len,
                      UgChunkFn fn, void *user, char *err, size_t errlen);

/* ---- screen packages (3.2, 14.3) ---- */
#define UG_CHUNK_PKG_HASHED 0x00030210u
#define UG_CHUNK_PKG_RAW    0x00030203u
#define UG_CHUNK_FONT       0x00030201u
#define UG_CHUNK_STRINGS    0x00039000u

typedef struct {
    uint32_t key;      /* hash(upper(PkHd name)) */
    char *name;        /* PkHd name, e.g. "Menu.fng" */
    uint8_t *data;     /* decoded package bytes starting at the root chunk */
    size_t len;
} UgPackageBlob;

/* Builds a blob from a 0x00030210 or 0x00030203 chunk: decodes JDLZ/HUFF when
 * present and reads the name from the package's own header. */
int  ug_package_from_chunk(const UgChunkRef *c, UgPackageBlob *out, char *err, size_t errlen);
void ug_package_free(UgPackageBlob *p);

#ifdef __cplusplus
}
#endif
#endif
