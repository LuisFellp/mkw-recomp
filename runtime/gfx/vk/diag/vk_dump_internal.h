// Internal interface of the frame dump (vk_dump.c, vk_dump_img.c, vk_dump_buf.c): its state and the calls between
// the parts. Everything else goes through vk.h.
#pragma once

#include <stdint.h>
#include <stdio.h>

#include "vk.h"
#include "vk_exec.h"
#include "platform/platform.h"

#define MAX_PASSES 400
#define PNG_CAP (2ull << 30)
#define INFLIGHT_CAP (1ull << 30)   // host buffers waiting for their submission

// vk_dump.c
extern uint64_t dump_frame;
extern char dump_dir[400];
extern FILE *images_f;
extern volatile int64_t png_bytes, inflight;
extern PlatMutex img_lock;
VmaAllocator dvma(void);

// vk_dump_img.c: formats, the copy of an image into a host buffer and its PNGs
enum { M_UNORM, M_SNORM, M_INT, M_FLOAT, M_DEPTH };   // how a format's values read (mode_of)
typedef struct Fmt { VkFormat vk; const char *name; uint8_t bytes, nc, bits, kind, bgr; } Fmt;
const Fmt *fmt_of(VkFormat f);   // NULL: a format the dump does not decode
int mode_of(const Fmt *f);
void decode(const Fmt *f, const uint8_t *p, float o[4]);   // one texel as copied out; o[3] is 1 without alpha
const char *fmt_name(VkFormat f, char tmp[16]);
const char *copy_region(VkExec *ex, VkImage image, VkFormat vf, VkImageAspectFlags aspect, uint32_t level,
                        uint32_t layer, uint32_t z, VkExtent3D ext, uint32_t w, uint32_t h, uint32_t d,
                        const char *name);
const char *copy_out(VkExec *ex, VkImage image, VkFormat vf, VkImageAspectFlags aspect, uint32_t level,
                     uint32_t layer, uint32_t w, uint32_t h, uint32_t d, const char *name);

// vk_dump_buf.c: MKW_DUMP_BUF
extern uint64_t bd_addr, bd_size, bd_off, bd_stride;
extern uint32_t bd_w, bd_h;
extern FILE *bufs_f;
void buf_snap(VkExec *ex, int pass);
