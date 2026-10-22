/* vk_offscreen_window_shim.c -- offscreen-window shim for the Vulkan
 * (`make nfsu2-vk`) build ONLY.
 *
 * WHY THIS EXISTS
 * The Vulkan backend (src/rhi_vk.c) advertises SDL_WINDOW_VULKAN via
 * rhi_window_flags(), and main.c passes that flag straight into
 * SDL_CreateWindow(). Under SDL_VIDEODRIVER=offscreen (headless lavapipe) the
 * offscreen video driver has NO Vulkan surface support, so SDL_CreateWindow()
 * refuses the SDL_WINDOW_VULKAN flag and returns NULL -- main.c then aborts with
 * "window: Vulkan support is either not configured in SDL ...".
 *
 * The offscreen backend does not use the SDL window as a Vulkan surface at all:
 * it renders into its own offscreen VkImage and reads it back (no swapchain, no
 * present). The SDL window is only a size/opacity holder. So the fix is exactly
 * what tools/rhi_vk_smoke.c does deliberately: create a plain hidden window
 * WITHOUT SDL_WINDOW_VULKAN. rhi_vk.c must not be modified (backend logic is
 * frozen) and main.c is left byte-for-byte identical so the Vulkan and GL
 * binaries differ only in the backend -- a fair screenshot comparison.
 *
 * Mechanism: ELF symbol interposition. A strong definition of SDL_CreateWindow
 * in the main executable is resolved before libSDL2.so's (the executable's
 * global symbols are searched first, for PIE and non-PIE alike). This wrapper
 * strips SDL_WINDOW_VULKAN and forwards to the real SDL_CreateWindow found via
 * dlsym(RTLD_NEXT, ...). Needs -ldl.
 *
 * Compiled into nfsu2-vk only; never into any GL build.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <string.h>
#include <SDL.h>

SDL_Window *SDL_CreateWindow(const char *title, int x, int y, int w, int h,
                             Uint32 flags) {
    static SDL_Window *(*real)(const char *, int, int, int, int, Uint32) = NULL;
    if (!real)
        real = (SDL_Window *(*)(const char *, int, int, int, int, Uint32))
               dlsym(RTLD_NEXT, "SDL_CreateWindow");
    /* Under the offscreen video driver (headless) the backend uses its own VkImage and no
     * surface; drop SDL_WINDOW_VULKAN so that driver accepts the window. With a real video
     * driver (x11, wayland, cocoa) keep the flag: the backend then creates a window surface. */
    const char *drv = SDL_GetCurrentVideoDriver();   /* the driver SDL picked, whether or not SDL_VIDEODRIVER was set */
    if (drv && (!strcmp(drv, "offscreen") || !strcmp(drv, "dummy")))
        flags &= ~(Uint32)SDL_WINDOW_VULKAN;
    return real ? real(title, x, y, w, h, flags) : NULL;
}
