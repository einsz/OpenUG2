/* rhi_vk.c - Vulkan backend of rhi.h (issue #14, P11a slice).
 *
 * Offscreen v0: no swapchain, no surface. The "main target" is a single RGBA8
 * VkImage (plus a D16 depth image) sized to the window, which we clear and read
 * back. This slice implements only device/frame/readback/clear/caps; buffers,
 * textures, shaders, pipelines, draws and passes are stubs returning {0} /
 * RHI_ERR_UNSUPPORTED until the later P11b.. slices fill them in.
 *
 * The device/queue/offscreen-image/readback flow mirrors the proven,
 * validation-clean spike in ~/nfsu2/vk-spike/vk_triangle.c (its CHECK macro,
 * dbg_cb, find_mem and image->host-buffer copy); the g_vk singleton mirrors
 * g_rhi in rhi_gl.c and the semantics (caps, drawable size, read_pixels row
 * order/format, rhi_cmd_clear_main) match rhi_gl.c exactly. */
#include "rhi.h"
#include "generated_shaders_vk.h"   /* build-time SPIR-V for the src/shaders/vk shaders */
#include <SDL.h>
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Validation-message counter, like the spike. Non-static so the smoke test can
 * read it (no rhi.h surface for it); WARNING and above count as a failure. */
unsigned rhi_vk_validation_messages;

#define VK_DEFAULT_W 1280
#define VK_DEFAULT_H 720

/* Buffer handle id = generation << 24 | (slot + 1), id 0 = no buffer, the same
 * scheme and bit layout as the GL backend's buffer table (rhi_gl.c). */
#define VK_SLOT_BITS 24u
#define VK_SLOT_MASK ((1u << VK_SLOT_BITS) - 1u)

typedef struct {
    VkBuffer           buf;         /* VK_NULL_HANDLE when the slot holds no storage */
    VkDeviceMemory     mem;
    void              *mapped;      /* persistent map for STREAM, NULL for STATIC */
    VkDeviceSize       size;        /* current storage size in bytes */
    VkBufferUsageFlags usage;       /* VERTEX or INDEX usage bit (no transfer bits) */
    uint8_t            update;      /* RHI_UPDATE_STATIC / RHI_UPDATE_STREAM */
    uint8_t            live;        /* 1 while the slot is allocated (STREAM size 0 keeps buf NULL) */
    uint8_t            generation;
} RhiVkBuffer;

/* Texture handle slot, same scheme as RhiVkBuffer. The whole image (every mip
 * level and array layer) is tracked by one `layout`: uploads/blits/readbacks
 * transition the full subresource range together, so it stays uniform. */
typedef struct {
    VkImage            img;         /* VK_NULL_HANDLE = free slot */
    VkDeviceMemory     mem;
    VkImageView        view;        /* default 2D/CUBE sampling (or depth) view */
    VkFormat           vkfmt;
    VkImageAspectFlags aspect;      /* COLOR, or DEPTH for D16/DEPTH */
    VkImageLayout      layout;      /* current layout of all subresources */
    RhiFormat          format;
    uint32_t           usage;
    int32_t            width, height;
    uint32_t           layers;      /* 1 for 2D, 6 for CUBE */
    uint32_t           mip_levels;
    RhiSampler         sampler;     /* sampled with this (recorded; used in P11e) */
    uint8_t            live;
    uint8_t            generation;
} RhiVkTexture;

/* One parameter block = a host-visible, persistently-mapped UBO. The game writes
 * params through the mapping (rhi_shader_params), so the UBO is always current
 * for the next draw (host-coherent; no GL-style flush needed). Its std140 layout
 * equals the RhiParamLayout, which rhi_param_layout_check enforces. */
#define VK_MAX_PARAM_BLOCKS 4u
typedef struct {
    VkBuffer       ubo;
    VkDeviceMemory ubo_mem;
    void          *mapped;
    uint32_t       size;
    const RhiParamLayout *layout;
} RhiVkParamBlock;

typedef struct {
    VkShaderModule  vert, frag;     /* VK_NULL_HANDLE = free slot */
    RhiVkParamBlock blocks[VK_MAX_PARAM_BLOCKS];
    uint32_t        block_count;
    uint8_t         live;
    uint8_t         generation;
} RhiVkShader;

static struct {
    SDL_Window              *window;
    VkInstance               instance;
    VkDebugUtilsMessengerEXT messenger;
    VkPhysicalDevice         phys;
    VkDevice                 device;
    uint32_t                 queue_family;
    VkQueue                  queue;
    VkCommandPool            pool;
    VkCommandBuffer          cmd;        /* one primary buffer, re-used per frame */
    VkFence                  fence;      /* frame submit completion */

    VkImage        color;                /* offscreen RGBA8 main target */
    VkDeviceMemory color_mem;
    VkImageLayout  color_layout;         /* tracked so barriers are minimal/clean */
    VkImage        depth;                /* D16 depth target */
    VkDeviceMemory depth_mem;

    VkBuffer       readback;             /* host-visible, full-frame sized */
    VkDeviceMemory readback_mem;

    RhiCaps  caps;
    int32_t  width, height;              /* offscreen target size (pixels) */
    uint64_t frame_index;
    int      in_frame;
    int      recording;                  /* cmd buffer is mid-recording */

    struct {                             /* buffer handle table, shape of GL's g_buffers */
        RhiVkBuffer *slots;
        uint32_t     count, capacity;
        uint32_t    *free_list;
        uint32_t     free_count;
        unsigned     stale_logged;
    } buffers;
    struct {                             /* texture handle table, same shape */
        RhiVkTexture *slots;
        uint32_t      count, capacity;
        uint32_t     *free_list;
        uint32_t      free_count;
        unsigned      stale_logged;
    } textures;
    struct {                             /* sampler list, searched by value (GL's g_samplers) */
        RhiSamplerDesc *descs;
        VkSampler      *handles;
        uint32_t        count, capacity;
    } samplers;
    struct {                             /* shader handle table, same shape as buffers */
        RhiVkShader *slots;
        uint32_t     count, capacity;
        uint32_t    *free_list;
        uint32_t     free_count;
        unsigned     stale_logged;
    } shaders;
} g_vk;

/* -------------------------------------------------------------------------- */
/* Helpers                                                                    */
/* -------------------------------------------------------------------------- */
static VKAPI_ATTR VkBool32 VKAPI_CALL dbg_cb(VkDebugUtilsMessageSeverityFlagBitsEXT sev,
    VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT *d, void *u) {
    (void)type; (void)u;
    if (sev >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        rhi_vk_validation_messages++;
        fprintf(stderr, "[vk validation %s] %s\n",
                sev >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ? "ERROR" : "WARN", d->pMessage);
    }
    return VK_FALSE;
}

/* Log and translate a VkResult; returns 1 on VK_SUCCESS. */
static int vk_ok(VkResult r, const char *what) {
    if (r == VK_SUCCESS) return 1;
    fprintf(stderr, "rhi_vk: %s failed: %d\n", what, (int)r);
    return 0;
}

/* First memory type in `bits` with every `want` property, UINT32_MAX if none. */
static uint32_t find_mem(uint32_t bits, VkMemoryPropertyFlags want) {
    VkPhysicalDeviceMemoryProperties mp;
    vkGetPhysicalDeviceMemoryProperties(g_vk.phys, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
        if ((bits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & want) == want) return i;
    return UINT32_MAX;
}

/* Create an image + device-local memory, binding them. 1 on success. */
static int make_image(VkFormat fmt, VkImageUsageFlags usage, VkImage *img, VkDeviceMemory *mem) {
    VkImageCreateInfo ci = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, NULL, 0, VK_IMAGE_TYPE_2D, fmt,
        { (uint32_t)g_vk.width, (uint32_t)g_vk.height, 1 }, 1, 1, VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL, usage, VK_SHARING_MODE_EXCLUSIVE, 0, NULL, VK_IMAGE_LAYOUT_UNDEFINED };
    if (!vk_ok(vkCreateImage(g_vk.device, &ci, NULL, img), "vkCreateImage")) return 0;
    VkMemoryRequirements mr; vkGetImageMemoryRequirements(g_vk.device, *img, &mr);
    uint32_t mt = find_mem(mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mt == UINT32_MAX) { fprintf(stderr, "rhi_vk: no device-local memory type\n"); return 0; }
    VkMemoryAllocateInfo ai = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, mr.size, mt };
    if (!vk_ok(vkAllocateMemory(g_vk.device, &ai, NULL, mem), "vkAllocateMemory(image)")) return 0;
    return vk_ok(vkBindImageMemory(g_vk.device, *img, *mem, 0), "vkBindImageMemory");
}

/* Record an image layout transition covering a single colour or depth level. */
static void image_barrier(VkImage img, VkImageAspectFlags aspect, VkImageLayout old, VkImageLayout neu,
                          VkAccessFlags src_access, VkAccessFlags dst_access,
                          VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage) {
    VkImageMemoryBarrier b = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, NULL, src_access, dst_access,
        old, neu, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, img, { aspect, 0, 1, 0, 1 } };
    vkCmdPipelineBarrier(g_vk.cmd, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &b);
}

/* Fresh recording on the frame command buffer if not already open. */
static int ensure_recording(void) {
    if (g_vk.recording) return 1;
    if (!vk_ok(vkResetCommandBuffer(g_vk.cmd, 0), "vkResetCommandBuffer")) return 0;
    VkCommandBufferBeginInfo bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, NULL };
    if (!vk_ok(vkBeginCommandBuffer(g_vk.cmd, &bi), "vkBeginCommandBuffer")) return 0;
    g_vk.recording = 1;
    return 1;
}

/* End, submit and wait for the frame command buffer (offscreen: no present). */
static int flush(void) {
    if (!g_vk.recording) return 1;
    if (!vk_ok(vkEndCommandBuffer(g_vk.cmd), "vkEndCommandBuffer")) return 0;
    g_vk.recording = 0;
    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, NULL, 1, &g_vk.cmd, 0, NULL };
    if (!vk_ok(vkQueueSubmit(g_vk.queue, 1, &si, g_vk.fence), "vkQueueSubmit")) return 0;
    if (!vk_ok(vkWaitForFences(g_vk.device, 1, &g_vk.fence, VK_TRUE, UINT64_MAX), "vkWaitForFences")) return 0;
    return vk_ok(vkResetFences(g_vk.device, 1, &g_vk.fence), "vkResetFences");
}

/* Create a VkBuffer of `size` bytes with `usage`, back it with memory having
 * every `props` property and bind them. Cleans up on any failure. 1 on success. */
static int make_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props,
                       VkBuffer *buf, VkDeviceMemory *mem) {
    *buf = VK_NULL_HANDLE; *mem = VK_NULL_HANDLE;
    VkBufferCreateInfo bci = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, NULL, 0, size, usage,
        VK_SHARING_MODE_EXCLUSIVE, 0, NULL };
    if (!vk_ok(vkCreateBuffer(g_vk.device, &bci, NULL, buf), "vkCreateBuffer")) return 0;
    VkMemoryRequirements mr; vkGetBufferMemoryRequirements(g_vk.device, *buf, &mr);
    uint32_t mt = find_mem(mr.memoryTypeBits, props);
    if (mt == UINT32_MAX) {
        fprintf(stderr, "rhi_vk: no suitable buffer memory type\n");
        vkDestroyBuffer(g_vk.device, *buf, NULL); *buf = VK_NULL_HANDLE; return 0;
    }
    VkMemoryAllocateInfo ai = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, mr.size, mt };
    if (!vk_ok(vkAllocateMemory(g_vk.device, &ai, NULL, mem), "vkAllocateMemory(buffer)")) {
        vkDestroyBuffer(g_vk.device, *buf, NULL); *buf = VK_NULL_HANDLE; return 0;
    }
    if (!vk_ok(vkBindBufferMemory(g_vk.device, *buf, *mem, 0), "vkBindBufferMemory")) {
        vkFreeMemory(g_vk.device, *mem, NULL); vkDestroyBuffer(g_vk.device, *buf, NULL);
        *buf = VK_NULL_HANDLE; *mem = VK_NULL_HANDLE; return 0;
    }
    return 1;
}

/* One-shot `vkCmdCopyBuffer` of `size` bytes src[src_off] -> dst[dst_off] on a
 * temporary command buffer, blocking on a fence until it completes. The device
 * queue serialises this with any frame submit, so it is safe mid-frame. 1 ok. */
static int copy_buffer(VkBuffer src, VkDeviceSize src_off, VkBuffer dst, VkDeviceSize dst_off,
                       VkDeviceSize size) {
    VkCommandBuffer cb = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo cbai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, NULL, g_vk.pool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
    if (!vk_ok(vkAllocateCommandBuffers(g_vk.device, &cbai, &cb), "vkAllocateCommandBuffers(copy)")) return 0;
    VkCommandBufferBeginInfo bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, NULL };
    int ok = vk_ok(vkBeginCommandBuffer(cb, &bi), "vkBeginCommandBuffer(copy)");
    if (ok) {
        VkBufferCopy region = { src_off, dst_off, size };
        vkCmdCopyBuffer(cb, src, dst, 1, &region);
        ok = vk_ok(vkEndCommandBuffer(cb), "vkEndCommandBuffer(copy)");
    }
    VkFence fence = VK_NULL_HANDLE;
    if (ok) {
        VkFenceCreateInfo fci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, 0 };
        ok = vk_ok(vkCreateFence(g_vk.device, &fci, NULL, &fence), "vkCreateFence(copy)");
    }
    if (ok) {
        VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, NULL, 1, &cb, 0, NULL };
        ok = vk_ok(vkQueueSubmit(g_vk.queue, 1, &si, fence), "vkQueueSubmit(copy)") &&
             vk_ok(vkWaitForFences(g_vk.device, 1, &fence, VK_TRUE, UINT64_MAX), "vkWaitForFences(copy)");
    }
    if (fence) vkDestroyFence(g_vk.device, fence, NULL);
    vkFreeCommandBuffers(g_vk.device, g_vk.pool, 1, &cb);
    return ok;
}

/* Upload `size` bytes of `data` into device-local `dst` at `dst_off` through a
 * temporary host-visible staging buffer and a one-shot copy. 1 on success. */
static int stage_to_device(VkBuffer dst, VkDeviceSize dst_off, const void *data, VkDeviceSize size) {
    VkBuffer staging; VkDeviceMemory smem;
    if (!make_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &staging, &smem)) return 0;
    void *map = NULL;
    int ok = vk_ok(vkMapMemory(g_vk.device, smem, 0, VK_WHOLE_SIZE, 0, &map), "vkMapMemory(staging)");
    if (ok) {
        memcpy(map, data, size);
        vkUnmapMemory(g_vk.device, smem);
        ok = copy_buffer(staging, 0, dst, dst_off, size);
    }
    vkDestroyBuffer(g_vk.device, staging, NULL);
    vkFreeMemory(g_vk.device, smem, NULL);
    return ok;
}

/* -------------------------------------------------------------------------- */
/* Window setup                                                               */
/* -------------------------------------------------------------------------- */
RhiResult rhi_prepare_window(RhiBackend backend) {
    if (backend != RHI_BACKEND_DEFAULT && backend != RHI_BACKEND_VULKAN) return RHI_ERR_UNSUPPORTED;
    return RHI_OK;   /* Vulkan needs no SDL_GL_* context attributes */
}

uint32_t rhi_window_flags(RhiBackend backend) {
    return (backend == RHI_BACKEND_DEFAULT || backend == RHI_BACKEND_VULKAN)
               ? (uint32_t)SDL_WINDOW_VULKAN : 0u;
}

/* -------------------------------------------------------------------------- */
/* Capabilities                                                               */
/* -------------------------------------------------------------------------- */
/* 1 if `fmt` can be sampled with linear filtering from optimal-tiled storage. */
static uint8_t bc_supported(VkFormat fmt) {
    VkFormatProperties fp; vkGetPhysicalDeviceFormatProperties(g_vk.phys, fmt, &fp);
    const VkFormatFeatureFlags need =
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
    return (fp.optimalTilingFeatures & need) == need;
}

static void query_caps(RhiCaps *c) {
    VkPhysicalDeviceProperties props; vkGetPhysicalDeviceProperties(g_vk.phys, &props);
    VkPhysicalDeviceFeatures feats; vkGetPhysicalDeviceFeatures(g_vk.phys, &feats);
    const VkPhysicalDeviceLimits *lim = &props.limits;
    memset(c, 0, sizeof *c);
    c->backend = RHI_BACKEND_VULKAN;
    /* One 2D image dimension bounds both the target and a texture. */
    c->max_viewport[0] = c->max_viewport[1] = (int32_t)lim->maxImageDimension2D;
    c->max_texture_size = (int32_t)lim->maxImageDimension2D;
    c->tex_bc1 = bc_supported(VK_FORMAT_BC1_RGBA_UNORM_BLOCK);   /* DXT1 */
    c->tex_bc2 = bc_supported(VK_FORMAT_BC2_UNORM_BLOCK);        /* DXT3 */
    c->tex_bc3 = bc_supported(VK_FORMAT_BC3_UNORM_BLOCK);        /* DXT5 */
    c->wireframe = (uint8_t)feats.fillModeNonSolid;
    c->max_anisotropy = feats.samplerAnisotropy ? lim->maxSamplerAnisotropy : 1.0f;
    /* Timer queries need timestamps on the graphics queue family. */
    uint32_t qc = 0; vkGetPhysicalDeviceQueueFamilyProperties(g_vk.phys, &qc, NULL);
    VkQueueFamilyProperties *qp = calloc(qc ? qc : 1, sizeof *qp);
    vkGetPhysicalDeviceQueueFamilyProperties(g_vk.phys, &qc, qp);
    c->gpu_timer = (uint8_t)(lim->timestampComputeAndGraphics &&
                             g_vk.queue_family < qc && qp[g_vk.queue_family].timestampValidBits > 0);
    free(qp);
}

const RhiCaps *rhi_caps(void) {
    return &g_vk.caps;
}

/* -------------------------------------------------------------------------- */
/* Device lifetime                                                            */
/* -------------------------------------------------------------------------- */
RhiResult rhi_device_create(const RhiDeviceDesc *desc) {
    if (!desc || !desc->window || g_vk.device) return RHI_ERR_INVALID;
    if (desc->backend != RHI_BACKEND_DEFAULT && desc->backend != RHI_BACKEND_VULKAN)
        return RHI_ERR_UNSUPPORTED;

    g_vk.window = (SDL_Window *)desc->window;
    int w = 0, h = 0;
    SDL_GetWindowSize(g_vk.window, &w, &h);
    g_vk.width = w > 0 ? w : VK_DEFAULT_W;
    g_vk.height = h > 0 ? h : VK_DEFAULT_H;

    /* ---- instance: validation layer + debug messenger when present ---- */
    const char *layers[1]; uint32_t n_layers = 0;
    const char *exts[1]; uint32_t n_exts = 0;
    int have_debug = 0;
    {
        uint32_t lc = 0; vkEnumerateInstanceLayerProperties(&lc, NULL);
        VkLayerProperties *lp = calloc(lc ? lc : 1, sizeof *lp);
        vkEnumerateInstanceLayerProperties(&lc, lp);
        for (uint32_t i = 0; i < lc; i++)
            if (!strcmp(lp[i].layerName, "VK_LAYER_KHRONOS_validation")) layers[n_layers++] = "VK_LAYER_KHRONOS_validation";
        free(lp);
        if (n_layers) {   /* debug_utils rides with the layer */
            uint32_t ec = 0; vkEnumerateInstanceExtensionProperties(NULL, &ec, NULL);
            VkExtensionProperties *ep = calloc(ec ? ec : 1, sizeof *ep);
            vkEnumerateInstanceExtensionProperties(NULL, &ec, ep);
            for (uint32_t i = 0; i < ec; i++)
                if (!strcmp(ep[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) have_debug = 1;
            free(ep);
            if (have_debug) exts[n_exts++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        } else {
            fprintf(stderr, "rhi_vk: VK_LAYER_KHRONOS_validation not found (set VK_ADD_LAYER_PATH)\n");
        }
    }
    VkApplicationInfo app = { VK_STRUCTURE_TYPE_APPLICATION_INFO, NULL, "OpenUG2", 1, "OpenUG2", 1,
                              VK_API_VERSION_1_1 };
    VkDebugUtilsMessengerCreateInfoEXT dci = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT, NULL, 0,
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, dbg_cb, NULL };
    VkInstanceCreateInfo ici = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, have_debug ? &dci : NULL, 0, &app,
                                 n_layers, layers, n_exts, exts };
    if (!vk_ok(vkCreateInstance(&ici, NULL, &g_vk.instance), "vkCreateInstance")) goto fail;
    if (have_debug) {
        PFN_vkCreateDebugUtilsMessengerEXT cdm =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_vk.instance, "vkCreateDebugUtilsMessengerEXT");
        if (cdm && !vk_ok(cdm(g_vk.instance, &dci, NULL, &g_vk.messenger), "vkCreateDebugUtilsMessengerEXT")) goto fail;
    }

    /* ---- physical device + graphics queue family (no present: offscreen) ---- */
    uint32_t pdc = 0;
    if (!vk_ok(vkEnumeratePhysicalDevices(g_vk.instance, &pdc, NULL), "vkEnumeratePhysicalDevices") || !pdc) {
        fprintf(stderr, "rhi_vk: no Vulkan devices (check VK_DRIVER_FILES)\n"); goto fail;
    }
    VkPhysicalDevice *pds = calloc(pdc, sizeof *pds);
    vkEnumeratePhysicalDevices(g_vk.instance, &pdc, pds);
    for (uint32_t i = 0; i < pdc && !g_vk.phys; i++) {
        uint32_t qc = 0; vkGetPhysicalDeviceQueueFamilyProperties(pds[i], &qc, NULL);
        VkQueueFamilyProperties *qp = calloc(qc ? qc : 1, sizeof *qp);
        vkGetPhysicalDeviceQueueFamilyProperties(pds[i], &qc, qp);
        for (uint32_t q = 0; q < qc; q++)
            if (qp[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) { g_vk.phys = pds[i]; g_vk.queue_family = q; break; }
        free(qp);
    }
    free(pds);
    if (!g_vk.phys) { fprintf(stderr, "rhi_vk: no device with a graphics queue\n"); goto fail; }

    /* ---- logical device: enable only supported features we advertise ---- */
    VkPhysicalDeviceFeatures sup; vkGetPhysicalDeviceFeatures(g_vk.phys, &sup);
    VkPhysicalDeviceFeatures enable = {0};
    enable.textureCompressionBC = sup.textureCompressionBC;
    enable.samplerAnisotropy    = sup.samplerAnisotropy;
    enable.fillModeNonSolid     = sup.fillModeNonSolid;
    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, NULL, 0, g_vk.queue_family, 1, &prio };
    VkDeviceCreateInfo dc = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, NULL, 0, 1, &qci, 0, NULL, 0, NULL, &enable };
    if (!vk_ok(vkCreateDevice(g_vk.phys, &dc, NULL, &g_vk.device), "vkCreateDevice")) goto fail;
    vkGetDeviceQueue(g_vk.device, g_vk.queue_family, 0, &g_vk.queue);

    /* ---- command pool + one primary buffer + a frame fence ---- */
    VkCommandPoolCreateInfo cpci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, NULL,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, g_vk.queue_family };
    if (!vk_ok(vkCreateCommandPool(g_vk.device, &cpci, NULL, &g_vk.pool), "vkCreateCommandPool")) goto fail;
    VkCommandBufferAllocateInfo cbai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, NULL, g_vk.pool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
    if (!vk_ok(vkAllocateCommandBuffers(g_vk.device, &cbai, &g_vk.cmd), "vkAllocateCommandBuffers")) goto fail;
    VkFenceCreateInfo fci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, 0 };
    if (!vk_ok(vkCreateFence(g_vk.device, &fci, NULL, &g_vk.fence), "vkCreateFence")) goto fail;

    /* ---- offscreen main target: RGBA8 colour + D16 depth ----
     * The colour image needs TRANSFER_DST for vkCmdClearColorImage and
     * TRANSFER_SRC for the readback copy, besides COLOR_ATTACHMENT for draws
     * (P11e). The depth image needs TRANSFER_DST for vkCmdClearDepthStencilImage. */
    if (!make_image(VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    &g_vk.color, &g_vk.color_mem)) goto fail;
    g_vk.color_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (!make_image(VK_FORMAT_D16_UNORM,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    &g_vk.depth, &g_vk.depth_mem)) goto fail;

    /* ---- host-visible readback buffer, full-frame sized (reused each read) ---- */
    VkDeviceSize bsize = (VkDeviceSize)g_vk.width * g_vk.height * 4;
    VkBufferCreateInfo bci = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, NULL, 0, bsize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, 0, NULL };
    if (!vk_ok(vkCreateBuffer(g_vk.device, &bci, NULL, &g_vk.readback), "vkCreateBuffer")) goto fail;
    VkMemoryRequirements bmr; vkGetBufferMemoryRequirements(g_vk.device, g_vk.readback, &bmr);
    uint32_t bmt = find_mem(bmr.memoryTypeBits,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (bmt == UINT32_MAX) { fprintf(stderr, "rhi_vk: no host-visible memory type\n"); goto fail; }
    VkMemoryAllocateInfo bmai = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, bmr.size, bmt };
    if (!vk_ok(vkAllocateMemory(g_vk.device, &bmai, NULL, &g_vk.readback_mem), "vkAllocateMemory(buffer)")) goto fail;
    if (!vk_ok(vkBindBufferMemory(g_vk.device, g_vk.readback, g_vk.readback_mem, 0), "vkBindBufferMemory")) goto fail;

    g_vk.frame_index = 0;
    query_caps(&g_vk.caps);
    return RHI_OK;

fail:
    rhi_device_destroy();
    return RHI_ERR_UNSUPPORTED;
}

void rhi_device_destroy(void) {
    if (g_vk.device) vkDeviceWaitIdle(g_vk.device);
    /* Free every live buffer, then the handle table's own heap arrays. */
    for (uint32_t i = 0; i < g_vk.buffers.count; i++) {
        RhiVkBuffer *b = &g_vk.buffers.slots[i];
        if (b->mapped) vkUnmapMemory(g_vk.device, b->mem);
        if (b->buf) vkDestroyBuffer(g_vk.device, b->buf, NULL);
        if (b->mem) vkFreeMemory(g_vk.device, b->mem, NULL);
    }
    free(g_vk.buffers.slots);
    free(g_vk.buffers.free_list);
    /* Free every live texture (view/image/memory), then its table arrays. */
    for (uint32_t i = 0; i < g_vk.textures.count; i++) {
        RhiVkTexture *t = &g_vk.textures.slots[i];
        if (t->view) vkDestroyImageView(g_vk.device, t->view, NULL);
        if (t->img)  vkDestroyImage(g_vk.device, t->img, NULL);
        if (t->mem)  vkFreeMemory(g_vk.device, t->mem, NULL);
    }
    free(g_vk.textures.slots);
    free(g_vk.textures.free_list);
    /* Destroy every VkSampler, then the sampler list arrays. */
    for (uint32_t i = 0; i < g_vk.samplers.count; i++)
        if (g_vk.samplers.handles[i]) vkDestroySampler(g_vk.device, g_vk.samplers.handles[i], NULL);
    free(g_vk.samplers.descs);
    free(g_vk.samplers.handles);
    /* Free every live shader (modules + param UBOs), then its table arrays. */
    for (uint32_t i = 0; i < g_vk.shaders.count; i++) {
        RhiVkShader *s = &g_vk.shaders.slots[i];
        for (uint32_t b = 0; b < s->block_count; b++) {
            if (s->blocks[b].mapped)  vkUnmapMemory(g_vk.device, s->blocks[b].ubo_mem);
            if (s->blocks[b].ubo)     vkDestroyBuffer(g_vk.device, s->blocks[b].ubo, NULL);
            if (s->blocks[b].ubo_mem) vkFreeMemory(g_vk.device, s->blocks[b].ubo_mem, NULL);
        }
        if (s->vert) vkDestroyShaderModule(g_vk.device, s->vert, NULL);
        if (s->frag) vkDestroyShaderModule(g_vk.device, s->frag, NULL);
    }
    free(g_vk.shaders.slots);
    free(g_vk.shaders.free_list);
    if (g_vk.readback)     vkDestroyBuffer(g_vk.device, g_vk.readback, NULL);
    if (g_vk.readback_mem) vkFreeMemory(g_vk.device, g_vk.readback_mem, NULL);
    if (g_vk.depth)        vkDestroyImage(g_vk.device, g_vk.depth, NULL);
    if (g_vk.depth_mem)    vkFreeMemory(g_vk.device, g_vk.depth_mem, NULL);
    if (g_vk.color)        vkDestroyImage(g_vk.device, g_vk.color, NULL);
    if (g_vk.color_mem)    vkFreeMemory(g_vk.device, g_vk.color_mem, NULL);
    if (g_vk.fence)        vkDestroyFence(g_vk.device, g_vk.fence, NULL);
    if (g_vk.pool)         vkDestroyCommandPool(g_vk.device, g_vk.pool, NULL);
    if (g_vk.device)       vkDestroyDevice(g_vk.device, NULL);
    if (g_vk.messenger) {
        PFN_vkDestroyDebugUtilsMessengerEXT ddm =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_vk.instance, "vkDestroyDebugUtilsMessengerEXT");
        if (ddm) ddm(g_vk.instance, g_vk.messenger, NULL);
    }
    if (g_vk.instance) vkDestroyInstance(g_vk.instance, NULL);
    memset(&g_vk, 0, sizeof g_vk);
}

void rhi_drawable_size(int32_t *width, int32_t *height) {
    if (width)  *width  = g_vk.width;
    if (height) *height = g_vk.height;
}

/* -------------------------------------------------------------------------- */
/* Frame lifecycle                                                            */
/* -------------------------------------------------------------------------- */
RhiResult rhi_frame_begin(RhiFrameInfo *out) {
    if (!g_vk.device) return RHI_ERR_INVALID;
    if (!ensure_recording()) return RHI_ERR_DEVICE_LOST;
    g_vk.in_frame = 1;
    if (out) {
        out->width = g_vk.width;
        out->height = g_vk.height;
        out->frame_index = g_vk.frame_index;
    }
    return RHI_OK;
}

RhiResult rhi_frame_end(void) {
    if (!g_vk.device || !g_vk.in_frame) return RHI_ERR_INVALID;
    if (!flush()) return RHI_ERR_DEVICE_LOST;   /* submit + fence wait (no present) */
    g_vk.in_frame = 0;
    g_vk.frame_index++;
    return RHI_OK;
}

/* -------------------------------------------------------------------------- */
/* Clear                                                                      */
/* -------------------------------------------------------------------------- */
/* Clears the whole main target with transfer clears; contents are discarded
 * (oldLayout UNDEFINED), matching rhi_cmd_clear_main's "clears the whole target"
 * contract. The colour image is left in TRANSFER_SRC_OPTIMAL ready for readback. */
RhiResult rhi_cmd_clear_main(const float color[4], float depth) {
    if (!color || !g_vk.in_frame) return RHI_ERR_INVALID;
    if (!ensure_recording()) return RHI_ERR_DEVICE_LOST;
    VkImageSubresourceRange crange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    VkImageSubresourceRange drange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

    /* Wait on any prior transfer to these images this frame (an earlier clear,
       a read_pixels or a copy_main_to_texture): source stage TRANSFER with
       TRANSFER_WRITE|READ, not TOP_OF_PIPE, so a second clear is hazard-free.
       oldLayout stays UNDEFINED since the clear discards the old contents. */
    image_barrier(g_vk.color, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkClearColorValue cc; memcpy(cc.float32, color, sizeof cc.float32);
    vkCmdClearColorImage(g_vk.cmd, g_vk.color, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &cc, 1, &crange);
    /* Settle to TRANSFER_SRC so a later read_pixels copy needs no extra barrier. */
    image_barrier(g_vk.color, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
                  VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    g_vk.color_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    image_barrier(g_vk.depth, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkClearDepthStencilValue cd = { depth, 0 };
    vkCmdClearDepthStencilImage(g_vk.cmd, g_vk.depth, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &cd, 1, &drange);
    return RHI_OK;
}

/* -------------------------------------------------------------------------- */
/* Readback                                                                   */
/* -------------------------------------------------------------------------- */
/* Copies a w x h rectangle of the offscreen colour image into the host buffer
 * and converts it into `dst`, rows top-down. The image is treated as having a
 * GL-style bottom-left origin (the renderer's draws will target it that way in
 * P11e), so, exactly like rhi_gl.c's glReadPixels path, the sampled image rows
 * run y = frame_h - y - h upward and are flipped into dst to give top-down rows.
 * Blocks until the copy has finished (submit + fence wait). */
RhiResult rhi_read_pixels(int32_t x, int32_t y, int32_t w, int32_t h,
                          RhiReadFormat format, void *dst, size_t stride) {
    size_t bpp = format == RHI_READ_RGB8 ? 3 : format == RHI_READ_RGBA8 ? 4 : 0;
    if (!g_vk.in_frame || !dst || !bpp || w <= 0 || h <= 0 || x < 0 || y < 0 ||
        w > g_vk.width - x || h > g_vk.height - y) return RHI_ERR_INVALID;
    size_t limit = (size_t)PTRDIFF_MAX;
    if ((size_t)w > limit / 4 / (size_t)h) return RHI_ERR_INVALID;
    size_t row_bytes = (size_t)w * bpp;
    if (!stride) stride = row_bytes;
    if (stride < row_bytes || (size_t)(h - 1) > (limit - row_bytes) / stride) return RHI_ERR_INVALID;
    if (!ensure_recording()) return RHI_ERR_DEVICE_LOST;

    /* The colour image is in TRANSFER_SRC after a clear; guard in case it is not. */
    if (g_vk.color_layout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        image_barrier(g_vk.color, VK_IMAGE_ASPECT_COLOR_BIT, g_vk.color_layout,
                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, VK_ACCESS_TRANSFER_READ_BIT,
                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        g_vk.color_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    }
    /* Copy RGBA8 rows tightly packed into the readback buffer; bottom-left origin. */
    VkBufferImageCopy reg = { 0, 0, 0, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        { x, g_vk.height - y - h, 0 }, { (uint32_t)w, (uint32_t)h, 1 } };
    vkCmdCopyImageToBuffer(g_vk.cmd, g_vk.color, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, g_vk.readback, 1, &reg);
    VkBufferMemoryBarrier bmb = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, NULL, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_HOST_READ_BIT, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, g_vk.readback, 0, VK_WHOLE_SIZE };
    vkCmdPipelineBarrier(g_vk.cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, NULL,
                         1, &bmb, 0, NULL);
    if (!flush()) return RHI_ERR_DEVICE_LOST;

    unsigned char *map = NULL;
    if (!vk_ok(vkMapMemory(g_vk.device, g_vk.readback_mem, 0, VK_WHOLE_SIZE, 0, (void **)&map), "vkMapMemory"))
        return RHI_ERR_DEVICE_LOST;
    /* RGBA8 storage, R at byte 0 (like GL's GL_RGBA read); flip rows to top-down. */
    for (int32_t row = 0; row < h; row++) {
        const unsigned char *src = map + (size_t)(h - 1 - row) * w * 4;
        unsigned char *o = (unsigned char *)dst + (size_t)row * stride;
        if (bpp == 4) { memcpy(o, src, (size_t)w * 4); continue; }
        for (int32_t i = 0; i < w; i++) { o[i*3] = src[i*4]; o[i*3+1] = src[i*4+1]; o[i*3+2] = src[i*4+2]; }
    }
    vkUnmapMemory(g_vk.device, g_vk.readback_mem);
    return RHI_OK;
}

/* ========================================================================== */
/* Stubs for the rest of the rhi.h surface (filled in by P11b..P11g).         */
/* Handle-returning: {0}; RhiResult: RHI_ERR_UNSUPPORTED; void: no-op.        */
/* ========================================================================== */

/* --- Buffers (P11b) ---
 * The handle table mirrors the GL backend's (slot + generation, free list);
 * STATIC buffers are device-local and filled through a staging copy, STREAM
 * buffers are host-visible + coherent and persistently mapped. */

/* Resolve a handle to its live slot, logging a bounded number of stale hits
 * exactly like rhi_gl.c's buffer_lookup. */
static RhiVkBuffer *buffer_lookup(RhiBuffer buf, const char *what) {
    uint32_t slot = buf.id & VK_SLOT_MASK;
    if (!buf.id) return NULL;
    if (slot && slot <= g_vk.buffers.count) {
        RhiVkBuffer *b = &g_vk.buffers.slots[slot - 1];
        if (b->live && b->generation == (uint8_t)(buf.id >> VK_SLOT_BITS)) return b;
    }
    if (g_vk.buffers.stale_logged < 8) {
        g_vk.buffers.stale_logged++;
        fprintf(stderr, "rhi: %s: stale or invalid buffer handle 0x%08x\n", what, (unsigned)buf.id);
    }
    return NULL;
}

static int buffer_size_ok(size_t size) {
    return size <= (size_t)PTRDIFF_MAX;
}

static uint32_t buffer_slot_alloc(void) {
    if (g_vk.buffers.free_count) return g_vk.buffers.free_list[--g_vk.buffers.free_count];
    if (g_vk.buffers.count == g_vk.buffers.capacity) {
        uint32_t cap = g_vk.buffers.capacity ? g_vk.buffers.capacity * 2u : 256u;
        if (cap > VK_SLOT_MASK) return 0;
        RhiVkBuffer *slots = (RhiVkBuffer *)realloc(g_vk.buffers.slots, cap * sizeof *slots);
        if (!slots) return 0;
        g_vk.buffers.slots = slots;
        uint32_t *list = (uint32_t *)realloc(g_vk.buffers.free_list, cap * sizeof *list);
        if (!list) return 0;
        g_vk.buffers.free_list = list;
        g_vk.buffers.capacity = cap;
    }
    memset(&g_vk.buffers.slots[g_vk.buffers.count], 0, sizeof g_vk.buffers.slots[0]);
    return ++g_vk.buffers.count;
}

/* Give `b` fresh storage of `size` bytes (any previous storage must already be
 * released) and optionally fill it from `data`. size 0 leaves the slot empty. */
static RhiResult buffer_alloc_storage(RhiVkBuffer *b, size_t size, const void *data) {
    b->buf = VK_NULL_HANDLE; b->mem = VK_NULL_HANDLE; b->mapped = NULL; b->size = 0;
    if (!size) return RHI_OK;
    if (b->update == RHI_UPDATE_STREAM) {
        if (!make_buffer(size, b->usage,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &b->buf, &b->mem))
            return RHI_ERR_DEVICE_LOST;
        if (!vk_ok(vkMapMemory(g_vk.device, b->mem, 0, VK_WHOLE_SIZE, 0, &b->mapped), "vkMapMemory(stream)")) {
            vkDestroyBuffer(g_vk.device, b->buf, NULL); vkFreeMemory(g_vk.device, b->mem, NULL);
            b->buf = VK_NULL_HANDLE; b->mem = VK_NULL_HANDLE; b->mapped = NULL;
            return RHI_ERR_DEVICE_LOST;
        }
        if (data) memcpy(b->mapped, data, size);
    } else {
        if (!make_buffer(size, b->usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &b->buf, &b->mem))
            return RHI_ERR_DEVICE_LOST;
        if (data && !stage_to_device(b->buf, 0, data, size)) {
            vkDestroyBuffer(g_vk.device, b->buf, NULL); vkFreeMemory(g_vk.device, b->mem, NULL);
            b->buf = VK_NULL_HANDLE; b->mem = VK_NULL_HANDLE;
            return RHI_ERR_DEVICE_LOST;
        }
    }
    b->size = size;
    return RHI_OK;
}

RhiBuffer rhi_buffer_create(const RhiBufferDesc *desc) {
    RhiBuffer out = {0};
    if (!desc || (desc->usage != RHI_BUFFER_VERTEX && desc->usage != RHI_BUFFER_INDEX) ||
        (desc->update != RHI_UPDATE_STATIC && desc->update != RHI_UPDATE_STREAM) ||
        !buffer_size_ok(desc->size)) {
        fprintf(stderr, "rhi: buffer_create: invalid description\n");
        return out;
    }
    if (!g_vk.device) { fprintf(stderr, "rhi: buffer_create: no device\n"); return out; }
    uint32_t slot = buffer_slot_alloc();
    if (!slot) { fprintf(stderr, "rhi: buffer_create: out of handles\n"); return out; }
    RhiVkBuffer *b = &g_vk.buffers.slots[slot - 1];
    b->update = (uint8_t)desc->update;
    b->usage  = desc->usage == RHI_BUFFER_INDEX ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                                                : VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    b->buf = VK_NULL_HANDLE; b->mem = VK_NULL_HANDLE; b->mapped = NULL; b->size = 0;
    b->live = 1;
    out.id = ((uint32_t)b->generation << VK_SLOT_BITS) | slot;
    /* A STREAM buffer with no size/data gets its storage from the first replace. */
    if (desc->update == RHI_UPDATE_STATIC || desc->size || desc->data) {
        if (buffer_alloc_storage(b, desc->size, desc->data) != RHI_OK) {
            fprintf(stderr, "rhi: buffer_create: %zu bytes could not be allocated\n", desc->size);
            rhi_buffer_destroy(out);
            return (RhiBuffer){0};
        }
    }
    return out;
}

RhiResult rhi_buffer_update(RhiBuffer buf, size_t offset, size_t size, const void *data) {
    RhiVkBuffer *b = buffer_lookup(buf, "buffer_update");
    if (!b || !data || offset > b->size || size > b->size - offset) return RHI_ERR_INVALID;
    if (!size) return RHI_OK;
    if (b->update == RHI_UPDATE_STREAM) {
        memcpy((unsigned char *)b->mapped + offset, data, size);
        return RHI_OK;
    }
    return stage_to_device(b->buf, offset, data, size) ? RHI_OK : RHI_ERR_DEVICE_LOST;
}

RhiResult rhi_buffer_replace(RhiBuffer buf, size_t size, const void *data) {
    RhiVkBuffer *b = buffer_lookup(buf, "buffer_replace");
    if (!b || !buffer_size_ok(size)) return RHI_ERR_INVALID;
    /* Fresh storage (orphan/realloc). No draws reference it yet (pre-P11e) and
     * every copy above blocks, so releasing the old storage now is safe. */
    if (b->mapped) { vkUnmapMemory(g_vk.device, b->mem); b->mapped = NULL; }
    if (b->buf) vkDestroyBuffer(g_vk.device, b->buf, NULL);
    if (b->mem) vkFreeMemory(g_vk.device, b->mem, NULL);
    return buffer_alloc_storage(b, size, data);
}

void rhi_buffer_destroy(RhiBuffer buf) {
    RhiVkBuffer *b = buffer_lookup(buf, "buffer_destroy");
    if (!b) return;
    if (b->mapped) { vkUnmapMemory(g_vk.device, b->mem); b->mapped = NULL; }
    if (b->buf) vkDestroyBuffer(g_vk.device, b->buf, NULL);
    if (b->mem) vkFreeMemory(g_vk.device, b->mem, NULL);
    b->buf = VK_NULL_HANDLE; b->mem = VK_NULL_HANDLE; b->size = 0; b->live = 0;
    /* Retire the slot before its generation wraps and revives stale handles. */
    if (b->generation == UINT8_MAX) return;
    b->generation++;
    g_vk.buffers.free_list[g_vk.buffers.free_count++] = buf.id & VK_SLOT_MASK;
}

/* Test-only (not in rhi.h): read `size` bytes at `offset` of `buf` into `dst`.
 * STREAM reads the persistent mapping; STATIC copies device-local -> a host
 * staging buffer with vkCmdCopyBuffer. The smoke declares it extern to verify
 * buffer contents, since rhi.h exposes no buffer readback. 1 ok, 0 on failure. */
int rhi_vk_buffer_readback(RhiBuffer buf, size_t offset, size_t size, void *dst) {
    RhiVkBuffer *b = buffer_lookup(buf, "buffer_readback");
    if (!b || !dst || offset > b->size || size > b->size - offset) return 0;
    if (!size) return 1;
    if (b->update == RHI_UPDATE_STREAM) {
        memcpy(dst, (const unsigned char *)b->mapped + offset, size);
        return 1;
    }
    VkBuffer staging; VkDeviceMemory smem;
    if (!make_buffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &staging, &smem)) return 0;
    int ok = copy_buffer(b->buf, offset, staging, 0, size);
    if (ok) {
        void *map = NULL;
        ok = vk_ok(vkMapMemory(g_vk.device, smem, 0, VK_WHOLE_SIZE, 0, &map), "vkMapMemory(readback)");
        if (ok) { memcpy(dst, map, size); vkUnmapMemory(g_vk.device, smem); }
    }
    vkDestroyBuffer(g_vk.device, staging, NULL);
    vkFreeMemory(g_vk.device, smem, NULL);
    return ok;
}

/* --- Textures (P11c) ---
 * The handle table mirrors the buffer table. A texture is a VkImage with
 * device-local memory and a default sampling view. Uploads go through a
 * host-visible staging buffer and a one-shot vkCmdCopyBufferToImage; mip
 * generation blits the chain down. Every transfer runs on its own fenced
 * command buffer (like copy_buffer/stage_to_device), so texture work is safe
 * whether or not a frame is open. RGB8 is stored as RGBA8 (A expanded to 255
 * on upload); BC1/2/3 store the compressed blocks verbatim. */

static const RhiSamplerDesc k_default_sampler = {
    RHI_FILTER_NEAREST, RHI_FILTER_LINEAR, RHI_MIP_LINEAR, RHI_WRAP_REPEAT, RHI_WRAP_REPEAT, 1.0f
};

static int format_compressed(RhiFormat f) {
    return f == RHI_FORMAT_BC1 || f == RHI_FORMAT_BC2 || f == RHI_FORMAT_BC3;
}

/* Bytes per 4x4 block for the BC formats (0 if not block-compressed). */
static uint32_t format_block_bytes(RhiFormat f) {
    return f == RHI_FORMAT_BC1 ? 8u : (f == RHI_FORMAT_BC2 || f == RHI_FORMAT_BC3) ? 16u : 0u;
}

/* Where a texture rests after an upload / mip generation. SHADER_READ_ONLY only
 * when it is sampled (that layout requires SAMPLED/INPUT_ATTACHMENT usage);
 * otherwise GENERAL, which every usage accepts and the next use re-barriers. */
static VkImageLayout rest_layout(uint32_t usage) {
    return (usage & RHI_TEXTURE_SAMPLED) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                         : VK_IMAGE_LAYOUT_GENERAL;
}

/* full mip chain length for a w x h image: floor(log2(max(w,h))) + 1 */
static uint32_t mip_count(int32_t w, int32_t h) {
    uint32_t m = (uint32_t)(w > h ? w : h), n = 1;
    while (m > 1) { m >>= 1u; n++; }
    return n;
}

/* Record a layout transition on `cb` over an explicit subresource range. */
static void img_barrier(VkCommandBuffer cb, VkImage img, VkImageAspectFlags aspect,
                        uint32_t base_lvl, uint32_t n_lvl, uint32_t base_layer, uint32_t n_layer,
                        VkImageLayout old, VkImageLayout neu, VkAccessFlags srcA, VkAccessFlags dstA,
                        VkPipelineStageFlags srcS, VkPipelineStageFlags dstS) {
    VkImageMemoryBarrier b = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, NULL, srcA, dstA, old, neu,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, img,
        { aspect, base_lvl, n_lvl, base_layer, n_layer } };
    vkCmdPipelineBarrier(cb, srcS, dstS, 0, 0, NULL, 0, NULL, 1, &b);
}

/* Allocate + begin a one-shot primary command buffer; VK_NULL_HANDLE on error. */
static VkCommandBuffer oneshot_begin(void) {
    VkCommandBuffer cb = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo cbai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, NULL, g_vk.pool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
    if (!vk_ok(vkAllocateCommandBuffers(g_vk.device, &cbai, &cb), "vkAllocateCommandBuffers(oneshot)"))
        return VK_NULL_HANDLE;
    VkCommandBufferBeginInfo bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, NULL };
    if (!vk_ok(vkBeginCommandBuffer(cb, &bi), "vkBeginCommandBuffer(oneshot)")) {
        vkFreeCommandBuffers(g_vk.device, g_vk.pool, 1, &cb);
        return VK_NULL_HANDLE;
    }
    return cb;
}

/* End, submit and block on `cb`, then free it. 1 on success. */
static int oneshot_end(VkCommandBuffer cb) {
    int ok = vk_ok(vkEndCommandBuffer(cb), "vkEndCommandBuffer(oneshot)");
    VkFence fence = VK_NULL_HANDLE;
    if (ok) {
        VkFenceCreateInfo fci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, 0 };
        ok = vk_ok(vkCreateFence(g_vk.device, &fci, NULL, &fence), "vkCreateFence(oneshot)");
    }
    if (ok) {
        VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, NULL, 1, &cb, 0, NULL };
        ok = vk_ok(vkQueueSubmit(g_vk.queue, 1, &si, fence), "vkQueueSubmit(oneshot)") &&
             vk_ok(vkWaitForFences(g_vk.device, 1, &fence, VK_TRUE, UINT64_MAX), "vkWaitForFences(oneshot)");
    }
    if (fence) vkDestroyFence(g_vk.device, fence, NULL);
    vkFreeCommandBuffers(g_vk.device, g_vk.pool, 1, &cb);
    return ok;
}

static VkFormat texture_vkformat(RhiFormat f) {
    switch (f) {
    case RHI_FORMAT_RGB8:
    case RHI_FORMAT_RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;   /* RGB8 stored as RGBA8 */
    case RHI_FORMAT_BC1:   return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case RHI_FORMAT_BC2:   return VK_FORMAT_BC2_UNORM_BLOCK;
    case RHI_FORMAT_BC3:   return VK_FORMAT_BC3_UNORM_BLOCK;
    case RHI_FORMAT_D16:
    case RHI_FORMAT_DEPTH: return VK_FORMAT_D16_UNORM;        /* main depth precision */
    default:               return VK_FORMAT_UNDEFINED;
    }
}

static RhiVkTexture *texture_find(RhiTexture tex) {
    uint32_t slot = tex.id & VK_SLOT_MASK;
    if (!tex.id) return NULL;
    if (slot && slot <= g_vk.textures.count) {
        RhiVkTexture *t = &g_vk.textures.slots[slot - 1];
        if (t->live && t->generation == (uint8_t)(tex.id >> VK_SLOT_BITS)) return t;
    }
    return NULL;
}

static RhiVkTexture *texture_lookup(RhiTexture tex, const char *what) {
    RhiVkTexture *t = texture_find(tex);
    if (!t && tex.id && g_vk.textures.stale_logged < 8) {
        g_vk.textures.stale_logged++;
        fprintf(stderr, "rhi: %s: stale or invalid texture handle 0x%08x\n", what, (unsigned)tex.id);
    }
    return t;
}

static uint32_t texture_slot_alloc(void) {
    if (g_vk.textures.free_count) return g_vk.textures.free_list[--g_vk.textures.free_count];
    if (g_vk.textures.count == g_vk.textures.capacity) {
        uint32_t cap = g_vk.textures.capacity ? g_vk.textures.capacity * 2u : 256u;
        if (cap > VK_SLOT_MASK) return 0;
        RhiVkTexture *slots = (RhiVkTexture *)realloc(g_vk.textures.slots, cap * sizeof *slots);
        if (!slots) return 0;
        g_vk.textures.slots = slots;
        uint32_t *list = (uint32_t *)realloc(g_vk.textures.free_list, cap * sizeof *list);
        if (!list) return 0;
        g_vk.textures.free_list = list;
        g_vk.textures.capacity = cap;
    }
    memset(&g_vk.textures.slots[g_vk.textures.count], 0, sizeof g_vk.textures.slots[0]);
    return ++g_vk.textures.count;
}

RhiTexture rhi_texture_create(const RhiTextureDesc *desc) {
    RhiTexture out = {0};
    int depth16 = desc && desc->format == RHI_FORMAT_D16;
    /* Validation mirrors the GL backend exactly (see rhi_texture_create there). */
    if (!desc || (desc->type != RHI_TEXTURE_2D && desc->type != RHI_TEXTURE_CUBE) ||
        desc->format < RHI_FORMAT_RGB8 || desc->format > RHI_FORMAT_DEPTH ||
        desc->width <= 0 || desc->height <= 0 || !desc->usage ||
        (desc->usage & ~(uint32_t)(RHI_TEXTURE_SAMPLED | RHI_TEXTURE_COLOR_TARGET |
                                   RHI_TEXTURE_DEPTH_TARGET | RHI_TEXTURE_COPY_DST)) ||
        (desc->format == RHI_FORMAT_DEPTH &&
         (desc->type != RHI_TEXTURE_2D || desc->usage != (RHI_TEXTURE_SAMPLED | RHI_TEXTURE_COPY_DST))) ||
        (depth16 && (desc->type != RHI_TEXTURE_2D || desc->usage != RHI_TEXTURE_DEPTH_TARGET)) ||
        (!depth16 && (desc->usage & RHI_TEXTURE_DEPTH_TARGET)) ||
        (desc->type == RHI_TEXTURE_CUBE && desc->width != desc->height)) {
        fprintf(stderr, "rhi: texture_create: invalid description\n");
        return out;
    }
    if (!g_vk.device) { fprintf(stderr, "rhi: texture_create: no device\n"); return out; }

    int is_depth = depth16 || desc->format == RHI_FORMAT_DEPTH;
    VkFormat vkfmt = texture_vkformat(desc->format);
    uint32_t layers = desc->type == RHI_TEXTURE_CUBE ? 6u : 1u;
    /* Depth images are single-level; colour images get a full mip chain so they
     * can receive uploads up to level floor(log2(max(w,h))) (matches GL). */
    uint32_t levels = is_depth ? 1u : mip_count(desc->width, desc->height);

    VkImageUsageFlags vku = 0;
    if (desc->usage & RHI_TEXTURE_SAMPLED)       vku |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (desc->usage & RHI_TEXTURE_COLOR_TARGET)  vku |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (desc->usage & RHI_TEXTURE_DEPTH_TARGET)  vku |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (desc->usage & RHI_TEXTURE_COPY_DST)      vku |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    /* Colour textures are uploadable and blittable (mips) and readable by the
     * test path, so they always carry both transfer bits. */
    if (!is_depth) vku |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    uint32_t slot = texture_slot_alloc();
    if (!slot) { fprintf(stderr, "rhi: texture_create: out of handles\n"); return out; }
    RhiVkTexture *t = &g_vk.textures.slots[slot - 1];
    t->img = VK_NULL_HANDLE; t->mem = VK_NULL_HANDLE; t->view = VK_NULL_HANDLE;
    t->vkfmt = vkfmt;
    t->aspect = is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    t->layout = VK_IMAGE_LAYOUT_UNDEFINED;
    t->format = desc->format;
    t->usage = desc->usage;
    t->width = desc->width;
    t->height = desc->height;
    t->layers = layers;
    t->mip_levels = levels;
    t->live = 1;
    out.id = ((uint32_t)t->generation << VK_SLOT_BITS) | slot;

    VkImageCreateInfo ici = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, NULL,
        desc->type == RHI_TEXTURE_CUBE ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0u,
        VK_IMAGE_TYPE_2D, vkfmt, { (uint32_t)desc->width, (uint32_t)desc->height, 1 }, levels, layers,
        VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, vku, VK_SHARING_MODE_EXCLUSIVE, 0, NULL,
        VK_IMAGE_LAYOUT_UNDEFINED };
    if (!vk_ok(vkCreateImage(g_vk.device, &ici, NULL, &t->img), "vkCreateImage(texture)")) goto fail;
    VkMemoryRequirements mr; vkGetImageMemoryRequirements(g_vk.device, t->img, &mr);
    uint32_t mt = find_mem(mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mt == UINT32_MAX) { fprintf(stderr, "rhi_vk: no device-local memory type\n"); goto fail; }
    VkMemoryAllocateInfo ai = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, mr.size, mt };
    if (!vk_ok(vkAllocateMemory(g_vk.device, &ai, NULL, &t->mem), "vkAllocateMemory(texture)")) goto fail;
    if (!vk_ok(vkBindImageMemory(g_vk.device, t->img, t->mem, 0), "vkBindImageMemory(texture)")) goto fail;

    /* A view needs a view-compatible usage (sampled / attachment). A transfer-
     * only COPY_DST texture has none, so it gets no view (nor is it ever sampled). */
    if (desc->usage & (RHI_TEXTURE_SAMPLED | RHI_TEXTURE_COLOR_TARGET | RHI_TEXTURE_DEPTH_TARGET)) {
        VkImageViewCreateInfo vci = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, NULL, 0, t->img,
            desc->type == RHI_TEXTURE_CUBE ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D, vkfmt,
            { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
              VK_COMPONENT_SWIZZLE_IDENTITY }, { t->aspect, 0, levels, 0, layers } };
        if (!vk_ok(vkCreateImageView(g_vk.device, &vci, NULL, &t->view), "vkCreateImageView(texture)")) goto fail;
    }

    /* A fresh texture starts with the documented default sampler. */
    t->sampler = rhi_sampler_create(&k_default_sampler);
    if (!t->sampler.id) goto fail;
    return out;

fail:
    rhi_texture_destroy(out);
    return (RhiTexture){0};
}

RhiResult rhi_texture_upload(RhiTexture tex, int32_t face, int32_t level, const void *data, size_t size) {
    RhiVkTexture *t = texture_lookup(tex, "texture_upload");
    int faces = t && t->layers == 6 ? 6 : 1;
    if (!t || t->format == RHI_FORMAT_D16 || t->format == RHI_FORMAT_DEPTH ||
        face < 0 || face >= faces || level < 0 || (uint32_t)level >= t->mip_levels)
        return RHI_ERR_INVALID;
    int compressed = format_compressed(t->format);
    if (compressed && (!data || !size || size > 0x7fffffff)) return RHI_ERR_INVALID;
    /* Uncompressed with no data: the level is already allocated; undefined is fine. */
    if (!compressed && !data) return RHI_OK;

    int32_t w = t->width >> level, h = t->height >> level;
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    /* Compressed: the copy reads the mip's full block footprint, so `size` must
       cover it, else vkCmdCopyBufferToImage reads past the staging buffer. */
    if (compressed) {
        uint32_t blocks = (uint32_t)((w + 3) / 4) * (uint32_t)((h + 3) / 4);
        if (size < (size_t)blocks * format_block_bytes(t->format)) return RHI_ERR_INVALID;
    }

    /* Build the staging contents: BC blocks verbatim, RGBA8 verbatim, RGB8
     * expanded to RGBA8 with A = 255. */
    VkDeviceSize staging_size = compressed ? (VkDeviceSize)size : (VkDeviceSize)w * h * 4;
    VkBuffer staging; VkDeviceMemory smem;
    if (!make_buffer(staging_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &staging, &smem)) return RHI_ERR_DEVICE_LOST;
    void *map = NULL;
    int ok = vk_ok(vkMapMemory(g_vk.device, smem, 0, VK_WHOLE_SIZE, 0, &map), "vkMapMemory(tex staging)");
    if (ok) {
        if (compressed) {
            memcpy(map, data, size);
        } else if (t->format == RHI_FORMAT_RGB8) {
            const unsigned char *s = (const unsigned char *)data;
            unsigned char *d = (unsigned char *)map;
            for (int32_t i = 0; i < w * h; i++) {
                d[i*4] = s[i*3]; d[i*4+1] = s[i*3+1]; d[i*4+2] = s[i*3+2]; d[i*4+3] = 255;
            }
        } else {
            memcpy(map, data, (size_t)w * h * 4);
        }
        vkUnmapMemory(g_vk.device, smem);
    }
    VkCommandBuffer cb = ok ? oneshot_begin() : VK_NULL_HANDLE;
    if (cb) {
        /* Whole image current -> TRANSFER_DST (preserves other levels unless it
         * was UNDEFINED), copy one subresource, then whole image -> SHADER_READ. */
        img_barrier(cb, t->img, t->aspect, 0, t->mip_levels, 0, t->layers,
                    t->layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkBufferImageCopy reg = { 0, 0, 0, { t->aspect, (uint32_t)level, (uint32_t)face, 1 },
            { 0, 0, 0 }, { (uint32_t)w, (uint32_t)h, 1 } };
        vkCmdCopyBufferToImage(cb, staging, t->img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &reg);
        VkImageLayout rl = rest_layout(t->usage);
        int sampled = rl == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        img_barrier(cb, t->img, t->aspect, 0, t->mip_levels, 0, t->layers,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, rl,
                    VK_ACCESS_TRANSFER_WRITE_BIT, sampled ? VK_ACCESS_SHADER_READ_BIT : 0,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    sampled ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        ok = oneshot_end(cb);
        if (ok) t->layout = rl;
    } else {
        ok = 0;
    }
    vkDestroyBuffer(g_vk.device, staging, NULL);
    vkFreeMemory(g_vk.device, smem, NULL);
    return ok ? RHI_OK : RHI_ERR_DEVICE_LOST;
}

RhiResult rhi_texture_generate_mips(RhiTexture tex) {
    RhiVkTexture *t = texture_lookup(tex, "texture_generate_mips");
    if (!t || t->layers != 1 || (t->format != RHI_FORMAT_RGB8 && t->format != RHI_FORMAT_RGBA8))
        return RHI_ERR_INVALID;
    if (t->mip_levels < 2) return RHI_OK;   /* nothing to generate */
    VkCommandBuffer cb = oneshot_begin();
    if (!cb) return RHI_ERR_DEVICE_LOST;
    /* Level 0 -> TRANSFER_SRC, then blit each level from the one above with a
     * LINEAR filter and the standard per-level barriers. */
    img_barrier(cb, t->img, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1,
                t->layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, VK_ACCESS_TRANSFER_READ_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    int32_t sw = t->width, sh = t->height;
    for (uint32_t i = 1; i < t->mip_levels; i++) {
        int32_t dw = sw > 1 ? sw >> 1 : 1, dh = sh > 1 ? sh >> 1 : 1;
        img_barrier(cb, t->img, VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, 1,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    0, VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkImageBlit blit = {
            { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1 }, { { 0, 0, 0 }, { sw, sh, 1 } },
            { VK_IMAGE_ASPECT_COLOR_BIT, i,     0, 1 }, { { 0, 0, 0 }, { dw, dh, 1 } } };
        vkCmdBlitImage(cb, t->img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       t->img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
        /* This level becomes the source for the next. */
        img_barrier(cb, t->img, VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, 1,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        sw = dw; sh = dh;
    }
    /* Leave every level in the usage-appropriate rest layout. */
    VkImageLayout rl = rest_layout(t->usage);
    int sampled = rl == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    img_barrier(cb, t->img, VK_IMAGE_ASPECT_COLOR_BIT, 0, t->mip_levels, 0, 1,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, rl,
                VK_ACCESS_TRANSFER_READ_BIT, sampled ? VK_ACCESS_SHADER_READ_BIT : 0,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                sampled ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    if (!oneshot_end(cb)) return RHI_ERR_DEVICE_LOST;
    t->layout = rl;
    return RHI_OK;
}

void rhi_texture_destroy(RhiTexture tex) {
    RhiVkTexture *t = texture_lookup(tex, "texture_destroy");
    if (!t) return;
    if (t->view) vkDestroyImageView(g_vk.device, t->view, NULL);
    if (t->img)  vkDestroyImage(g_vk.device, t->img, NULL);
    if (t->mem)  vkFreeMemory(g_vk.device, t->mem, NULL);
    t->view = VK_NULL_HANDLE; t->img = VK_NULL_HANDLE; t->mem = VK_NULL_HANDLE; t->live = 0;
    /* Retire the slot before its generation wraps and revives stale handles. */
    if (t->generation == UINT8_MAX) return;
    t->generation++;
    g_vk.textures.free_list[g_vk.textures.free_count++] = tex.id & VK_SLOT_MASK;
}

int rhi_texture_is_live(RhiTexture tex) {
    return texture_find(tex) != NULL;
}

uint32_t rhi_texture_native_id(RhiTexture tex) {
    /* Stable, nonzero per live texture (the slot index); 0 for {0}/destroyed.
     * An identity for equality, not an ordering, per the rhi.h contract. */
    return texture_find(tex) ? (tex.id & VK_SLOT_MASK) : 0u;
}

/* Test-only (not in rhi.h): read one uncompressed-colour mip level/face back
 * into `dst` as tightly packed RGBA8 (R at byte 0), via an image->host-buffer
 * copy like rhi_read_pixels. The smoke declares it extern to verify uploaded
 * pixels, since rhi.h exposes no texture readback. 1 ok, 0 on failure. */
int rhi_vk_texture_readback(RhiTexture tex, int32_t face, int32_t level, void *dst) {
    RhiVkTexture *t = texture_lookup(tex, "texture_readback");
    int faces = t && t->layers == 6 ? 6 : 1;
    if (!t || !dst || format_compressed(t->format) || t->aspect != VK_IMAGE_ASPECT_COLOR_BIT ||
        face < 0 || face >= faces || level < 0 || (uint32_t)level >= t->mip_levels) return 0;
    int32_t w = t->width >> level, h = t->height >> level;
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    VkDeviceSize size = (VkDeviceSize)w * h * 4;
    VkBuffer staging; VkDeviceMemory smem;
    if (!make_buffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &staging, &smem)) return 0;
    VkCommandBuffer cb = oneshot_begin();
    int ok = cb != VK_NULL_HANDLE;
    if (ok) {
        img_barrier(cb, t->img, VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)level, 1, (uint32_t)face, 1,
                    t->layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, VK_ACCESS_TRANSFER_READ_BIT,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkBufferImageCopy reg = { 0, 0, 0, { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)level, (uint32_t)face, 1 },
            { 0, 0, 0 }, { (uint32_t)w, (uint32_t)h, 1 } };
        vkCmdCopyImageToBuffer(cb, t->img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, staging, 1, &reg);
        /* Restore the subresource to the tracked whole-image layout. */
        img_barrier(cb, t->img, VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)level, 1, (uint32_t)face, 1,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, t->layout, VK_ACCESS_TRANSFER_READ_BIT, 0,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        ok = oneshot_end(cb);
    }
    if (ok) {
        void *map = NULL;
        ok = vk_ok(vkMapMemory(g_vk.device, smem, 0, VK_WHOLE_SIZE, 0, &map), "vkMapMemory(tex readback)");
        if (ok) { memcpy(dst, map, (size_t)size); vkUnmapMemory(g_vk.device, smem); }
    }
    vkDestroyBuffer(g_vk.device, staging, NULL);
    vkFreeMemory(g_vk.device, smem, NULL);
    return ok;
}

/* --- Samplers (P11c) ---
 * A short list searched by value (GL's g_samplers): equal descriptions return
 * the same handle id (index + 1). Each distinct description gets one VkSampler. */
static int sampler_desc_valid(const RhiSamplerDesc *d) {
    return d && (d->min_filter == RHI_FILTER_NEAREST || d->min_filter == RHI_FILTER_LINEAR) &&
           (d->mag_filter == RHI_FILTER_NEAREST || d->mag_filter == RHI_FILTER_LINEAR) &&
           d->mip_filter >= RHI_MIP_NONE && d->mip_filter <= RHI_MIP_LINEAR &&
           (d->wrap_u == RHI_WRAP_REPEAT || d->wrap_u == RHI_WRAP_CLAMP) &&
           (d->wrap_v == RHI_WRAP_REPEAT || d->wrap_v == RHI_WRAP_CLAMP) &&
           d->max_anisotropy >= 1.0f;
}

static VkSamplerAddressMode vk_wrap(RhiWrap w) {
    return w == RHI_WRAP_CLAMP ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

RhiSampler rhi_sampler_create(const RhiSamplerDesc *desc) {
    RhiSampler out = {0};
    if (!sampler_desc_valid(desc)) { fprintf(stderr, "rhi: sampler_create: invalid description\n"); return out; }
    for (uint32_t i = 0; i < g_vk.samplers.count; i++) {
        const RhiSamplerDesc *d = &g_vk.samplers.descs[i];
        if (d->min_filter == desc->min_filter && d->mag_filter == desc->mag_filter &&
            d->mip_filter == desc->mip_filter && d->wrap_u == desc->wrap_u &&
            d->wrap_v == desc->wrap_v && d->max_anisotropy == desc->max_anisotropy) {
            out.id = i + 1; return out;
        }
    }
    if (g_vk.samplers.count == g_vk.samplers.capacity) {
        uint32_t cap = g_vk.samplers.capacity ? g_vk.samplers.capacity * 2u : 16u;
        RhiSamplerDesc *descs = (RhiSamplerDesc *)realloc(g_vk.samplers.descs, cap * sizeof *descs);
        if (!descs) { fprintf(stderr, "rhi: sampler_create: out of memory\n"); return out; }
        g_vk.samplers.descs = descs;
        VkSampler *handles = (VkSampler *)realloc(g_vk.samplers.handles, cap * sizeof *handles);
        if (!handles) { fprintf(stderr, "rhi: sampler_create: out of memory\n"); return out; }
        g_vk.samplers.handles = handles;
        g_vk.samplers.capacity = cap;
    }
    /* Clamp anisotropy to the device limit, off at 1x or with no support. */
    VkBool32 aniso_en = VK_FALSE;
    float aniso = 1.0f;
    if (g_vk.caps.max_anisotropy > 1.0f && desc->max_anisotropy > 1.0f) {
        aniso_en = VK_TRUE;
        aniso = desc->max_anisotropy < g_vk.caps.max_anisotropy ? desc->max_anisotropy
                                                                : g_vk.caps.max_anisotropy;
    }
    VkSamplerCreateInfo sci = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, NULL, 0,
        desc->mag_filter == RHI_FILTER_LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
        desc->min_filter == RHI_FILTER_LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
        desc->mip_filter == RHI_MIP_LINEAR ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST,
        vk_wrap(desc->wrap_u), vk_wrap(desc->wrap_v), VK_SAMPLER_ADDRESS_MODE_REPEAT,
        0.0f, aniso_en, aniso, VK_FALSE, VK_COMPARE_OP_ALWAYS,
        0.0f, desc->mip_filter == RHI_MIP_NONE ? 0.0f : VK_LOD_CLAMP_NONE,
        VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK, VK_FALSE };
    VkSampler handle = VK_NULL_HANDLE;
    if (!vk_ok(vkCreateSampler(g_vk.device, &sci, NULL, &handle), "vkCreateSampler")) return out;
    g_vk.samplers.descs[g_vk.samplers.count] = *desc;
    g_vk.samplers.handles[g_vk.samplers.count] = handle;
    g_vk.samplers.count++;
    out.id = g_vk.samplers.count;
    return out;
}

const RhiSamplerDesc *rhi_sampler_desc(RhiSampler sampler) {
    if (!sampler.id || sampler.id > g_vk.samplers.count) return NULL;
    return &g_vk.samplers.descs[sampler.id - 1];
}

RhiResult rhi_texture_set_sampler(RhiTexture tex, RhiSampler sampler) {
    RhiVkTexture *t = texture_lookup(tex, "texture_set_sampler");
    const RhiSamplerDesc *d = rhi_sampler_desc(sampler);
    if (!t || !d || t->format == RHI_FORMAT_D16) return RHI_ERR_INVALID;   /* D16 is not sampled */
    t->sampler = sampler;
    return RHI_OK;
}

RhiSampler rhi_texture_sampler(RhiTexture tex) {
    RhiVkTexture *t = texture_lookup(tex, "texture_sampler");
    return t ? t->sampler : (RhiSampler){0};
}

/* --- Shaders / param blocks (P11d) --- */
/* std140 layout validation, identical to the GL backend (backend-neutral). */
static uint32_t param_size(RhiParamType t) {
    switch (t) {
    case RHI_PARAM_VEC2: return 8u;
    case RHI_PARAM_VEC3: return 12u;
    case RHI_PARAM_VEC4: return 16u;
    case RHI_PARAM_MAT4: return 64u;
    default:             return 4u;
    }
}
static uint32_t param_align(RhiParamType t) {
    return t == RHI_PARAM_VEC2 ? 8u : (t == RHI_PARAM_VEC3 || t == RHI_PARAM_VEC4 ||
                                       t == RHI_PARAM_MAT4) ? 16u : 4u;
}
RhiResult rhi_param_layout_check(const RhiParamLayout *l) {
    const char *name = l && l->name ? l->name : "params";
    if (!l || !l->fields || !l->field_count) {
        fprintf(stderr, "rhi: %s: empty parameter layout\n", name);
        return RHI_ERR_INVALID;
    }
    for (uint32_t i = 0; i < l->field_count; i++) {
        const RhiParamField *f = &l->fields[i];
        if (!f->name || f->type < RHI_PARAM_FLOAT || f->type > RHI_PARAM_INT || !f->count) {
            fprintf(stderr, "rhi: %s: field %u is malformed\n", name, (unsigned)i);
            return RHI_ERR_INVALID;
        }
        if (f->count > 1 && f->type != RHI_PARAM_VEC4 && f->type != RHI_PARAM_MAT4) {
            fprintf(stderr, "rhi: %s: %s: std140 arrays must be vec4 or mat4\n", name, f->name);
            return RHI_ERR_INVALID;
        }
        uint64_t end = (uint64_t)f->offset + (uint64_t)param_size(f->type) * f->count;
        if (f->offset % param_align(f->type) || end > l->size || end < f->offset) {
            fprintf(stderr, "rhi: %s: %s: offset %u breaks std140 alignment or the block size\n",
                    name, f->name, (unsigned)f->offset);
            return RHI_ERR_INVALID;
        }
        for (uint32_t j = 0; j < i; j++) {
            const RhiParamField *g = &l->fields[j];
            uint64_t gend = (uint64_t)g->offset + (uint64_t)param_size(g->type) * g->count;
            if (f->offset < gend && g->offset < end) {
                fprintf(stderr, "rhi: %s: %s overlaps %s\n", name, f->name, g->name);
                return RHI_ERR_INVALID;
            }
        }
    }
    return RHI_OK;
}

/* Build-time SPIR-V for a shader label + stage. The Vulkan backend can't compile
 * the GLSL in RhiShaderDesc at runtime, so it ships a Vulkan-GLSL variant per
 * label (src/shaders/vk/), compiled to SPIR-V by the build; desc->label selects.
 * An unknown label is unsupported (the engine's shader set is fixed). */
static const uint32_t *vk_shader_spirv(const char *label, int frag, size_t *bytes) {
    if (label && !strcmp(label, "frontend")) {
        if (frag) { *bytes = sizeof vk_frontend_frag; return vk_frontend_frag; }
        *bytes = sizeof vk_frontend_vert; return vk_frontend_vert;
    }
    return NULL;   /* TODO P11e: ug2, headlight shadow, world_debug, hud */
}

static RhiVkShader *shader_lookup(RhiShader sh, const char *what) {
    uint32_t slot = sh.id & VK_SLOT_MASK;
    if (!sh.id) return NULL;
    if (slot && slot <= g_vk.shaders.count) {
        RhiVkShader *s = &g_vk.shaders.slots[slot - 1];
        if (s->live && s->generation == (uint8_t)(sh.id >> VK_SLOT_BITS)) return s;
    }
    if (g_vk.shaders.stale_logged < 8) {
        g_vk.shaders.stale_logged++;
        fprintf(stderr, "rhi: %s: stale or invalid shader handle 0x%08x\n", what, (unsigned)sh.id);
    }
    return NULL;
}
static uint32_t shader_slot_alloc(void) {
    if (g_vk.shaders.free_count) return g_vk.shaders.free_list[--g_vk.shaders.free_count];
    if (g_vk.shaders.count == g_vk.shaders.capacity) {
        uint32_t cap = g_vk.shaders.capacity ? g_vk.shaders.capacity * 2u : 64u;
        if (cap > VK_SLOT_MASK) return 0;
        RhiVkShader *slots = (RhiVkShader *)realloc(g_vk.shaders.slots, cap * sizeof *slots);
        if (!slots) return 0;
        g_vk.shaders.slots = slots;
        uint32_t *list = (uint32_t *)realloc(g_vk.shaders.free_list, cap * sizeof *list);
        if (!list) return 0;
        g_vk.shaders.free_list = list;
        g_vk.shaders.capacity = cap;
    }
    memset(&g_vk.shaders.slots[g_vk.shaders.count], 0, sizeof g_vk.shaders.slots[0]);
    return ++g_vk.shaders.count;
}

static VkShaderModule make_module(const uint32_t *spv, size_t bytes) {
    VkShaderModule m = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo ci = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, NULL, 0, bytes, spv };
    if (!vk_ok(vkCreateShaderModule(g_vk.device, &ci, NULL, &m), "vkCreateShaderModule")) return VK_NULL_HANDLE;
    return m;
}

/* Tear down a shader slot's resources and retire it (bump generation, return the
 * slot to the free list). Used by the create-failure path and rhi_shader_destroy.
 * Unmap is guarded by `mapped` so a slot whose vkMapMemory failed is freed
 * without unmapping memory that was never mapped. */
static void shader_slot_free(uint32_t slot) {
    RhiVkShader *s = &g_vk.shaders.slots[slot - 1];
    for (uint32_t i = 0; i < s->block_count; i++) {
        if (s->blocks[i].mapped)  vkUnmapMemory(g_vk.device, s->blocks[i].ubo_mem);
        if (s->blocks[i].ubo)     vkDestroyBuffer(g_vk.device, s->blocks[i].ubo, NULL);
        if (s->blocks[i].ubo_mem) vkFreeMemory(g_vk.device, s->blocks[i].ubo_mem, NULL);
    }
    if (s->vert) vkDestroyShaderModule(g_vk.device, s->vert, NULL);
    if (s->frag) vkDestroyShaderModule(g_vk.device, s->frag, NULL);
    uint8_t gen = s->generation;
    memset(s, 0, sizeof *s);
    if (gen != UINT8_MAX) {   /* retire the slot unless its generation would wrap */
        s->generation = gen + 1;
        g_vk.shaders.free_list[g_vk.shaders.free_count++] = slot;
    } else {
        s->generation = gen;
    }
}

RhiShader rhi_shader_create(const RhiShaderDesc *desc) {
    RhiShader out = {0};
    if (!desc || desc->param_block_count > VK_MAX_PARAM_BLOCKS ||
        (desc->param_block_count && !desc->param_blocks)) {
        fprintf(stderr, "rhi: shader_create: invalid description\n"); return out;
    }
    for (uint32_t i = 0; i < desc->param_block_count; i++)
        if (rhi_param_layout_check(desc->param_blocks[i]) != RHI_OK) return out;
    size_t vb = 0, fb = 0;
    const uint32_t *vspv = vk_shader_spirv(desc->label, 0, &vb);
    const uint32_t *fspv = vk_shader_spirv(desc->label, 1, &fb);
    if (!vspv || !fspv) {
        fprintf(stderr, "rhi_vk: no Vulkan SPIR-V for shader '%s'\n", desc->label ? desc->label : "?");
        return out;
    }
    if (!g_vk.device) { fprintf(stderr, "rhi: shader_create: no device\n"); return out; }

    uint32_t slot = shader_slot_alloc();
    if (!slot) { fprintf(stderr, "rhi: shader_create: out of handles\n"); return out; }
    RhiVkShader *s = &g_vk.shaders.slots[slot - 1];
    s->vert = make_module(vspv, vb);
    s->frag = make_module(fspv, fb);
    if (!s->vert || !s->frag) goto fail;
    s->block_count = desc->param_block_count;
    for (uint32_t i = 0; i < s->block_count; i++) {
        const RhiParamLayout *l = desc->param_blocks[i];
        RhiVkParamBlock *pb = &s->blocks[i];
        pb->layout = l;
        pb->size = (uint32_t)l->size;
        if (!make_buffer(l->size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &pb->ubo, &pb->ubo_mem)) goto fail;
        if (!vk_ok(vkMapMemory(g_vk.device, pb->ubo_mem, 0, VK_WHOLE_SIZE, 0, &pb->mapped), "vkMapMemory(ubo)"))
            goto fail;
        memset(pb->mapped, 0, l->size);   /* params start zeroed, like the GL mirror */
    }
    s->live = 1;
    out.id = ((uint32_t)s->generation << VK_SLOT_BITS) | slot;
    return out;
fail:
    shader_slot_free(slot);
    return (RhiShader){0};
}

void *rhi_shader_params(RhiShader shader, uint32_t block) {
    RhiVkShader *s = shader_lookup(shader, "shader_params");
    if (!s || block >= s->block_count) return NULL;
    return s->blocks[block].mapped;   /* mapped UBO == the std140 mirror (host-coherent) */
}

void rhi_shader_destroy(RhiShader shader) {
    /* shader_lookup validates the slot (nonzero, in range), the generation (so a
     * stale handle to a reused slot is ignored) and liveness. */
    if (!shader.id) return;
    if (!shader_lookup(shader, "shader_destroy")) return;
    shader_slot_free(shader.id & VK_SLOT_MASK);
}

uint32_t rhi_shader_native_program(RhiShader shader) {
    RhiVkShader *s = shader_lookup(shader, "shader_native_program");
    return s ? (shader.id & VK_SLOT_MASK) : 0u;   /* stable nonzero id for a live shader */
}

/* --- Pipelines (TODO P11e). desc_init is pure: copy the GL backend's defaults. --- */
void rhi_pipeline_desc_init(RhiPipelineDesc *d) {
    memset(d, 0, sizeof *d);
    d->primitive = RHI_PRIMITIVE_TRIANGLES;
    d->cull = RHI_CULL_NONE;
    d->depth_test = 0;
    d->depth_write = 1;
    d->depth_compare = RHI_COMPARE_LESS;
    d->blend = 0;
    d->src_rgb = d->src_alpha = RHI_BLEND_ONE;
    d->dst_rgb = d->dst_alpha = RHI_BLEND_ZERO;
    d->color_write = 0xF;
    d->line_width = 1.0f;
    d->stencil_test = 0;
    d->stencil_compare = RHI_COMPARE_ALWAYS;
    d->stencil_ref = 0;
    d->stencil_read_mask = d->stencil_write_mask = 0xFF;
    d->stencil_fail = d->stencil_depth_fail = d->stencil_pass = RHI_STENCIL_KEEP;
}
RhiPipeline rhi_pipeline_create(const RhiPipelineDesc *desc) { (void)desc; RhiPipeline p = {0}; return p; }

/* --- Draws (TODO P11e) --- */
void rhi_cmd_bind_pipeline(RhiPipeline pipeline) { (void)pipeline; }
void rhi_cmd_bind_vertex_buffer(uint32_t stream, RhiBuffer buffer, uint32_t offset) {
    (void)stream; (void)buffer; (void)offset;
}
void rhi_cmd_bind_index_buffer(RhiBuffer buffer) { (void)buffer; }
void rhi_cmd_bind_texture(uint32_t slot, RhiTexture texture) { (void)slot; (void)texture; }
void rhi_cmd_draw_indexed(uint32_t index_count, uint32_t first_index) { (void)index_count; (void)first_index; }
void rhi_cmd_draw(uint32_t vertex_count, uint32_t first_vertex) { (void)vertex_count; (void)first_vertex; }

/* --- Passes / copies (TODO P11f) --- */
RhiResult rhi_cmd_begin_pass(const RhiPassDesc *desc) { (void)desc; return RHI_ERR_UNSUPPORTED; }
void rhi_cmd_end_pass(void) { }
RhiResult rhi_cmd_copy_main_to_texture(RhiTexture texture, int32_t x, int32_t y, int32_t w, int32_t h) {
    (void)texture; (void)x; (void)y; (void)w; (void)h; return RHI_ERR_UNSUPPORTED;
}

/* --- Viewport / state snapshot / wireframe (TODO P11e/P11g) --- */
void rhi_viewport(int32_t viewport[4]) {
    viewport[0] = 0; viewport[1] = 0; viewport[2] = g_vk.width; viewport[3] = g_vk.height;
}
void rhi_cmd_save_state(RhiStateSnapshot *out) { if (out) memset(out, 0, sizeof *out); }
void rhi_cmd_restore_state(const RhiStateSnapshot *state) { (void)state; }
void rhi_state_set_texture(RhiStateSnapshot *state, uint32_t slot, RhiTexture texture) {
    (void)state; (void)slot; (void)texture;
}
RhiResult rhi_cmd_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height) {
    (void)x; (void)y; (void)width; (void)height; return RHI_ERR_UNSUPPORTED;
}
RhiResult rhi_cmd_set_wireframe(int on) { (void)on; return RHI_ERR_UNSUPPORTED; }

/* --- GPU timers (TODO P11g) --- */
RhiTimer rhi_timer_create(void) { RhiTimer t = {0}; return t; }
void rhi_timer_destroy(RhiTimer timer) { (void)timer; }
RhiResult rhi_cmd_timer_begin(RhiTimer timer) { (void)timer; return RHI_ERR_UNSUPPORTED; }
RhiResult rhi_cmd_timer_end(void) { return RHI_ERR_UNSUPPORTED; }
int rhi_timer_read(RhiTimer timer, uint64_t *ns) { (void)timer; (void)ns; return 0; }
