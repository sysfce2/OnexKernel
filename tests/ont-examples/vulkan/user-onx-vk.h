
#include <linmath-plus.h>

#define MAX_PANELS 32 // TODO set src/ont/unix/onx.vert

extern float aspect_ratio;
extern float aspect_ratio_proj;

extern uint32_t max_img;
extern uint32_t cur_img;

extern VkPipelineLayout pipeline_layout;

struct push_constants {
  uint32_t phase;
};

extern mat4x4 proj_matrix;
extern mat4x4 view_l_matrix;
extern mat4x4 view_r_matrix;
extern mat4x4 model_matrix[MAX_PANELS];

void set_up_scene_begin(float** vertices);
void set_up_scene_end();

extern bool            scene_ready;
extern pthread_mutex_t scene_lock;

extern VkDescriptorSetLayout descriptor_layout;

void set_proj_view();

VkCommandBuffer begin_cmd_buf_and_render_pass(uint32_t ii);
void            end_cmd_buf_and_render_pass(  uint32_t ii, VkCommandBuffer cmd_buf);

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
