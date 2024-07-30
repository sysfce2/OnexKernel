#ifndef ONL_VK
#define ONL_VK

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include <vulkan/vulkan.h>

#include <onex-kernel/log.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define ERR_EXIT(msg) \
  do {                             \
    log_write("%s\n", msg);     \
    onl_vk_exit();             \
  } while (0)

#define VK_CHECK(r) \
  do {  \
    VkResult res = (r); \
    if(res != VK_SUCCESS){  \
       log_write("r=%d %s:%d\n", r, __FILE__, __LINE__); \
       onl_vk_exit();  \
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
extern float aspect_ratio_proj;

extern uint32_t max_img;
extern uint32_t cur_img;

extern VkPipelineLayout pipeline_layout;

extern VkDescriptorSetLayout descriptor_layout;

extern VkPipelineVertexInputStateCreateInfo vertex_input_state_ci;

struct push_constants { //!!
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

void onl_vk_exit();

// calls from ONL VK up to ONT
void ont_init();
void ont_iostate_changed();
void ont_prepare_render_data(bool restart);
void ont_prepare_uniform_buffers(bool restart);
void ont_prepare_descriptor_layout(bool restart);
void ont_prepare_descriptor_pool(bool restart);
void ont_prepare_descriptor_set(bool restart);
void ont_prepare_shaders(bool restart);
void ont_update_uniforms();
void ont_finish_render_data();

#endif
