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

/* --- Textures (TODO P11c) --- */
RhiTexture rhi_texture_create(const RhiTextureDesc *desc) { (void)desc; RhiTexture t = {0}; return t; }
RhiResult rhi_texture_upload(RhiTexture tex, int32_t face, int32_t level, const void *data, size_t size) {
    (void)tex; (void)face; (void)level; (void)data; (void)size; return RHI_ERR_UNSUPPORTED;
}
RhiResult rhi_texture_generate_mips(RhiTexture tex) { (void)tex; return RHI_ERR_UNSUPPORTED; }
void rhi_texture_destroy(RhiTexture tex) { (void)tex; }
int rhi_texture_is_live(RhiTexture tex) { (void)tex; return 0; }
uint32_t rhi_texture_native_id(RhiTexture tex) { (void)tex; return 0; }

/* --- Samplers (TODO P11c) --- */
RhiSampler rhi_sampler_create(const RhiSamplerDesc *desc) { (void)desc; RhiSampler s = {0}; return s; }
const RhiSamplerDesc *rhi_sampler_desc(RhiSampler sampler) { (void)sampler; return NULL; }
RhiResult rhi_texture_set_sampler(RhiTexture tex, RhiSampler sampler) {
    (void)tex; (void)sampler; return RHI_ERR_UNSUPPORTED;
}
RhiSampler rhi_texture_sampler(RhiTexture tex) { (void)tex; RhiSampler s = {0}; return s; }

/* --- Shaders / param blocks (TODO P11d) --- */
RhiResult rhi_param_layout_check(const RhiParamLayout *layout) { (void)layout; return RHI_ERR_UNSUPPORTED; }
RhiShader rhi_shader_create(const RhiShaderDesc *desc) { (void)desc; RhiShader s = {0}; return s; }
void rhi_shader_destroy(RhiShader shader) { (void)shader; }
void *rhi_shader_params(RhiShader shader, uint32_t block) { (void)shader; (void)block; return NULL; }
uint32_t rhi_shader_native_program(RhiShader shader) { (void)shader; return 0; }

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
