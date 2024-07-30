#ifndef VK_H
#define VK_H

#include <vulkan/vulkan.h>

extern VkFormat surface_format;

// calls from vk.c down to vulkan-xcb.c etc
void onl_vk_init();
void onl_vk_create_window();
void onl_vk_create_surface(VkInstance inst, VkSurfaceKHR* surface);
void onl_vk_finish();

// calls from vulkan-xcb.c etc up to vk.c
void onl_vk_loop(bool running);
void onl_vk_iostate_changed();
void onl_vk_set_io_mouse(int32_t x, int32_t y);

// calls from vk.c up to vk-rg.c
void onl_vk_prepare_swapchain_images(bool restart);
void onl_vk_prepare_semaphores_and_fences(bool restart);
void onl_vk_prepare_command_buffers(bool restart);
void onl_vk_prepare_rendering(bool restart);
void onl_vk_prepare_pipeline_layout(bool restart);
void onl_vk_prepare_render_pass(bool restart);
void onl_vk_prepare_pipeline(bool restart);
void onl_vk_prepare_framebuffers(bool restart);
void onl_vk_render_frame();
void onl_vk_finish_rendering();

// calls from vk-rg.c down to vk.c
void onl_vk_restart();

#endif
