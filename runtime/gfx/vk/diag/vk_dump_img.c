#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"
#include "host.h"
#include "nvn_api.h"
#include "vk.h"
#include "vk_exec.h"
#include "platform/platform.h"
#include "vk_dump_internal.h"

// ---- formats

enum { K_UNORM, K_SNORM, K_UINT, K_SINT, K_FLOAT, K_RGB10A2, K_BGR10A2, K_RG11B10, K_E5, K_R5G6B5, K_D16, K_D24, K_D32F };

#define F(v, n, by, nc, bi, k) {VK_FORMAT_##v, n, by, nc, bi, k, 0}
static const Fmt fmts[] = {
    F(R8_UNORM, "r8", 1, 1, 8, K_UNORM), F(R8_SNORM, "r8sn", 1, 1, 8, K_SNORM), F(R8_UINT, "r8ui", 1, 1, 8, K_UINT),
    F(R8_SINT, "r8i", 1, 1, 8, K_SINT), F(R8_SRGB, "r8srgb", 1, 1, 8, K_UNORM),
    F(R8G8_UNORM, "rg8", 2, 2, 8, K_UNORM), F(R8G8_SNORM, "rg8sn", 2, 2, 8, K_SNORM),
    F(R8G8_UINT, "rg8ui", 2, 2, 8, K_UINT), F(R8G8_SINT, "rg8i", 2, 2, 8, K_SINT),
    F(R8G8B8A8_UNORM, "rgba8", 4, 4, 8, K_UNORM), F(R8G8B8A8_SNORM, "rgba8sn", 4, 4, 8, K_SNORM),
    F(R8G8B8A8_UINT, "rgba8ui", 4, 4, 8, K_UINT), F(R8G8B8A8_SINT, "rgba8i", 4, 4, 8, K_SINT),
    F(R8G8B8A8_SRGB, "rgba8srgb", 4, 4, 8, K_UNORM),
    {VK_FORMAT_B8G8R8A8_UNORM, "bgra8", 4, 4, 8, K_UNORM, 1}, {VK_FORMAT_B8G8R8A8_SRGB, "bgra8srgb", 4, 4, 8, K_UNORM, 1},
    F(A2B10G10R10_UNORM_PACK32, "rgb10a2", 4, 4, 10, K_RGB10A2), F(A2R10G10B10_UNORM_PACK32, "bgr10a2", 4, 4, 10, K_BGR10A2),
    F(R5G6B5_UNORM_PACK16, "r5g6b5", 2, 3, 5, K_R5G6B5),
    F(R16_UNORM, "r16", 2, 1, 16, K_UNORM), F(R16_SNORM, "r16sn", 2, 1, 16, K_SNORM), F(R16_UINT, "r16ui", 2, 1, 16, K_UINT),
    F(R16_SINT, "r16i", 2, 1, 16, K_SINT), F(R16_SFLOAT, "r16f", 2, 1, 16, K_FLOAT),
    F(R16G16_UNORM, "rg16", 4, 2, 16, K_UNORM), F(R16G16_SNORM, "rg16sn", 4, 2, 16, K_SNORM),
    F(R16G16_UINT, "rg16ui", 4, 2, 16, K_UINT), F(R16G16_SINT, "rg16i", 4, 2, 16, K_SINT),
    F(R16G16_SFLOAT, "rg16f", 4, 2, 16, K_FLOAT),
    F(R16G16B16A16_UNORM, "rgba16", 8, 4, 16, K_UNORM), F(R16G16B16A16_SNORM, "rgba16sn", 8, 4, 16, K_SNORM),
    F(R16G16B16A16_UINT, "rgba16ui", 8, 4, 16, K_UINT), F(R16G16B16A16_SINT, "rgba16i", 8, 4, 16, K_SINT),
    F(R16G16B16A16_SFLOAT, "rgba16f", 8, 4, 16, K_FLOAT),
    F(R32_UINT, "r32ui", 4, 1, 32, K_UINT), F(R32_SINT, "r32i", 4, 1, 32, K_SINT), F(R32_SFLOAT, "r32f", 4, 1, 32, K_FLOAT),
    F(R32G32_UINT, "rg32ui", 8, 2, 32, K_UINT), F(R32G32_SINT, "rg32i", 8, 2, 32, K_SINT),
    F(R32G32_SFLOAT, "rg32f", 8, 2, 32, K_FLOAT),
    F(R32G32B32A32_UINT, "rgba32ui", 16, 4, 32, K_UINT), F(R32G32B32A32_SINT, "rgba32i", 16, 4, 32, K_SINT),
    F(R32G32B32A32_SFLOAT, "rgba32f", 16, 4, 32, K_FLOAT),
    F(B10G11R11_UFLOAT_PACK32, "r11g11b10f", 4, 3, 11, K_RG11B10), F(E5B9G9R9_UFLOAT_PACK32, "rgb9e5", 4, 3, 9, K_E5),
    // depth aspect of depth formats, as vkCmdCopyImageToBuffer writes it
    F(D16_UNORM, "d16", 2, 1, 16, K_D16), F(D16_UNORM_S8_UINT, "d16s8", 2, 1, 16, K_D16),
    F(X8_D24_UNORM_PACK32, "d24", 4, 1, 24, K_D24), F(D24_UNORM_S8_UINT, "d24s8", 4, 1, 24, K_D24),
    F(D32_SFLOAT, "d32f", 4, 1, 32, K_D32F), F(D32_SFLOAT_S8_UINT, "d32fs8", 4, 1, 32, K_D32F),
};
#undef F

const Fmt *fmt_of(VkFormat f) {
    for (size_t i = 0; i < sizeof fmts / sizeof fmts[0]; i++)
        if (fmts[i].vk == f) return &fmts[i];
    return NULL;
}

const char *fmt_name(VkFormat f, char tmp[16]) {
    const Fmt *d = fmt_of(f);
    if (d) return d->name;
    snprintf(tmp, 16, "vk%d", (int)f);
    return tmp;
}

int mode_of(const Fmt *f) {
    switch (f->kind) {
    case K_SNORM: return M_SNORM;
    case K_UINT: case K_SINT: return M_INT;
    case K_FLOAT: case K_RG11B10: case K_E5: return M_FLOAT;
    case K_D16: case K_D24: case K_D32F: return M_DEPTH;
    default: return M_UNORM;
    }
}

// Unsigned small float with a 5-bit exponent and mbits of mantissa (half: 10, R11G11B10: 6 and 5).
static float ufloat(uint32_t v, int mbits) {
    uint32_t e = v >> mbits, m = v & ((1u << mbits) - 1);
    if (e == 0) return ldexpf((float)m, -14 - mbits);
    if (e == 31) return m ? NAN : INFINITY;
    return ldexpf((float)(m | 1u << mbits), (int)e - 15 - mbits);
}

static int32_t sext(uint32_t v, int bits) { return bits >= 32 ? (int32_t)v : (int32_t)(v << (32 - bits)) >> (32 - bits); }

void decode(const Fmt *f, const uint8_t *p, float o[4]) {
    uint32_t u = 0;
    o[0] = o[1] = o[2] = 0, o[3] = 1;
    switch (f->kind) {
    case K_RGB10A2: case K_BGR10A2:
        memcpy(&u, p, 4);
        o[0] = (u & 1023) / 1023.f, o[1] = (u >> 10 & 1023) / 1023.f, o[2] = (u >> 20 & 1023) / 1023.f, o[3] = (u >> 30) / 3.f;
        if (f->kind == K_BGR10A2) { float t = o[0]; o[0] = o[2], o[2] = t; }
        return;
    case K_RG11B10:
        memcpy(&u, p, 4);
        o[0] = ufloat(u & 2047, 6), o[1] = ufloat(u >> 11 & 2047, 6), o[2] = ufloat(u >> 22 & 1023, 5);
        return;
    case K_E5: {
        memcpy(&u, p, 4);
        float s = ldexpf(1.f, (int)(u >> 27) - 24);
        o[0] = (u & 511) * s, o[1] = (u >> 9 & 511) * s, o[2] = (u >> 18 & 511) * s;
        return;
    }
    case K_R5G6B5:
        memcpy(&u, p, 2);
        o[0] = (u >> 11 & 31) / 31.f, o[1] = (u >> 5 & 63) / 63.f, o[2] = (u & 31) / 31.f;
        return;
    case K_D16: memcpy(&u, p, 2); o[0] = u / 65535.f; return;
    case K_D24: memcpy(&u, p, 4); o[0] = (u & 0xffffff) / 16777215.f; return;
    case K_D32F: memcpy(&o[0], p, 4); return;
    }
    int cb = f->bits / 8;
    for (int c = 0; c < f->nc; c++) {
        u = 0;
        memcpy(&u, p + c * cb, (size_t)cb);
        float x;
        switch (f->kind) {
        case K_UNORM: x = (float)u / (float)((1ull << f->bits) - 1); break;
        case K_SNORM: x = fmaxf((float)sext(u, f->bits) / (float)((1u << (f->bits - 1)) - 1), -1.f); break;
        case K_UINT: x = (float)u; break;
        case K_SINT: x = (float)sext(u, f->bits); break;
        default:
            if (f->bits == 16) x = (u & 0x8000 ? -1.f : 1.f) * ufloat(u & 0x7fff, 10);
            else memcpy(&x, &u, 4);
        }
        o[c] = x;
    }
    if (f->bgr) { float t = o[0]; o[0] = o[2], o[2] = t; }
}

// ---- conversion on the completion thread

typedef struct {
    VkBuffer buf;
    VmaAllocation alloc;
    const uint8_t *map;
    uint64_t bytes;
    const Fmt *f;
    uint32_t w, h, d;   // d: 3D slices, laid side by side in the PNG
    char name[160];
} Job;

// Distinct raw texel values among rows [y0, y1) of every slice.
static uint64_t distinct(const Job *j, uint32_t y0, uint32_t y1) {
    uint64_t n = (uint64_t)j->w * (y1 - y0) * j->d, cap = 1024;
    while (cap < 2 * n) cap *= 2;
    uint64_t *set = calloc(cap, sizeof *set), count = 0;
    if (!set) return 0;
    for (uint32_t z = 0; z < j->d; z++)
        for (uint32_t y = y0; y < y1; y++)
            for (uint32_t x = 0; x < j->w; x++) {
                const uint8_t *p = j->map + (((uint64_t)z * j->h + y) * j->w + x) * j->f->bytes;
                uint64_t a = 0, b = 0;
                memcpy(&a, p, j->f->bytes < 8 ? j->f->bytes : 8);
                if (j->f->bytes > 8) memcpy(&b, p + 8, j->f->bytes - 8u);
                uint64_t k = (a ^ (b * 0x9E3779B97F4A7C15ull)) + 1;
                if (!k) k = 1;
                uint64_t i = (k * 0xff51afd7ed558ccdull) >> 17;
                for (;; i++) {
                    uint64_t *s = &set[i & (cap - 1)];
                    if (*s == k) break;
                    if (!*s) { *s = k, count++; break; }
                }
            }
    free(set);
    return count;
}

static uint8_t u8(float x) { return x > 0 ? x < 1 ? (uint8_t)(x * 255.f + .5f) : 255 : 0; }

static void write_images(const Job *j) {
    uint32_t pw = j->w * j->d, ph = j->h;
    uint64_t n = (uint64_t)pw * ph;
    float *px = malloc(n * 16);
    uint8_t *out = malloc(n * 4);
    if (!px || !out) rt_fatal("vk_dump: out of memory for %s", j->name);
    float mn[4] = {INFINITY, INFINITY, INFINITY, INFINITY}, mx[4] = {-INFINITY, -INFINITY, -INFINITY, -INFINITY};
    uint64_t nans = 0;
    uint32_t nan_x = 0, nan_y = 0, nan_x1 = 0, nan_y1 = 0;   // bounding box of the NaNs: a row or a column is a clue
    for (uint32_t z = 0; z < j->d; z++)
        for (uint32_t y = 0; y < j->h; y++)
            for (uint32_t x = 0; x < j->w; x++) {
                float *o = px + 4 * ((uint64_t)y * pw + (uint64_t)z * j->w + x);
                decode(j->f, j->map + (((uint64_t)z * j->h + y) * j->w + x) * j->f->bytes, o);
                for (int c = 0; c < 4; c++) {
                    if (isnan(o[c])) {
                        uint32_t px_x = z * j->w + x;
                        if (!nans++) nan_x = nan_x1 = px_x, nan_y = nan_y1 = y;
                        if (px_x < nan_x) nan_x = px_x;
                        if (px_x > nan_x1) nan_x1 = px_x;
                        if (y > nan_y1) nan_y1 = y;
                        o[c] = 0;
                        continue;
                    }
                    if (o[c] < mn[c]) mn[c] = o[c];
                    if (o[c] > mx[c]) mx[c] = o[c];
                }
            }
    int mode = mode_of(j->f), grey = j->f->nc == 1;
    float scale = 1, lo = mn[0], range = mx[0] - mn[0];
    if (mode == M_INT) {
        float m = 1;
        for (int c = 0; c < j->f->nc && c < 3; c++) m = fmaxf(m, fmaxf(fabsf(mx[c]), fabsf(mn[c])));
        scale = 1.f / m;
    }
    for (int pass = 0; pass < (mode == M_FLOAT ? 2 : 1); pass++) {
        for (uint64_t i = 0; i < n; i++) {
            const float *o = px + 4 * i;
            for (int c = 0; c < 3; c++) {
                float x = o[grey ? 0 : c];
                if (mode == M_SNORM) x = x * .5f + .5f;
                else if (mode == M_INT) x *= scale;
                else if (mode == M_DEPTH) x = range > 0 ? (x - lo) / range : 0;
                else if (mode == M_FLOAT && pass) x = x > 0 ? x / (1 + x) : 0;
                out[4 * i + c] = u8(x);
            }
            out[4 * i + 3] = 255;
        }
        char path[600];
        snprintf(path, sizeof path, "%s/%s%s.png", dump_dir, j->name, pass ? "_tm" : "");
        rt_write_png(path, out, pw, ph);
    }
    uint64_t all = distinct(j, 0, j->h), top = distinct(j, 0, j->h / 4 ? j->h / 4 : 1);
    plat_mutex_lock(&img_lock);
    if (images_f) {
        fprintf(images_f, "%s %ux%ux%u min %g %g %g %g max %g %g %g %g distinct %llu top-quarter %llu nan %llu", j->name,
                j->w, j->h, j->d, mn[0], mn[1], mn[2], mn[3], mx[0], mx[1], mx[2], mx[3], (unsigned long long)all,
                (unsigned long long)top, (unsigned long long)nans);
        if (nans) fprintf(images_f, " in %u,%u-%u,%u", nan_x, nan_y, nan_x1, nan_y1);
        fputc('\n', images_f);
        fflush(images_f);
    }
    plat_mutex_unlock(&img_lock);
    free(out);
    free(px);
}

static void job_done(void *arg) {
    Job *j = arg;
    VmaAllocator vma = dvma();
    vmaInvalidateAllocation(vma, j->alloc, 0, VK_WHOLE_SIZE);
    write_images(j);
    vmaDestroyBuffer(vma, j->buf, j->alloc);
    plat_add64(&inflight, -(int64_t)j->bytes);
    free(j);
}

// ---- recording

// Records the copy of one subresource into a host buffer and queues its PNGs. Returns NULL, or why it was skipped.
// The buffer is w x h x d texels of vf; the image region is `ext` image texels from off (differs for the blocks of a
// BC image read as an uncompressed format of the block size, a 3D slice, and a draw snapshot's MKW_DUMP_DRAWS_RECT).
const char *copy_region(VkExec *ex, VkImage image, VkFormat vf, VkImageAspectFlags aspect, uint32_t level,
                               uint32_t layer, VkOffset3D off, VkExtent3D ext, uint32_t w, uint32_t h, uint32_t d,
                               const char *name) {
    const Fmt *f = fmt_of(vf);
    if (!f) return "format not decoded";
    if (__atomic_load_n(&gfx_counters.frames, __ATOMIC_RELAXED) + 1 != dump_frame && (uint64_t)w * h * d > 65536)
        return "logged frame, large image";
    if (aspect & VK_IMAGE_ASPECT_DEPTH_BIT) aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    else if (aspect != VK_IMAGE_ASPECT_COLOR_BIT) return "stencil only";
    uint64_t bytes = (uint64_t)w * h * d * f->bytes;
    uint64_t est = ((uint64_t)h * (1 + 4ull * w * d) + 128) * (mode_of(f) == M_FLOAT ? 2 : 1);
    if ((uint64_t)png_bytes + est > PNG_CAP) return "PNG cap reached";
    if ((uint64_t)inflight + bytes > INFLIGHT_CAP) return "too many copies in flight";
    Job *j = calloc(1, sizeof *j);
    if (!j) rt_fatal("vk_dump: out of memory");
    VkBufferCreateInfo bi = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = bytes,
                             .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT};
    VmaAllocationCreateInfo ai = {.usage = VMA_MEMORY_USAGE_AUTO,
                                  .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT};
    VmaAllocationInfo info;
    if (vmaCreateBuffer(dvma(), &bi, &ai, &j->buf, &j->alloc, &info) != VK_SUCCESS || !info.pMappedData) {
        free(j);
        return "host buffer allocation failed";
    }
    j->map = info.pMappedData, j->bytes = bytes, j->f = f, j->w = w, j->h = h, j->d = d;
    snprintf(j->name, sizeof j->name, "%s", name);
    VkMemoryBarrier2 pre = {.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                            .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                            .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                            .dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                            .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT};
    VkMemoryBarrier2 post = {.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                             .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                             .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                             .dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
                             .dstAccessMask = VK_ACCESS_2_HOST_READ_BIT};
    VkDependencyInfo di = {.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .memoryBarrierCount = 1, .pMemoryBarriers = &pre};
    vkCmdPipelineBarrier2(ex->cb, &di);
    VkBufferImageCopy r = {.imageSubresource = {aspect, level, layer, 1}, .imageOffset = off,
                           .imageExtent = ext};   // packed rows: a BC row of ext.width texels is w blocks
    vkCmdCopyImageToBuffer(ex->cb, image, VK_IMAGE_LAYOUT_GENERAL, j->buf, 1, &r);
    di.pMemoryBarriers = &post;
    vkCmdPipelineBarrier2(ex->cb, &di);
    ex->recorded += 3;
    plat_add64(&png_bytes, (int64_t)est);
    plat_add64(&inflight, (int64_t)bytes);
    vk_completion_add(ex->fence, job_done, j);
    return NULL;
}

const char *copy_out(VkExec *ex, VkImage image, VkFormat vf, VkImageAspectFlags aspect, uint32_t level,
                            uint32_t layer, uint32_t w, uint32_t h, uint32_t d, const char *name) {
    return copy_region(ex, image, vf, aspect, level, layer, (VkOffset3D){0, 0, 0}, (VkExtent3D){w, h, d}, w, h, d, name);
}

