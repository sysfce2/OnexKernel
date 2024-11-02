#ifndef ONL_VK_H
#define ONL_VK_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#include <vulkan/vulkan.h>

#include <onex-kernel/log.h>

#include "onl.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define ONL_VK_CHECK_EXIT(vkAPICall) \
  do {                               \
    VkResult res = (vkAPICall);      \
    if(res != VK_SUCCESS){           \
       log_write("vkAPI() call, result=%d at %s:%d\n", res, __FILE__, __LINE__); \
       exit(1);                      \
    }                                \
  } while (0)

#define ONL_VK_CHECK_QUIT(vkAPICall) \
  do {                               \
    VkResult res = (vkAPICall);      \
    if(res != VK_SUCCESS){           \
       log_write("vkAPI() call, result=%d at %s:%d\n", res, __FILE__, __LINE__); \
       onl_vk_quit();                \
    }                                \
  } while (0)

#define ONL_VK_ERR_EXIT(msg) \
  do {                       \
    log_write("%s\n", msg);  \
    exit(1);                 \
  } while (0)

#define ONL_VK_ERR_QUIT(msg) \
  do {                       \
    log_write("%s\n", msg);  \
    onl_vk_quit();           \
  } while (0)

extern uint32_t                             onl_vk_width;
extern uint32_t                             onl_vk_height;
extern float                                onl_vk_aspect_ratio;

extern VkDevice                             onl_vk_device;
extern VkFormat                             onl_vk_texture_format;
extern VkFormatProperties                   onl_vk_texture_format_properties;
extern VkCommandBuffer                      onl_vk_init_cmdbuf;
extern uint32_t                             onl_vk_min_storage_buffer_offset_alignment;
extern VkPipelineLayout                     onl_vk_pipeline_layout;
extern VkPipelineVertexInputStateCreateInfo onl_vk_vertex_input_state_ci;
extern uint32_t                             onl_vk_max_img;
extern uint32_t                             onl_vk_cur_img;
extern VkShaderModule                       onl_vk_vert_shader_module;
extern VkShaderModule                       onl_vk_frag_shader_module;

// ONL waits for running (scene ready) from ONT
// ONT detects finishing from ONL
#define ONL_VK_RENDER_STAGE_PREPARE    0
#define ONL_VK_RENDER_STAGE_RUNNING    1
#define ONL_VK_RENDER_STAGE_FINISHING  2
extern uint8_t                              onl_vk_render_stage;
extern pthread_mutex_t                      onl_vk_render_stage_lock;

// -----------------------------------

void onl_vk_begin_init_command_buffer();
void onl_vk_end_init_command_buffer();

VkCommandBuffer onl_vk_begin_cmd_buf(uint32_t ii);
void            onl_vk_begin_render_pass(uint32_t ii, VkCommandBuffer cmd_buf);
void            onl_vk_end_cmd_buf_and_render_pass(uint32_t ii, VkCommandBuffer cmd_buf);

void onl_vk_transition_image(VkCommandBuffer cmdBuffer,
                             VkImage image,
                             VkImageLayout oldLayout,
                             VkImageLayout newLayout,
                             VkAccessFlagBits srcAccessMask,
                             VkAccessFlagBits dstAccessMask,
                             VkPipelineStageFlags srcStage,
                             VkPipelineStageFlags dstStage);

uint32_t onl_vk_create_buffer_with_memory(VkBufferCreateInfo*   buffer_ci,
                                          VkMemoryPropertyFlags prop_flags,
                                          VkBuffer*             buffer,
                                          VkDeviceMemory*       memory);

uint32_t onl_vk_create_image_with_memory(VkImageCreateInfo*    image_ci,
                                         VkMemoryPropertyFlags prop_flags,
                                         VkImage*              image,
                                         VkDeviceMemory*       memory);

void onl_vk_quit();

// calls from ONL VK up to ONT VK
void ont_vk_init();
void ont_vk_iostate_changed();
void ont_vk_prepare_render_data(bool restart);
void ont_vk_prepare_uniform_buffers(bool restart);
void ont_vk_prepare_descriptor_layout(bool restart);
void ont_vk_prepare_pipeline_layout(bool restart);
void ont_vk_prepare_descriptor_pool(bool restart);
void ont_vk_prepare_descriptor_set(bool restart);
void ont_vk_prepare_shaders(bool restart);
void ont_vk_update_uniforms();
void ont_vk_finish_render_data();

#endif
