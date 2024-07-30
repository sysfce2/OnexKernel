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

#endif
