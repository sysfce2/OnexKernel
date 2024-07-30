#ifndef VK_H
#define VK_H

#include <vulkan/vulkan.h>

// calls down from vk.c to vulkan-xcb.c etc
void onl_init();
void onl_create_window();
void onl_create_surface(VkInstance inst, VkSurfaceKHR* surface);
void onl_finish();

// calls from vulkan-xcb.c etc up to vk.c
void ont_vk_loop(bool running);
void ont_vk_iostate_changed();
void ont_vk_set_io_mouse(int32_t x, int32_t y);

// calls from src/onl/vulkan/vk.c to src/onl/vulkan/vk-rg.c
void onl_vk_rg_prepare_swapchain_images(bool restart);
void onl_vk_rg_prepare_semaphores_and_fences(bool restart);
void onl_vk_rg_prepare_command_buffers(bool restart);
void onl_vk_rg_prepare_rendering(bool restart);
void onl_vk_rg_prepare_pipeline_layout(bool restart);
void onl_vk_rg_prepare_render_pass(bool restart);
void onl_vk_rg_prepare_pipeline(bool restart);
void onl_vk_rg_prepare_framebuffers(bool restart);
void onl_vk_rg_render_frame();
void onl_vk_rg_finish_rendering();

#endif
