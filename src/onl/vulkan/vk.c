
/* Platform-independent Vulkan common code */

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

#include "onl-vk.h"
#include "onl/vulkan/object_type_string_helper.h"
#include "onl/vulkan/vk.h"

#include "inttypes.h"

bool validate = true;

VkSurfaceKHR surface;
bool prepared;
uint16_t frames = 0;
int32_t gpu_number = -1;
VkInstance inst;
VkPhysicalDevice gpu;
VkDevice device;
VkQueue queue;
uint32_t queue_family_index;
VkQueueFamilyProperties *queue_props;
uint32_t enabled_extension_count;
uint32_t enabled_layer_count;
char *extension_names[64];
VkFormat surface_format;
VkColorSpaceKHR color_space;

VkSwapchainKHR swapchain;
VkExtent2D swapchain_extent;
VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
VkCommandPool command_pool;
VkCommandBuffer initcmd;
uint32_t queue_family_count;

PFN_vkCreateDebugUtilsMessengerEXT  vxCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT vxDestroyDebugUtilsMessengerEXT;

VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_ci;
VkDebugUtilsMessengerEXT           dbg_messenger;

static int validation_error = 0;

static char const *gpu_type_to_string(VkPhysicalDeviceType const type) {
    switch (type) {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            return "Other";
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return "IntegratedGpu";
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return "DiscreteGpu";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return "VirtualGpu";
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return "Cpu";
        default:
            return "Unknown";
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(
                                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                VkDebugUtilsMessageTypeFlagsEXT messageType,
                                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                void *pUserData) {
    char prefix[64] = "";
    char *message = (char *)malloc(strlen(pCallbackData->pMessage) + 5000);
    assert(message);

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        strcat(prefix, "VERBOSE : ");
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        strcat(prefix, "INFO : ");
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        strcat(prefix, "WARNING : ");
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        strcat(prefix, "ERROR : ");
    }

    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
        strcat(prefix, "GENERAL");
    } else {
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
            strcat(prefix, "VALIDATION");
            validation_error = 1;
        }
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
            if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
                strcat(prefix, "|");
            }
            strcat(prefix, "PERFORMANCE");
        }
    }

    if(!strcmp(pCallbackData->pMessageIdName, "Loader Message") ||
       !strcmp(pCallbackData->pMessageIdName, "WARNING-CreateInstance-status-message")){

      free(message);
      return false;
    }

    sprintf(message,
            "%s - Message Id Number: %d | Message Id Name: %s\n\t%s\n",
            prefix,
            pCallbackData->messageIdNumber,
            pCallbackData->pMessageIdName,
            pCallbackData->pMessage);

    if (pCallbackData->objectCount > 0) {

        char tmp_message[500];
        sprintf(tmp_message, "\n\tObjects - %d\n", pCallbackData->objectCount);
        strcat(message, tmp_message);
        for (uint32_t object = 0; object < pCallbackData->objectCount; ++object) {
            if(pCallbackData->pObjects[object].pObjectName &&
               strlen(pCallbackData->pObjects[object].pObjectName) > 0) {
              sprintf(tmp_message,
                       "\t\tObject[%d] - %s, Handle %p, Name \"%s\"\n",
                        object,
                        string_VkObjectType(pCallbackData->pObjects[object].objectType),
                        (void *)(pCallbackData->pObjects[object].objectHandle),
                        pCallbackData->pObjects[object].pObjectName);
            } else {
                sprintf(tmp_message, "\t\tObject[%d] - %s, Handle %p\n", object,
                        string_VkObjectType(pCallbackData->pObjects[object].objectType),
                        (void *)(pCallbackData->pObjects[object].objectHandle));
            }
            strcat(message, tmp_message);
        }
    }
    if (pCallbackData->cmdBufLabelCount > 0) {

        char tmp_message[500];

        sprintf(tmp_message,
                "\n\tCommand Buffer Labels - %d\n",
                pCallbackData->cmdBufLabelCount);

        strcat(message, tmp_message);

        for (uint32_t cmd_buf_label = 0;
             cmd_buf_label < pCallbackData->cmdBufLabelCount;
             ++cmd_buf_label) {

            sprintf(tmp_message,
                    "\t\tLabel[%d] - %s { %f, %f, %f, %f}\n",
                    cmd_buf_label,
                    pCallbackData->pCmdBufLabels[cmd_buf_label].pLabelName,
                    pCallbackData->pCmdBufLabels[cmd_buf_label].color[0],
                    pCallbackData->pCmdBufLabels[cmd_buf_label].color[1],
                    pCallbackData->pCmdBufLabels[cmd_buf_label].color[2],
                    pCallbackData->pCmdBufLabels[cmd_buf_label].color[3]);

            strcat(message, tmp_message);
        }
    }

    log_write("%s\n", message);
    fflush(stdout);

    free(message);

    // Don't bail out, but keep going.
    return false;
}

static void prepare_swapchain() {
    VkSwapchainKHR oldSwapchain = swapchain;

    VkSurfaceCapabilitiesKHR surfCapabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surfCapabilities));

    uint32_t presentModeCount;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, NULL));
    VkPresentModeKHR *presentModes = (VkPresentModeKHR *)malloc(presentModeCount * sizeof(VkPresentModeKHR));
    assert(presentModes);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, presentModes));

    VkPhysicalDeviceMultiviewFeaturesKHR extFeatures = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR,
    };
/*
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(gpu, &deviceFeatures);

    log_write("multiViewport = %d\n", deviceFeatures.multiViewport);
*/
    VkPhysicalDeviceFeatures2KHR deviceFeatures2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR,
      .pNext = &extFeatures,
    };
    vkGetPhysicalDeviceFeatures2(gpu, &deviceFeatures2);

    VkPhysicalDeviceMultiviewPropertiesKHR extProps = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES_KHR,
    };
    VkPhysicalDeviceProperties2KHR deviceProps2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR,
      .pNext = &extProps,
    };
    vkGetPhysicalDeviceProperties2(gpu, &deviceProps2);
/*
    log_write("Multiview features:\n");
    log_write("  multiview = %d\n", extFeatures.multiview);
    log_write("  multiviewGeometryShader =  %d\n", extFeatures.multiviewGeometryShader);
    log_write("  multiviewTessellationShader =  %d\n", extFeatures.multiviewTessellationShader);
    log_write("Multiview properties:\n");
    log_write("  maxMultiviewViewCount = %d\n", extProps.maxMultiviewViewCount);
    log_write("  maxMultiviewInstanceIndex = %d\n", extProps.maxMultiviewInstanceIndex);
*/
    // width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
    if (surfCapabilities.currentExtent.width == 0xFFFFFFFF) {
        // If the surface size is undefined, the size is set to the size
        // of the images requested, which must fit within the minimum and
        // maximum values.
        swapchain_extent.width  = io.swap_width;
        swapchain_extent.height = io.swap_height;

        if (swapchain_extent.width < surfCapabilities.minImageExtent.width) {
            swapchain_extent.width = surfCapabilities.minImageExtent.width;
        } else if (swapchain_extent.width > surfCapabilities.maxImageExtent.width) {
            swapchain_extent.width = surfCapabilities.maxImageExtent.width;
        }

        if (swapchain_extent.height < surfCapabilities.minImageExtent.height) {
            swapchain_extent.height = surfCapabilities.minImageExtent.height;
        } else if (swapchain_extent.height > surfCapabilities.maxImageExtent.height) {
            swapchain_extent.height = surfCapabilities.maxImageExtent.height;
        }
    } else {
        // If the surface size is defined, the swapchain size must match
        swapchain_extent = surfCapabilities.currentExtent;
        io.swap_width = surfCapabilities.currentExtent.width;
        io.swap_height = surfCapabilities.currentExtent.height;
    }

    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    if (presentMode != swapchainPresentMode) {
        for (size_t i = 0; i < presentModeCount; ++i) {
            if (presentModes[i] == presentMode) {
                swapchainPresentMode = presentMode;
                break;
            }
        }
    }
    if (presentMode != swapchainPresentMode) {
        ERR_EXIT("Present mode specified is not supported\n");
    }

    uint32_t desiredNumOfSwapchainImages = 3;
    if (desiredNumOfSwapchainImages < surfCapabilities.minImageCount) {
        desiredNumOfSwapchainImages = surfCapabilities.minImageCount;
    }
    if ((surfCapabilities.maxImageCount > 0) && (desiredNumOfSwapchainImages > surfCapabilities.maxImageCount)) {
        desiredNumOfSwapchainImages = surfCapabilities.maxImageCount;
    }

    VkSurfaceTransformFlagsKHR preTransform;
    if (surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = surfCapabilities.currentTransform;
    }

    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (uint32_t i = 0; i < ARRAY_SIZE(compositeAlphaFlags); i++) {
        if (surfCapabilities.supportedCompositeAlpha & compositeAlphaFlags[i]) {
            compositeAlpha = compositeAlphaFlags[i];
            break;
        }
    }

    VkSwapchainCreateInfoKHR swapchain_ci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .surface = surface,
        .minImageCount = desiredNumOfSwapchainImages,
        .imageFormat = surface_format,
        .imageColorSpace = color_space,
        .imageExtent =
            {
                .width = swapchain_extent.width,
                .height = swapchain_extent.height,
            },
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .preTransform = preTransform,
        .compositeAlpha = compositeAlpha,
        .imageArrayLayers = 1,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .presentMode = swapchainPresentMode,
        .oldSwapchain = oldSwapchain,
        .clipped = true,
    };
    VK_CHECK(vkCreateSwapchainKHR(device, &swapchain_ci, NULL, &swapchain));

    // If we just re-created an existing swapchain, we should destroy the old
    // swapchain at this point.
    // Note: destroying the swapchain also cleans up all its associated
    // presentable images once the platform is done with them.
    if (oldSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, oldSwapchain, NULL);
    }

    if (NULL != presentModes) {
        free(presentModes);
    }
}

static void prepare_command_pools()
{
    VkResult err;
    if (command_pool == VK_NULL_HANDLE) {
        const VkCommandPoolCreateInfo cmd_pool_ci = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = NULL,
            .queueFamilyIndex = queue_family_index,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        };
        err = vkCreateCommandPool(device, &cmd_pool_ci, NULL, &command_pool);
        assert(!err);
    }
}

static void begin_command_buffer() {

    const VkCommandBufferAllocateInfo cb_ai = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkResult err;
    err = vkAllocateCommandBuffers(device, &cb_ai, &initcmd);
    assert(!err);

    VkCommandBufferBeginInfo cmd_buf_bi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL,
    };
    err = vkBeginCommandBuffer(initcmd, &cmd_buf_bi);
    assert(!err);
}

static void end_command_buffer() {
    VkResult err;

    // This function could get called twice if the texture uses a staging buffer
    // In that case the second call should be ignored
    if (initcmd == VK_NULL_HANDLE) return;

    err = vkEndCommandBuffer(initcmd);
    assert(!err);

    VkFence fence;
    VkFenceCreateInfo fence_ci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = NULL, .flags = 0};
    err = vkCreateFence(device, &fence_ci, NULL, &fence);
    assert(!err);

    const VkCommandBuffer cmd_bufs[] = {initcmd};
    VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                .pNext = NULL,
                                .waitSemaphoreCount = 0,
                                .pWaitSemaphores = NULL,
                                .pWaitDstStageMask = NULL,
                                .commandBufferCount = 1,
                                .pCommandBuffers = cmd_bufs,
                                .signalSemaphoreCount = 0,
                                .pSignalSemaphores = NULL};

    err = vkQueueSubmit(queue, 1, &submit_info, fence);
    assert(!err);

    err = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    assert(!err);

    vkFreeCommandBuffers(device, command_pool, 1, cmd_bufs);
    vkDestroyFence(device, fence, NULL);
    initcmd = VK_NULL_HANDLE;
}

/*
 * Return 1 (true) if all layer names specified in check_names
 * can be found in given layer properties.
 */
static VkBool32 check_layers(uint32_t check_count, char **check_names, uint32_t layer_count, VkLayerProperties *layers) {
    for (uint32_t i = 0; i < check_count; i++) {
        VkBool32 found = 0;
        for (uint32_t j = 0; j < layer_count; j++) {
            if (!strcmp(check_names[i], layers[j].layerName)) {
                found = 1;
                break;
            }
        }
        if (!found) {
            log_write("Cannot find layer: %s\n", check_names[i]);
            return 0;
        }
    }
    return 1;
}

static void create_instance() {
    VkResult err;
    uint32_t instance_extension_count = 0;
    uint32_t instance_layer_count = 0;
    char *instance_validation_layers[] = {
      "VK_LAYER_KHRONOS_validation",
    //"VK_LAYER_LUNARG_api_dump",
    };
    enabled_extension_count = 0;
    enabled_layer_count = 0;
    command_pool = VK_NULL_HANDLE;

    VkBool32 validation_found = 0;
    if (validate) {
        err = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
        assert(!err);

        if (instance_layer_count > 0) {
            VkLayerProperties *instance_layers = malloc(sizeof(VkLayerProperties) * instance_layer_count);
            err = vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers);
            assert(!err);

            validation_found = check_layers(ARRAY_SIZE(instance_validation_layers),
                                            instance_validation_layers,
                                            instance_layer_count,
                                            instance_layers);
            if (validation_found) {
                enabled_layer_count = ARRAY_SIZE(instance_validation_layers);
            }
            free(instance_layers);
        }

        if (!validation_found) {
            ERR_EXIT("vkEnumerateInstanceLayerProperties failed to find required validation layer.\n\n");
        }
    }

    VkBool32 surfaceExtFound = 0;
    VkBool32 platformSurfaceExtFound = 0;
    bool portabilityEnumerationActive = false;
    memset(extension_names, 0, sizeof(extension_names));

    err = vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL);
    assert(!err);

    if (instance_extension_count > 0) {
        VkExtensionProperties *instance_extensions = malloc(sizeof(VkExtensionProperties) * instance_extension_count);
        err = vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, instance_extensions);
        assert(!err);
        for (uint32_t i = 0; i < instance_extension_count; i++) {
            if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName)) {
                surfaceExtFound = 1;
                extension_names[enabled_extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
            }
#if defined(VK_USE_PLATFORM_XCB_KHR)
            if (!strcmp(VK_KHR_XCB_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName)) {
                platformSurfaceExtFound = 1;
                extension_names[enabled_extension_count++] = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
            }
#endif
            if (!strcmp(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, instance_extensions[i].extensionName)) {
                extension_names[enabled_extension_count++] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
            }
            if (!strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, instance_extensions[i].extensionName)) {
                if (validate) {
                    extension_names[enabled_extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
                }
            }
            if (!strcmp(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, instance_extensions[i].extensionName)) {
                portabilityEnumerationActive = true;
                extension_names[enabled_extension_count++] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
            }
            assert(enabled_extension_count < 64);
        }

        free(instance_extensions);
    }

    if (!surfaceExtFound) {
        ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_SURFACE_EXTENSION_NAME " extension.\n\n");
    }
    if (!platformSurfaceExtFound) {
        ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find the platform surface extension.\n\n");
    }
    const VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "onx",
        .applicationVersion = 0,
        .pEngineName = "onx",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_0,
    };
    VkInstanceCreateInfo inst_ci = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = (portabilityEnumerationActive ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0),
        .pApplicationInfo = &app,
        .enabledLayerCount = enabled_layer_count,
        .ppEnabledLayerNames = (const char *const *)instance_validation_layers,
        .enabledExtensionCount = enabled_extension_count,
        .ppEnabledExtensionNames = (const char *const *)extension_names,
    };

    /*
     * This is info for a temp callback to use during CreateInstance.
     * After the instance is created, we use the instance-based
     * function to register the final callback.
     */
    if (validate) {
        // VK_EXT_debug_utils style
        dbg_messenger_ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        dbg_messenger_ci.pNext = NULL;
        dbg_messenger_ci.flags = 0;
        dbg_messenger_ci.messageSeverity =
              VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
              VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dbg_messenger_ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dbg_messenger_ci.pfnUserCallback = debug_messenger_callback;
        dbg_messenger_ci.pUserData = 0;
        inst_ci.pNext = &dbg_messenger_ci;
    }

    err = vkCreateInstance(&inst_ci, NULL, &inst);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
        ERR_EXIT("Cannot find a compatible Vulkan installable client driver (ICD).\n\n");
    } else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) {
        ERR_EXIT("Cannot find a specified extension library.\n");
    } else if (err) {
        ERR_EXIT("vkCreateInstance failed.\n\n");
    }
}

static void pick_physical_device(){
    uint32_t gpu_count = 0;
    VkResult err;
    err = vkEnumeratePhysicalDevices(inst, &gpu_count, NULL);
    assert(!err);

    if (gpu_count <= 0) {
        ERR_EXIT("vkEnumeratePhysicalDevices reported zero accessible devices.\n\n");
    }

    VkPhysicalDevice *physical_devices = malloc(sizeof(VkPhysicalDevice) * gpu_count);
    err = vkEnumeratePhysicalDevices(inst, &gpu_count, physical_devices);
    assert(!err);
    if (gpu_number >= 0 && !((uint32_t)gpu_number < gpu_count)) {
        log_write("GPU %d specified is not present, GPU count = %u\n", gpu_number, gpu_count);
        ERR_EXIT("Specified GPU number is not present");
    }

    if (gpu_number == -1) {
        uint32_t count_device_type[VK_PHYSICAL_DEVICE_TYPE_CPU + 1];
        memset(count_device_type, 0, sizeof(count_device_type));

        VkPhysicalDeviceProperties physicalDeviceProperties;
        for (uint32_t i = 0; i < gpu_count; i++) {
            vkGetPhysicalDeviceProperties(physical_devices[i], &physicalDeviceProperties);
            assert(physicalDeviceProperties.deviceType <= VK_PHYSICAL_DEVICE_TYPE_CPU);
            count_device_type[physicalDeviceProperties.deviceType]++;
        }

        VkPhysicalDeviceType search_for_device_type = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        if (count_device_type[VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU]) {
            search_for_device_type = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        } else if (count_device_type[VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU]) {
            search_for_device_type = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
        } else if (count_device_type[VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU]) {
            search_for_device_type = VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU;
        } else if (count_device_type[VK_PHYSICAL_DEVICE_TYPE_CPU]) {
            search_for_device_type = VK_PHYSICAL_DEVICE_TYPE_CPU;
        } else if (count_device_type[VK_PHYSICAL_DEVICE_TYPE_OTHER]) {
            search_for_device_type = VK_PHYSICAL_DEVICE_TYPE_OTHER;
        }

        for (uint32_t i = 0; i < gpu_count; i++) {
            vkGetPhysicalDeviceProperties(physical_devices[i], &physicalDeviceProperties);
            if (physicalDeviceProperties.deviceType == search_for_device_type) {
                gpu_number = i;
                break;
            }
        }
    }

    assert(gpu_number >= 0);
    gpu = physical_devices[gpu_number];
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(gpu, &physicalDeviceProperties);
        log_write("selected GPU %d: %s, type: %s\n", gpu_number, physicalDeviceProperties.deviceName,
                  gpu_type_to_string(physicalDeviceProperties.deviceType));
    }
    free(physical_devices);

    uint32_t device_extension_count = 0;
    VkBool32 swapchainExtFound = 0;
    enabled_extension_count = 0;
    memset(extension_names, 0, sizeof(extension_names));

    err = vkEnumerateDeviceExtensionProperties(gpu, NULL, &device_extension_count, NULL);
    assert(!err);

    if (device_extension_count > 0) {
        VkExtensionProperties *device_extensions = malloc(sizeof(VkExtensionProperties) * device_extension_count);
        err = vkEnumerateDeviceExtensionProperties(gpu, NULL, &device_extension_count, device_extensions);
        assert(!err);

        for (uint32_t i = 0; i < device_extension_count; i++) {
            if (!strcmp(VK_KHR_MULTIVIEW_EXTENSION_NAME, device_extensions[i].extensionName)) {
                extension_names[enabled_extension_count++] = VK_KHR_MULTIVIEW_EXTENSION_NAME;
            }
            if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, device_extensions[i].extensionName)) {
                swapchainExtFound = 1;
                extension_names[enabled_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
            }
            if (!strcmp("VK_KHR_portability_subset", device_extensions[i].extensionName)) {
                extension_names[enabled_extension_count++] = "VK_KHR_portability_subset";
            }
            assert(enabled_extension_count < 64);
        }

        free(device_extensions);
    }

    if (!swapchainExtFound) {
        ERR_EXIT("vkEnumerateDeviceExtensionProperties failed to find the " VK_KHR_SWAPCHAIN_EXTENSION_NAME " extension.\n\n");
    }

    if (validate) {

        vxCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)
                 vkGetInstanceProcAddr(inst, "vkCreateDebugUtilsMessengerEXT");

        vxDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)
                 vkGetInstanceProcAddr(inst,  "vkDestroyDebugUtilsMessengerEXT");

        if (!vxCreateDebugUtilsMessengerEXT || !vxDestroyDebugUtilsMessengerEXT) {

            ERR_EXIT("GetProcAddr: Failed to init VK_EXT_debug_utils\n");
        }

        err = vxCreateDebugUtilsMessengerEXT(inst, &dbg_messenger_ci, NULL, &dbg_messenger);
        switch (err) {
            case VK_SUCCESS:
                break;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                ERR_EXIT("vkCreateDebugUtilsMessengerEXT: out of host memory\n");
                break;
            default:
                ERR_EXIT("vkCreateDebugUtilsMessengerEXT: unknown failure\n");
                break;
        }
    }
}

static void create_device() {
    VkResult err;
    float queue_priorities[1] = {0.0};
    VkDeviceQueueCreateInfo queues_ci = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext = NULL,
      .queueFamilyIndex = queue_family_index,
      .queueCount = 1,
      .pQueuePriorities = queue_priorities,
      .flags = 0,
    };

    VkPhysicalDeviceMultiviewFeaturesKHR gpu_mv_features = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR,
      .multiview = VK_TRUE,
    };

    VkPhysicalDeviceFeatures2 gpu_features2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .features = {},
      .pNext = &gpu_mv_features,
    };

    VkDeviceCreateInfo dev_ci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &gpu_features2,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queues_ci,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = enabled_extension_count,
        .ppEnabledExtensionNames = (const char *const *)extension_names,
        .pEnabledFeatures = 0,
    };
    err = vkCreateDevice(gpu, &dev_ci, NULL, &device);
    assert(!err);
    vkGetDeviceQueue(device, queue_family_index, 0, &queue);
}

static void find_queue_families() {

    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, NULL);
    assert(queue_family_count >= 1);

    queue_props = (VkQueueFamilyProperties *)malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, queue_props);

    VkBool32 *supportsPresent = (VkBool32 *)malloc(queue_family_count * sizeof(VkBool32));
    for (uint32_t i = 0; i < queue_family_count; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &supportsPresent[i]);
    }

    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t presentQueueFamilyIndex = UINT32_MAX;

    for (uint32_t i = 0; i < queue_family_count; i++) {
        if ((queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            if (graphicsQueueFamilyIndex == UINT32_MAX) {
                graphicsQueueFamilyIndex = i;
            }
            if (supportsPresent[i] == VK_TRUE) {
                graphicsQueueFamilyIndex = i;
                presentQueueFamilyIndex = i;
                break;
            }
        }
    }

    if (graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX) {
        log_write("Could not find either/both graphics or present queues g=%d p=%d\n", graphicsQueueFamilyIndex, presentQueueFamilyIndex);
        ERR_EXIT("");
    }

    if(graphicsQueueFamilyIndex != presentQueueFamilyIndex){
      ERR_EXIT("Wow! the graphics and present queues are actually different!");
    }
    free(supportsPresent);

    queue_family_index = graphicsQueueFamilyIndex;
}

static VkSurfaceFormatKHR pick_surface_format(const VkSurfaceFormatKHR *surface_formats,
                                              uint32_t count) {

    for (uint32_t i = 0; i < count; i++) {
        VkFormat f = surface_formats[i].format;
        if (f == VK_FORMAT_R8G8B8A8_SRGB) {

            log_write("found VK_FORMAT_R8G8B8A8_SRGB\n");
            return surface_formats[i];
        }
    }
    log_write("failed to find SRGB format\n");
    for (uint32_t i = 0; i < count; i++) {
        VkFormat f = surface_formats[i].format;
        if (f == VK_FORMAT_R8G8B8A8_UNORM ||
            f == VK_FORMAT_B8G8R8A8_UNORM      ) {

            log_write("found VK_FORMAT_R8G8B8A8_UNORM\n");
            return surface_formats[i];
        }
    }
    for (uint32_t i = 0; i < count; i++) {
        VkFormat f = surface_formats[i].format;
        if (f == VK_FORMAT_A2R10G10B10_UNORM_PACK32 ||
            f == VK_FORMAT_A2B10G10R10_UNORM_PACK32    ) {

            log_write("found VK_FORMAT_A2R10G10B10_UNORM_PACK32\n");
            return surface_formats[i];
        }
    }
    for (uint32_t i = 0; i < count; i++) {
        VkFormat f = surface_formats[i].format;
        if (f == VK_FORMAT_R16G16B16A16_SFLOAT) {

            log_write("found VK_FORMAT_R16G16B16A16_SFLOAT\n");
            return surface_formats[i];
        }
    }
    log_write("Can't find our preferred formats.\n");
    log_write("Falling back to first exposed format. Rendering may be incorrect.\n");

    assert(count >= 1);
    return surface_formats[0];
}

static void choose_surface_format(){

    uint32_t count;

    assert(!vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &count, NULL));
    VkSurfaceFormatKHR *surface_formats = (VkSurfaceFormatKHR *)malloc(count * sizeof(VkSurfaceFormatKHR));
    assert(!vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &count, surface_formats));

    VkSurfaceFormatKHR surfaceFormat = pick_surface_format(surface_formats, count);

    surface_format = surfaceFormat.format;
    color_space    = surfaceFormat.colorSpace;

    free(surface_formats);
}

static void cleanup_swapchain_surface_instance(){

  vkDestroySwapchainKHR(device, swapchain, NULL);
  swapchain=0;

  free(queue_props);
  vkDestroyCommandPool(device, command_pool, NULL);
  command_pool=0;

  vkDeviceWaitIdle(device);
  vkDestroyDevice(device, NULL);
  device=0;

  if(validate) {
      vxDestroyDebugUtilsMessengerEXT(inst, dbg_messenger, NULL);
  }
  vkDestroySurfaceKHR(inst, surface, NULL);
  surface=0;

  vkDestroyInstance(inst, NULL);
  inst=0;
}

static void prepare(bool restart) {

  if(!restart){

    onl_init();
    onl_create_window();

    create_instance();

    onl_create_surface(inst, &surface);

    pick_physical_device();
    find_queue_families();
    create_device();

    prepare_command_pools();
    choose_surface_format();
  }

  prepare_swapchain();

  begin_command_buffer();
  {
    onl_vk_rg_prepare_swapchain_images(restart);
    onl_vk_rg_prepare_semaphores_and_fences(restart);
    onl_vk_rg_prepare_command_buffers(restart);
    onl_vk_rg_prepare_rendering(restart);
    onx_vk_rd_prepare_render_data(restart);
    onx_vk_rd_prepare_uniform_buffers(restart);
    onx_vk_rd_prepare_descriptor_layout(restart);
    onl_vk_rg_prepare_pipeline_layout(restart);
    onx_vk_rd_prepare_descriptor_pool(restart);
    onx_vk_rd_prepare_descriptor_set(restart);
    onl_vk_rg_prepare_render_pass(restart);
    onx_vk_rd_prepare_shaders(restart);
    onl_vk_rg_prepare_pipeline(restart);
    onl_vk_rg_prepare_framebuffers(restart);
  }
  end_command_buffer();

  prepared = true;
}

static void finish(bool restart) {

  prepared = false;

  vkDeviceWaitIdle(device);

  onl_vk_rg_finish_rendering();
  onx_vk_rd_finish_render_data();

  if(!restart){

    cleanup_swapchain_surface_instance();

    onl_finish();
  }
}

extern void onx_init();
extern void onx_iostate_changed();

void ont_vk_loop(bool running) {

  if(running && !prepared){
    prepare(false);
    onx_init();
  }
  else
  if(!running && prepared){
    finish(false);
  }
  if(prepared){
    onx_vk_rd_update_uniforms();
    onl_vk_rg_render_frame();
    frames++;
  }
  static uint64_t lt=0;
  if(!lt) lt=time_ms();
  uint64_t ct=time_ms();
  uint16_t dt=(uint16_t)(ct-lt);
  if(dt > 1000){
    log_write("fps: %d\n", frames * 1000 / dt);
    frames=0;
    lt=ct;
  }
}

void ont_vk_restart(){
  finish(true);
  prepare(true);
}


iostate io;

void ont_vk_set_io_mouse(int32_t x, int32_t y){

  io.mouse_x = x;
  io.mouse_y = y;
}

void ont_vk_iostate_changed() {

  onx_iostate_changed();
}

