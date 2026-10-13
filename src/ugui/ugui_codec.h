/* ugui_codec.h -- decompression of original assets (SPEC 13). GL-free.
 *
 *   JDLZ  LZ77 variant with two flag streams (13.2)
 *   HUFF  canonical Huffman, public EA "30FB" method (13.3)
 *
 * Both have a 16-byte wrapper header; detection is by the first 4 bytes. */
#ifndef UGUI_CODEC_H
#define UGUI_CODEC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { UG_CODEC_NONE = 0, UG_CODEC_JDLZ, UG_CODEC_HUFF } UgCodec;

/* Which codec the buffer starts with (by magic and a minimal header check). */
UgCodec ug_codec_detect(const uint8_t *data, size_t len);

/* Decoders take the whole stream including the 16-byte wrapper. Return 1 and a
 * malloc'ed buffer (caller frees) on success, 0 + message on corrupt input.
 * Trailing bytes after the encoded size are ignored (13.1). */
int ug_jdlz_decode(const uint8_t *in, size_t len, uint8_t **out, size_t *out_len,
                   char *err, size_t errlen);
int ug_huff_decode(const uint8_t *in, size_t len, uint8_t **out, size_t *out_len,
                   char *err, size_t errlen);

/* Detect and decode. Returns 1 decoded (`*out` new buffer), 2 not compressed
 * (`*out` untouched, caller keeps using `in`), 0 corrupt. */
int ug_decompress(const uint8_t *in, size_t len, uint8_t **out, size_t *out_len,
                  char *err, size_t errlen);

/* Upper bound accepted for a decoded size (sanity limit against corrupt files). */
#define UG_CODEC_MAX_OUT (512u * 1024u * 1024u)

#ifdef __cplusplus
}
#endif
#endif
