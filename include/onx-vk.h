#ifndef ONX_VK
#define ONX_VK

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include <vulkan/vulkan.h>

#include <onex-kernel/log.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define VK_DESTROY(func, dev, obj) func(dev, obj, NULL), obj = NULL

#define ERR_EXIT(msg) \
  do {                             \
    log_write("%s\n", msg);     \
    onl_exit();             \
  } while (0)

#define VK_CHECK(r) \
  do {  \
    VkResult res = (r); \
    if(res != VK_SUCCESS){  \
       log_write("r=%d %s:%d\n", r, __FILE__, __LINE__); \
       onl_exit();  \
    }  \
  } while (0)

typedef struct iostate {
  uint32_t swap_width;
  uint32_t swap_height;
  uint32_t mouse_x;
  uint32_t mouse_y;
  bool     left_pressed;
  bool     middle_pressed;
  bool     right_pressed;
  char     key;
} iostate;

extern iostate io;

extern VkFormat surface_format;
extern VkDevice device;
extern VkPhysicalDevice gpu;
extern VkCommandBuffer initcmd;
extern VkQueue queue;
extern VkCommandPool command_pool;
extern VkSwapchainKHR swapchain;
extern VkExtent2D swapchain_extent;
extern VkShaderModule vert_shader_module;
extern VkShaderModule frag_shader_module;

extern float aspect_ratio;

extern uint32_t max_img;
extern uint32_t cur_img;

extern VkPipelineLayout pipeline_layout;

extern VkPipelineVertexInputStateCreateInfo vertex_input_state_ci;

struct push_constants {
  uint32_t phase;
};

extern bool prepared;

extern bool            scene_ready;
extern pthread_mutex_t scene_lock;

void set_proj_view();

VkCommandBuffer begin_cmd_buf(uint32_t ii);
void            begin_render_pass(uint32_t ii, VkCommandBuffer cmd_buf);
void            end_cmd_buf_and_render_pass(uint32_t ii, VkCommandBuffer cmd_buf);

void transition_image(
    VkCommandBuffer cmdBuffer,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlagBits srcAccessMask,
    VkAccessFlagBits dstAccessMask,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage);

uint32_t create_buffer_with_memory(VkBufferCreateInfo*   buffer_ci,
                                   VkMemoryPropertyFlags prop_flags,
                                   VkBuffer*             buffer,
                                   VkDeviceMemory*       memory);

uint32_t create_image_with_memory(VkImageCreateInfo*    image_ci,
                                  VkMemoryPropertyFlags prop_flags,
                                  VkImage*              image,
                                  VkDeviceMemory*       memory);

void onl_exit();

void onl_vk_rg_prepare_swapchain_images(bool restart);
void onl_vk_rg_prepare_semaphores_and_fences(bool restart);
void onl_vk_rg_prepare_command_buffers(bool restart);
void onl_vk_rg_prepare_rendering(bool restart);
void onx_vk_rd_prepare_render_data(bool restart);
void onx_vk_rd_prepare_uniform_buffers(bool restart);
void onx_vk_rd_prepare_descriptor_layout(bool restart);
void onl_vk_rg_prepare_pipeline_layout(bool restart);
void onx_vk_rd_prepare_descriptor_pool(bool restart);
void onx_vk_rd_prepare_descriptor_set(bool restart);
void onl_vk_rg_prepare_render_pass(bool restart);
void onx_vk_rd_prepare_shaders(bool restart);
void onl_vk_rg_prepare_pipeline(bool restart);
void onl_vk_rg_prepare_framebuffers(bool restart);
void onx_vk_rd_update_uniforms();
void onl_vk_rg_render_frame();
void onl_vk_rg_finish_rendering();
void onx_vk_rd_finish_render_data();

#endif
