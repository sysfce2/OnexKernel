// ------------------------------------------------------------------------------------------------------------------------------------

#include <onex-kernel/log.h>
#include <onx-vk.h>

#include "user-onx-vk.h"

extern void ont_vk_restart(); //!! FIXME

static uint32_t image_count;
static uint32_t image_index;

static VkBuffer vertex_buffer;
static VkBuffer staging_buffer;

static VkDeviceMemory vertex_buffer_memory;
static VkDeviceMemory staging_buffer_memory;

static VkRenderPass render_pass;

static VkSemaphore image_acquired_semaphore;
static VkSemaphore render_complete_semaphore;

static VkPipeline       pipeline;
static VkPipelineLayout pipeline_layout;
static VkPipelineCache  pipeline_cache;

static VkDescriptorSetLayout descriptor_layout;
static VkDescriptorPool      descriptor_pool;

static VkFormatProperties               format_properties;
static VkPhysicalDeviceMemoryProperties memory_properties;

// ---------------------------------

static VkFormat texture_format = VK_FORMAT_R8G8B8A8_UNORM;

struct texture_object {
    int32_t texture_width;
    int32_t texture_height;
    VkSampler sampler;
    VkBuffer buffer;
    VkImageLayout image_layout;
    VkDeviceMemory device_memory;
    VkImage image;
    VkImageView image_view;
};

struct {
    VkFormat format;
    VkDeviceMemory device_memory;
    VkImage image;
    VkImageView image_view;
} depth;

struct uniforms {
    float proj[4][4];
    float view[4][4];
    float model[MAX_PANELS][4][4];
};

typedef struct {
    VkBuffer        uniform_buffer;
    VkDeviceMemory  uniform_memory;
    void*           uniform_memory_ptr;
    VkDescriptorSet descriptor_set;
} uniform_mem_t;

static uniform_mem_t *uniform_mem;

typedef struct {
    VkFramebuffer   framebuffer;
    VkImage         image;
    VkImageView     image_view;
    VkCommandBuffer command_buffer;
    VkFence         command_buffer_fence;
} SwapchainImageResources;

static SwapchainImageResources *swapchain_image_resources;

struct push_constants {
  uint32_t phase;
};

void transition_image(
    VkCommandBuffer cmdBuffer,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlagBits srcAccessMask,
    VkAccessFlagBits dstAccessMask,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage) {

    VkImageMemoryBarrier img_mem_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image = image,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAccessMask,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount     = VK_REMAINING_ARRAY_LAYERS,
        },
        .pNext = NULL,
    };

    vkCmdPipelineBarrier(
        cmdBuffer,
        srcStage,
        dstStage,
        0,
        0, NULL,
        0, NULL,
        1,
        &img_mem_barrier
    );
}

static void build_render_pass_and_cmdbufs(uint32_t ii) {

  vkWaitForFences(device, 1, &swapchain_image_resources[ii].command_buffer_fence, VK_TRUE, UINT64_MAX);

  VkCommandBuffer cmd_buf = swapchain_image_resources[ii].command_buffer;

  const VkCommandBufferBeginInfo command_buffer_bi = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
    .pInheritanceInfo = NULL,
    .pNext = 0,
  };

  VK_CHECK(vkBeginCommandBuffer(cmd_buf, &command_buffer_bi));

  // --------------------------------------------

  const VkClearValue clear_values[] = {
    { .color.float32 = { 0.2f, 0.8f, 1.0f, 0.0f } },
    { .depthStencil = { 1.0f, 0 }},
  };

  const VkRenderPassBeginInfo render_pass_bi = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = render_pass,
      .framebuffer = swapchain_image_resources[ii].framebuffer,
      .renderArea.offset = { 0, 0 },
      .renderArea.extent = swapchain_extent,
      .clearValueCount = 2,
      .pClearValues = clear_values,
      .pNext = 0,
  };

  vkCmdBeginRenderPass(cmd_buf, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);

  VkBuffer vertex_buffers[] = { vertex_buffer, };
  VkDeviceSize offsets[] = { 0 };
  vkCmdBindVertexBuffers(cmd_buf, 0, 1, vertex_buffers, offsets);

  vkCmdBindDescriptorSets(cmd_buf,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline_layout,
                          0, 1,
                          &uniform_mem[ii].descriptor_set,
                          0, NULL);

  vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  // --------------------------------------------

  struct push_constants pc;

  pc.phase = 0, // ground plane
  vkCmdPushConstants(cmd_buf, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                     sizeof(struct push_constants), &pc);
  vkCmdDraw(cmd_buf, 6, 1, 0, 0);

  pc.phase = 1, // panels
  vkCmdPushConstants(cmd_buf, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                     sizeof(struct push_constants), &pc);
  vkCmdDraw(cmd_buf, 6*6, 1, 0, 0);

  // --------------------------------------------

  vkCmdEndRenderPass(cmd_buf);
  VK_CHECK(vkEndCommandBuffer(cmd_buf));
}

static bool            scene_ready = false;
static pthread_mutex_t scene_lock;

void set_up_scene_begin(float** vertices) {

  pthread_mutex_lock(&scene_lock);
  scene_ready = false;

  size_t vertex_size = MAX_PANELS * 6*6 * (3 * sizeof(float) +
                                           2 * sizeof(float)  );

  VK_CHECK(vkMapMemory(device, vertex_buffer_memory, 0, vertex_size, 0, (void**)vertices));
}

void set_up_scene_end() {

  vkUnmapMemory(device, vertex_buffer_memory);

  for (uint32_t ii = 0; ii < image_count; ii++) {
      build_render_pass_and_cmdbufs(ii);
  }
  image_index = 0;

  scene_ready = true;
  pthread_mutex_unlock(&scene_lock);
}

void onx_vk_update_uniforms() {

  set_mvp_uniforms();

  memcpy(uniform_mem[image_index].uniform_memory_ptr,
         (const void*)&proj_matrix,  sizeof(proj_matrix));

  memcpy(uniform_mem[image_index].uniform_memory_ptr +
                                     sizeof(proj_matrix),
         (const void*)&view_matrix,  sizeof(view_matrix));

  memcpy(uniform_mem[image_index].uniform_memory_ptr +
                                     sizeof(proj_matrix)+sizeof(view_matrix),
         (const void*)&model_matrix, sizeof(model_matrix));
}

void onx_vk_render_frame() {

  vkWaitForFences(device, 1, &swapchain_image_resources[image_index].command_buffer_fence,
                  VK_TRUE, UINT64_MAX);

  pthread_mutex_lock(&scene_lock);
  if(!scene_ready){
    pthread_mutex_unlock(&scene_lock);
    return;
  }

  VkResult err;
  do {
      err = vkAcquireNextImageKHR(device,
                                  swapchain,
                                  UINT64_MAX,
                                  image_acquired_semaphore,
                                  VK_NULL_HANDLE,
                                  &image_index);

      if (err == VK_SUCCESS || err == VK_SUBOPTIMAL_KHR){
        break;
      }
      else
      if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        pthread_mutex_unlock(&scene_lock); // ??
        ont_vk_restart();
      }
      else {
        pthread_mutex_unlock(&scene_lock);
        return;
      }
  } while(true);

  VkPipelineStageFlags wait_stages[] = {
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
  };
  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pWaitDstStageMask = wait_stages,
    .waitSemaphoreCount = 1,
    .signalSemaphoreCount = 1,
    .commandBufferCount = 1,
  };

  VkSemaphore img_acq_semaphore[] = { image_acquired_semaphore };
  VkSemaphore ren_com_semaphore[] = { render_complete_semaphore };

  submit_info.pWaitSemaphores   = img_acq_semaphore;
  submit_info.pSignalSemaphores = ren_com_semaphore;
  submit_info.pCommandBuffers = &swapchain_image_resources[image_index].command_buffer,

  vkResetFences(device, 1, &swapchain_image_resources[image_index].command_buffer_fence);

  err = vkQueueSubmit(queue, 1, &submit_info,
                      swapchain_image_resources[image_index].command_buffer_fence);

  VkPresentInfoKHR present_info = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = ren_com_semaphore,
    .swapchainCount = 1,
    .pSwapchains = &swapchain,
    .pImageIndices = &image_index,
    .pNext = NULL,
  };

  err = vkQueuePresentKHR(queue, &present_info);

  pthread_mutex_unlock(&scene_lock); // ??
  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
    ont_vk_restart();
  }
}

// ---------------------------------------------------------------------------------------

#if defined(VK_USE_PLATFORM_XCB_KHR) // Ubuntu Desktop; Pi4
//static char* font_face = "/usr/share/fonts/truetype/open-sans/OpenSans-Regular.ttf";
static char* font_face = "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf";
#endif

static char *texture_files[] = {"ivory.ppm"};

#include "ivory.ppm.h"

#define TEXTURE_COUNT 1

struct texture_object textures[TEXTURE_COUNT];
struct texture_object staging_texture;

// ---------------------------------

static bool memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex) {
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        if ((typeBits & 1) == 1) {
            if ((memory_properties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    return false;
}

static uint32_t create_buffer_with_memory(VkBufferCreateInfo*   buffer_ci,
                                          VkMemoryPropertyFlags prop_flags,
                                          VkBuffer*             buffer,
                                          VkDeviceMemory*       memory){

  VK_CHECK(vkCreateBuffer(device, buffer_ci, 0, buffer));

  VkMemoryRequirements mem_reqs;
  vkGetBufferMemoryRequirements(device, *buffer, &mem_reqs);

  VkMemoryAllocateInfo memory_ai = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = mem_reqs.size,
  };
  assert(memory_type_from_properties(mem_reqs.memoryTypeBits,
                                     prop_flags,
                                     &memory_ai.memoryTypeIndex));

  VK_CHECK(vkAllocateMemory(device, &memory_ai, 0, memory));

  VK_CHECK(vkBindBufferMemory(device, *buffer, *memory, 0));

  return memory_ai.allocationSize;
}

static uint32_t create_image_with_memory(VkImageCreateInfo*    image_ci,
                                         VkMemoryPropertyFlags prop_flags,
                                         VkImage*              image,
                                         VkDeviceMemory*       memory) {

  VK_CHECK(vkCreateImage(device, image_ci, 0, image));

  VkMemoryRequirements mem_reqs;
  vkGetImageMemoryRequirements(device, *image, &mem_reqs);

  VkMemoryAllocateInfo memory_ai = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = mem_reqs.size,
  };
  assert(memory_type_from_properties(mem_reqs.memoryTypeBits,
                                     prop_flags,
                                     &memory_ai.memoryTypeIndex));

  VK_CHECK(vkAllocateMemory(device, &memory_ai, 0, memory));

  VK_CHECK(vkBindImageMemory(device, *image, *memory, 0));

  return memory_ai.allocationSize;
}

static void create_uniform_buffer_with_memory(VkBufferCreateInfo* buffer_ci,
                                              VkMemoryPropertyFlags prop_flags,
                                              uint32_t ii){
    VK_CHECK(vkCreateBuffer(device,
                            buffer_ci,
                            0,
                            &uniform_mem[ii].uniform_buffer
    ));

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(
                            device,
                            uniform_mem[ii].uniform_buffer,
                            &mem_reqs);

    VkMemoryAllocateInfo memory_ai = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_reqs.size,
    };
    assert(memory_type_from_properties(mem_reqs.memoryTypeBits,
                                       prop_flags,
                                       &memory_ai.memoryTypeIndex));

    VK_CHECK(vkAllocateMemory(device,
                             &memory_ai,
                              0,
                             &uniform_mem[ii].uniform_memory));

    VK_CHECK(vkMapMemory(device,
                         uniform_mem[ii].uniform_memory,
                         0, sizeof(struct uniforms), 0,
                        &uniform_mem[ii].uniform_memory_ptr));

    VK_CHECK(vkBindBufferMemory(device,
                                uniform_mem[ii].uniform_buffer,
                                uniform_mem[ii].uniform_memory,
                                0));
}

// ---------------------------------

static bool load_texture(const char *filename, uint8_t *rgba_data, uint64_t row_pitch, int32_t *w, int32_t *h) {
    (void)filename;
    char *cPtr;
    cPtr = (char *)texture_array;
    if ((unsigned char *)cPtr >= (texture_array + texture_len) || strncmp(cPtr, "P6\n", 3)) {
        return false;
    }
    while (strncmp(cPtr++, "\n", 1)) ;
    sscanf(cPtr, "%u %u", w, h);

    if(!rgba_data) return true;

    while (strncmp(cPtr++, "\n", 1))
        ;
    if ((unsigned char *)cPtr >= (texture_array + texture_len) || strncmp(cPtr, "255\n", 4)) {
        return false;
    }
    while (strncmp(cPtr++, "\n", 1))
        ;
    for (int y = 0; y < *h; y++) {
        uint8_t *rowPtr = rgba_data;
        for (int x = 0; x < *w; x++) {
            memcpy(rowPtr, cPtr, 3);
            rowPtr[3] = 255; /* Alpha of 1 */
            rowPtr += 4;
            cPtr += 3;
        }
        rgba_data += row_pitch;
    }
    return true;
}

extern unsigned char tests_ont_examples_vulkan_onx_frag_spv[];
extern unsigned int  tests_ont_examples_vulkan_onx_frag_spv_len;
extern unsigned char tests_ont_examples_vulkan_onx_vert_spv[];
extern unsigned int  tests_ont_examples_vulkan_onx_vert_spv_len;

static VkShaderModule load_c_shader(bool load_frag) {

    VkShaderModuleCreateInfo module_ci = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = load_frag? tests_ont_examples_vulkan_onx_frag_spv_len:
                               tests_ont_examples_vulkan_onx_vert_spv_len,
        .pCode = load_frag? tests_ont_examples_vulkan_onx_frag_spv:
                            tests_ont_examples_vulkan_onx_vert_spv,
        .flags = 0,
        .pNext = 0,
    };

    VkShaderModule module;
    VK_CHECK(vkCreateShaderModule(device,
                                  &module_ci,
                                  0,
                                  &module));
    return module;
}

// ---------------------------------

static void prepare_texture_image(const char *filename,
                                  struct texture_object *texture_obj,
                                  VkImageTiling tiling,
                                  VkImageUsageFlags usage,
                                  VkFlags prop_flags) {
    int32_t texture_width;
    int32_t texture_height;
    VkResult err;

    if (!load_texture(filename, NULL, 0, &texture_width, &texture_height)) {
        ERR_EXIT("Failed to load textures");
    }

    texture_obj->texture_width = texture_width;
    texture_obj->texture_height = texture_height;

    VkImageCreateInfo image_ci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = texture_format,
        .extent = {texture_width, texture_height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .flags = 0,
        .initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED,
    };

    uint32_t size = create_image_with_memory(&image_ci,
                                             prop_flags,
                                             &texture_obj->image,
                                             &texture_obj->device_memory);

    if (prop_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        const VkImageSubresource subres = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .arrayLayer = 0,
        };
        VkSubresourceLayout layout;

        vkGetImageSubresourceLayout(device, texture_obj->image, &subres, &layout);

        void *data;
        VK_CHECK(vkMapMemory(device, texture_obj->device_memory, 0, size, 0, &data));

        if (!load_texture(filename, data, layout.rowPitch, &texture_width, &texture_height)) {
            log_write("Error loading texture: %s\n", filename);
        }
        vkUnmapMemory(device, texture_obj->device_memory);
    }
    texture_obj->image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

static void prepare_texture_buffer(const char *filename, struct texture_object *texture_obj) {

    int32_t texture_width;
    int32_t texture_height;
    VkResult err;

    if (!load_texture(filename, 0, 0, &texture_width, &texture_height)) {
        ERR_EXIT("Failed to load textures");
    }

    texture_obj->texture_width = texture_width;
    texture_obj->texture_height = texture_height;

    VkBufferCreateInfo buffer_ci = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      .flags = 0,
      .size = texture_width * texture_height * 4,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = NULL,
      .pNext = 0,
    };

    VkFlags prop_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    uint32_t size = create_buffer_with_memory(&buffer_ci,
                                              prop_flags,
                                              &texture_obj->buffer,
                                              &texture_obj->device_memory);
    VkSubresourceLayout layout;
    memset(&layout, 0, sizeof(layout));
    layout.rowPitch = texture_width * 4;

    void *data;
    err = vkMapMemory(device, texture_obj->device_memory, 0, size, 0, &data);
    assert(!err);

    if (!load_texture(filename, data, layout.rowPitch, &texture_width, &texture_height)) {
        log_write("Error loading texture: %s\n", filename);
    }

    vkUnmapMemory(device, texture_obj->device_memory);
}

static void prepare_depth() {

    const VkFormat depth_format = VK_FORMAT_D16_UNORM;

    VkImageCreateInfo image_ci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depth_format,
        .extent = { io.swap_width, io.swap_height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .flags = 0,
    };

    depth.format = depth_format;

    VkMemoryPropertyFlags prop_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    create_image_with_memory(&image_ci,
                             prop_flags,
                             &depth.image,
                             &depth.device_memory);

    VkImageViewCreateInfo image_view_ci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .image = depth.image,
        .format = depth_format,
        .subresourceRange = {
           .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
           .baseMipLevel   = 0,
           .levelCount     = 1,
           .baseArrayLayer = 0,
           .layerCount     = 1,
        },
        .flags = 0,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
    };

    VK_CHECK(vkCreateImageView(device, &image_view_ci, NULL, &depth.image_view));
}

static void prepare_textures(){

    uint32_t i;

    for (i = 0; i < TEXTURE_COUNT; i++) {
        VkResult err;

#if defined(LIMIT_TO_LINEAR_AND_NO_STAGING_BUFFER)
        if ((format_properties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {

            prepare_texture_image(texture_files[i], &textures[i], VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            transition_image(initcmd, textures[i].image,
                               VK_IMAGE_LAYOUT_PREINITIALIZED,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                               0,
                               VK_ACCESS_SHADER_READ_BIT |
                               VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
                               VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            staging_texture.image = 0;
        } else
#endif
        if (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {

            memset(&staging_texture, 0, sizeof(staging_texture));
            prepare_texture_buffer(texture_files[i], &staging_texture);

            prepare_texture_image(texture_files[i], &textures[i], VK_IMAGE_TILING_OPTIMAL,
                                       (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            transition_image(initcmd, textures[i].image,
                               VK_IMAGE_LAYOUT_PREINITIALIZED,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               0, VK_ACCESS_TRANSFER_WRITE_BIT,
                               VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                               VK_PIPELINE_STAGE_TRANSFER_BIT);

            VkBufferImageCopy copy_region = {
                .bufferOffset = 0,
                .bufferRowLength = staging_texture.texture_width,
                .bufferImageHeight = staging_texture.texture_height,
                .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                .imageOffset = {0, 0, 0},
                .imageExtent = {staging_texture.texture_width, staging_texture.texture_height, 1},
            };

            vkCmdCopyBufferToImage(initcmd, staging_texture.buffer, textures[i].image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

            transition_image(initcmd, textures[i].image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                               VK_ACCESS_TRANSFER_WRITE_BIT,
                               VK_ACCESS_SHADER_READ_BIT |
                               VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
                               VK_PIPELINE_STAGE_TRANSFER_BIT,
                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        } else {
            assert(!"No support for R8G8B8A8_UNORM as texture image format");
        }

        VkSamplerCreateInfo sampler_ci = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = NULL,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 1,
            .compareOp = VK_COMPARE_OP_NEVER,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
            .unnormalizedCoordinates = VK_FALSE,
        };

        VkImageViewCreateInfo image_view_ci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .image = VK_NULL_HANDLE,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = texture_format,
            .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
            .subresourceRange = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
            .flags = 0,
        };

        err = vkCreateSampler(device, &sampler_ci, NULL, &textures[i].sampler);
        assert(!err);

        image_view_ci.image = textures[i].image;
        VK_CHECK(vkCreateImageView(device, &image_view_ci, NULL, &textures[i].image_view));
    }
}

static void prepare_vertex_buffers(){

  VkBufferCreateInfo buffer_ci = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = MAX_PANELS * 6*6 * (3 * sizeof(float) +
                                2 * sizeof(float)  ),
    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  create_buffer_with_memory(&buffer_ci,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            &vertex_buffer,
                            &vertex_buffer_memory);
}

// -------------------------------------------------------------------------------------

void onx_vk_prepare_swapchain_images(bool restart) {
    VkResult err;
    err = vkGetSwapchainImagesKHR(device, swapchain, &image_count, NULL);
    assert(!err);

    VkImage *swapchainImages = (VkImage *)malloc(image_count * sizeof(VkImage));
    assert(swapchainImages);
    err = vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchainImages);
    assert(!err);

    swapchain_image_resources = (SwapchainImageResources *)malloc(sizeof(SwapchainImageResources) * image_count);

    for (uint32_t i = 0; i < image_count; i++) {
        VkImageViewCreateInfo image_view_ci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchainImages[i],
            .pNext = NULL,
            .format = surface_format,
            .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
            .subresourceRange = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .flags = 0,
        };

        swapchain_image_resources[i].image = swapchainImages[i];
        VK_CHECK(vkCreateImageView(device, &image_view_ci, NULL,
                                   &swapchain_image_resources[i].image_view));
    }

    if (NULL != swapchainImages) {
        free(swapchainImages);
    }
}

void onx_vk_prepare_semaphores_and_fences(bool restart) {

  VkFenceCreateInfo fence_ci = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
      .pNext = 0,
  };

  for (uint32_t i = 0; i < image_count; i++) {
      VK_CHECK(vkCreateFence(device, &fence_ci, 0, &swapchain_image_resources[i].command_buffer_fence));
  }

  VkSemaphoreCreateInfo semaphore_ci = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = 0,
  };

  VK_CHECK(vkCreateSemaphore(device, &semaphore_ci, 0, &image_acquired_semaphore));
  VK_CHECK(vkCreateSemaphore(device, &semaphore_ci, 0, &render_complete_semaphore));
}

void onx_vk_prepare_command_buffers(bool restart){

  VkCommandBufferAllocateInfo command_buffer_ai = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
      .pNext = 0,
  };

  for (uint32_t i = 0; i < image_count; i++) {
      VK_CHECK(vkAllocateCommandBuffers(
                       device,
                       &command_buffer_ai,
                       &swapchain_image_resources[i].command_buffer
      ));
  }
}

void onx_vk_prepare_render_data(bool restart) {

  VkPhysicalDeviceProperties gpu_props;
  vkGetPhysicalDeviceProperties(gpu, &gpu_props);

  vkGetPhysicalDeviceFormatProperties(gpu, texture_format, &format_properties);
  vkGetPhysicalDeviceMemoryProperties(gpu, &memory_properties);

  prepare_depth();
  prepare_textures();
}

void onx_vk_prepare_uniform_buffers(bool restart) {

  prepare_vertex_buffers();

  uniform_mem = (uniform_mem_t*)malloc(sizeof(uniform_mem_t) * image_count);

  VkBufferCreateInfo buffer_ci = {
     .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
     .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
     .size = sizeof(struct uniforms),
  };
  for (uint32_t i = 0; i < image_count; i++) {

    create_uniform_buffer_with_memory(&buffer_ci,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      i);
  }
}

void onx_vk_prepare_descriptor_layout(bool restart) {

  VkDescriptorSetLayoutBinding bindings[] = {
      {
          .binding = 0,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
          .pImmutableSamplers = NULL,
      },
      {
          .binding = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = TEXTURE_COUNT,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
          .pImmutableSamplers = NULL,
      },
  };

  VkDescriptorSetLayoutCreateInfo descriptor_set_layout_ci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 2,
      .pBindings = bindings,
  };

  VK_CHECK(vkCreateDescriptorSetLayout(device,
                                       &descriptor_set_layout_ci,
                                       0,
                                       &descriptor_layout));
}

void onx_vk_prepare_pipeline_layout(bool restart) {

  VkPushConstantRange push_constant_range = {
    .offset = 0,
    .size = sizeof(struct push_constants),
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
  };

  VkPipelineLayoutCreateInfo pipeline_layout_ci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptor_layout,
      .pPushConstantRanges = &push_constant_range,
      .pushConstantRangeCount = 1,
      .pNext = 0,
  };

  VK_CHECK(vkCreatePipelineLayout(device,
                                  &pipeline_layout_ci,
                                  0,
                                  &pipeline_layout));
}

void onx_vk_prepare_descriptor_pool(bool restart) {

  VkDescriptorPoolSize pool_sizes[] = {
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        image_count                 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        image_count * 3             },
      { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        image_count * TEXTURE_COUNT },
  };

  VkDescriptorPoolCreateInfo descriptor_pool_ci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = (image_count * 3) + image_count + (image_count * TEXTURE_COUNT) + 23, //// ??
      .poolSizeCount = 3,
      .pPoolSizes = pool_sizes,
      .pNext = 0,
  };

  VK_CHECK(vkCreateDescriptorPool(device, &descriptor_pool_ci, NULL, &descriptor_pool));
}

void onx_vk_prepare_descriptor_set(bool restart) {

  VkDescriptorSetAllocateInfo descriptor_set_ai = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptor_pool,
    .descriptorSetCount = 1,
    .pSetLayouts = &descriptor_layout,
    .pNext = 0,
  };

  VkDescriptorBufferInfo uniform_info = {
    .offset = 0,
    .range = sizeof(struct uniforms),
  };

  VkDescriptorImageInfo texture_descs[TEXTURE_COUNT];
  memset(&texture_descs, 0, sizeof(texture_descs));

  for (unsigned int i = 0; i < TEXTURE_COUNT; i++) {
    texture_descs[i].sampler = textures[i].sampler;
    texture_descs[i].imageView = textures[i].image_view;
    texture_descs[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }

  VkWriteDescriptorSet writes[] = {
    {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .dstBinding = 0,
      .pBufferInfo = &uniform_info,
      .descriptorCount = 1,
    },
    {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .dstBinding = 1,
      .pImageInfo = texture_descs,
      .descriptorCount = TEXTURE_COUNT,
    },
  };

  for (uint32_t i = 0; i < image_count; i++) {

    VK_CHECK(vkAllocateDescriptorSets(
                              device,
                             &descriptor_set_ai,
                             &uniform_mem[i].descriptor_set));

    uniform_info.buffer = uniform_mem[i].uniform_buffer;
    writes[0].dstSet = uniform_mem[i].descriptor_set;
    writes[1].dstSet = uniform_mem[i].descriptor_set;

    vkUpdateDescriptorSets(device, 2, writes, 0, 0);
  }
}

void onx_vk_prepare_render_pass(bool restart) {
    const VkAttachmentDescription attachments[2] = {
            {
                .format = surface_format,
                .flags = 0,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            },
            {
                .format = depth.format,
                .flags = 0,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            },
    };
    const VkAttachmentReference color_reference = {
        .attachment = 0, // this refers to the color attachment
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkAttachmentReference depth_reference = {
        .attachment = 1, // this refers to the depth attachment
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .flags = 0,
        .inputAttachmentCount = 0,
        .pInputAttachments = NULL,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_reference,
        .pResolveAttachments = NULL,
        .pDepthStencilAttachment = &depth_reference,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = NULL,
    };

    VkSubpassDependency attachmentDependencies[2] = {
            {
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                .dependencyFlags = 0,
            },
            {
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dependencyFlags = 0,
            },
    };

    VkRenderPassCreateInfo rp_ci = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 2,
        .pDependencies = attachmentDependencies,
    };


    VkResult err;
    err = vkCreateRenderPass(device, &rp_ci, NULL, &render_pass);
    assert(!err);
}

void onx_vk_prepare_pipeline(bool restart) {

  VkShaderModule vert_shader_module = load_c_shader(false);
  VkShaderModule frag_shader_module = load_c_shader(true);

  VkPipelineShaderStageCreateInfo shader_stages_ci[] = {
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vert_shader_module,
      .pName = "main",
    },
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = frag_shader_module,
      .pName = "main",
    }
  };

  VkVertexInputBindingDescription vertices_input_binding = {
    .binding = 0,
    .stride = 3 * sizeof(float) +
              2 * sizeof(float),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };

  VkVertexInputAttributeDescription vertex_input_attributes[] = {
    { 0, 0, VK_FORMAT_R32G32B32_SFLOAT,  0 },  // vertex
    { 1, 0, VK_FORMAT_R32G32_SFLOAT,    12 },  // uv
  };

  VkVertexInputBindingDescription vibds[] = {
    vertices_input_binding,
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_state_ci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = vibds,
      .vertexAttributeDescriptionCount = 2,
      .pVertexAttributeDescriptions = vertex_input_attributes,
  };

  VkPipelineInputAssemblyStateCreateInfo input_assembly_state_ci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  float width  = io.swap_width;
  float height = io.swap_height;

  VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width  = width,
      .height = height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };

  VkRect2D scissor = {
      .offset = {
         0,
         0
      },
      .extent = {
         width,
         height,
      },
  };

  VkViewport viewports[] = { viewport };
  VkRect2D   scissors[]  = { scissor };

  VkPipelineViewportStateCreateInfo viewport_state_ci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1,
      .pViewports = viewports,
      .pScissors = scissors,
  };

  VkPipelineRasterizationStateCreateInfo rasterizer_state_ci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f,
  };

  VkPipelineMultisampleStateCreateInfo multisample_ci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
      .pSampleMask = NULL,
  };

  VkPipelineColorBlendAttachmentState color_blend_as = {
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                        VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT |
                        VK_COLOR_COMPONENT_A_BIT,
      .blendEnable = VK_TRUE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA,
      .alphaBlendOp = VK_BLEND_OP_MAX,
  };

  VkPipelineColorBlendStateCreateInfo blend_state_ci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &color_blend_as,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY, // enabled?
  };

  VkPipelineDepthStencilStateCreateInfo depth_stencil_ci = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
    .depthBoundsTestEnable = VK_FALSE,
    .back.failOp = VK_STENCIL_OP_KEEP,
    .back.passOp = VK_STENCIL_OP_KEEP,
    .back.compareOp = VK_COMPARE_OP_ALWAYS,
    .stencilTestEnable = VK_FALSE,
  };
  depth_stencil_ci.front = depth_stencil_ci.back;

  VkPipelineCacheCreateInfo pipeline_cache_ci = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
  };

  VK_CHECK(vkCreatePipelineCache(device,
                                 &pipeline_cache_ci,
                                 0,
                                 &pipeline_cache));

  VkGraphicsPipelineCreateInfo graphics_pipeline_ci = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pViewportState = &viewport_state_ci,
    .pRasterizationState = &rasterizer_state_ci,
    .pMultisampleState = &multisample_ci,
    .pColorBlendState = &blend_state_ci,
    .pDepthStencilState = &depth_stencil_ci,
    .stageCount = 2,
    .pStages = shader_stages_ci,
    .pVertexInputState = &vertex_input_state_ci,
    .pInputAssemblyState = &input_assembly_state_ci,
    .layout = pipeline_layout,
    .renderPass = render_pass,
    .subpass = 0,
  };

  VK_CHECK(vkCreateGraphicsPipelines(device,
                                     pipeline_cache,
                                     1,
                                     &graphics_pipeline_ci,
                                     0,
                                     &pipeline));

  vkDestroyShaderModule(device, frag_shader_module, NULL);
  vkDestroyShaderModule(device, vert_shader_module, NULL);
}

void onx_vk_prepare_framebuffers(bool restart) {

    VkFramebufferCreateInfo fb_ci = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = 0,
        .renderPass = render_pass,
        .attachmentCount = 2,
        .width =  io.swap_width,
        .height = io.swap_height,
        .layers = 1,
    };

    for (uint32_t i = 0; i < image_count; i++) {

        VkImageView attachments[] = {
          swapchain_image_resources[i].image_view,
          depth.image_view,
        };
        fb_ci.pAttachments = attachments;

        VkResult err = vkCreateFramebuffer(device, &fb_ci, 0,
                                           &swapchain_image_resources[i].framebuffer);
        assert(!err);
    }
}

// ----------------------------------------------------------------------------------------

void onx_vk_finish() {

  scene_ready = false;

  for (uint32_t i = 0; i < image_count; i++) {
    vkWaitForFences(device, 1, &swapchain_image_resources[i].command_buffer_fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(device, swapchain_image_resources[i].command_buffer_fence, NULL);
  }

  vkDestroyPipeline(device, pipeline, NULL);
  vkDestroyPipelineCache(device, pipeline_cache, NULL);

  // ---------------------------------

  vkFreeMemory(device, vertex_buffer_memory, NULL);
  vkFreeMemory(device, staging_buffer_memory, NULL);

  vkDestroyBuffer(device, vertex_buffer, NULL);
  vkDestroyBuffer(device, staging_buffer, NULL);

  // ---------------------------------

  vkDestroyDescriptorPool(device, descriptor_pool, NULL);

  // ---------------------------------

  vkDestroyPipelineLayout(device, pipeline_layout, NULL);
  vkDestroyDescriptorSetLayout(device, descriptor_layout, NULL);

  // ---------------------------------

  for (uint32_t i = 0; i < image_count; i++) {
      vkDestroyBuffer(device, uniform_mem[i].uniform_buffer, NULL);
      vkUnmapMemory(device, uniform_mem[i].uniform_memory);
      vkFreeMemory(device, uniform_mem[i].uniform_memory, NULL);
  }
  free(uniform_mem);

  // ---------------------------------

  for (uint32_t i = 0; i < TEXTURE_COUNT; i++) {
    vkDestroyImageView(device, textures[i].image_view, NULL);
    vkDestroyImage(device, textures[i].image, NULL);
    vkFreeMemory(device, textures[i].device_memory, NULL);
    vkDestroySampler(device, textures[i].sampler, NULL);
  }
  if(staging_texture.buffer) {
     vkFreeMemory(device, staging_texture.device_memory, NULL);
     if(staging_texture.image) vkDestroyImage(device, staging_texture.image, NULL);
     vkDestroyBuffer(device, staging_texture.buffer, NULL);
  }

  vkDestroyImageView(device, depth.image_view, NULL);
  vkDestroyImage(device, depth.image, NULL);
  vkFreeMemory(device, depth.device_memory, NULL);

  uint32_t i;
  if (swapchain_image_resources) {
     for (i = 0; i < image_count; i++) {
         vkFreeCommandBuffers(device, command_pool, 1, &swapchain_image_resources[i].command_buffer);
         vkDestroyFramebuffer(device, swapchain_image_resources[i].framebuffer, NULL);
         vkDestroyImageView(device, swapchain_image_resources[i].image_view, NULL);
     }
     free(swapchain_image_resources);
  }

  // ---------------------------------

  VK_DESTROY(vkDestroySemaphore, device, image_acquired_semaphore);
  VK_DESTROY(vkDestroySemaphore, device, render_complete_semaphore);

  vkDestroyRenderPass(device, render_pass, NULL);
}

// ---------------------------------
