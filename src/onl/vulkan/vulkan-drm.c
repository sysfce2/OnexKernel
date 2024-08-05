
/* Wiring between Vulkan and DRM */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include <onex-kernel/log.h>

#include <onl-vk.h>
#include "onl/drivers/libinput/libinput.h"
#include "onl/drivers/viture/viture_imu.h"
#include "onl/vulkan/vk.h"

// -----------------------------------------

bool quit=false;

static void sigint_handler(int signal, siginfo_t *siginfo, void *userdata) {
  log_write("\nEnd\n");
  quit = true;
}

static void set_signal(int sig, void (*h)(int, siginfo_t*, void*)){
  struct sigaction act;
  memset(&act, 0, sizeof(act));
  act.sa_sigaction = h;
  act.sa_flags = SA_SIGINFO;
  sigaction(sig, &act, 0);
}

void head_rotated(uint32_t ts, float yaw, float pitch, float roll){

  io.yaw   = yaw;
  io.pitch = pitch;
  io.roll  = roll;

  onl_vk_iostate_changed();
}

void onl_vk_init() {

  set_signal(SIGINT, sigint_handler);
  set_signal(SIGTERM, sigint_handler);

  int r=libinput_init(onl_vk_iostate_changed);
  if(r) log_write("failed to initialise libinput (%d)\n", r);

  viture_init(head_rotated);
}

void onl_vk_create_window() {
  // DRM has no window to create
}

static VkDisplayPlaneAlphaFlagBitsKHR alpha_modes_order[] = {
    VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR,
    VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR,
    VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR,
    VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR,
};

void onl_vk_create_surface(VkInstance inst, VkSurfaceKHR* surface) {

  uint32_t n_disp_props = 0;
  vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &n_disp_props, 0);
  VkDisplayPropertiesKHR disp_props[n_disp_props];
  vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &n_disp_props, disp_props);
  log_write("found %d display%s\n", n_disp_props, n_disp_props==1?"":"s");

  VkDisplayKHR     display = 0;
  VkDisplayModeKHR display_mode = 0;

  for (uint32_t d = 0; d < n_disp_props; d++) {

    VkDisplayKHR disp = disp_props[d].display;

    uint32_t n_mode_props = 0;
    vkGetDisplayModePropertiesKHR(gpu, disp, &n_mode_props, 0);
    VkDisplayModePropertiesKHR mode_props[n_mode_props];
    vkGetDisplayModePropertiesKHR(gpu, disp, &n_mode_props, mode_props);
    log_write("found %d mode%s for display %d\n", n_mode_props, n_mode_props==1?"":"s", d);

    for (uint32_t m = 0; m < n_mode_props; m++) {

			VkDisplayModePropertiesKHR* mode = &mode_props[m];

      log_write("display %d mode %d is %dx%d @%fHz\n", d, m,
                                  mode->parameters.visibleRegion.width,
                                  mode->parameters.visibleRegion.height,
                                  mode->parameters.refreshRate/1000.0f);

			if(mode->parameters.refreshRate > 20 /* or whatever test */ ){

				if(!display_mode){

          log_write("using this display mode\n");

          swap_width  = mode->parameters.visibleRegion.width;
          swap_height = mode->parameters.visibleRegion.height;

          display = disp;
          display_mode = mode->displayMode;
        }
        // .. but keep looping to log all the other options we had
			}
    }
  }
  if(!display_mode){
    ONL_VK_ERR_EXIT("Unable to find any mode on any display");
  }

  uint32_t n_plane_props = 0;
  vkGetPhysicalDeviceDisplayPlanePropertiesKHR(gpu, &n_plane_props, 0);
  VkDisplayPlanePropertiesKHR plane_props[n_plane_props];
  vkGetPhysicalDeviceDisplayPlanePropertiesKHR(gpu, &n_plane_props, plane_props);
  log_write("found %d plane%s\n", n_plane_props, n_plane_props==1?"":"s");

  uint32_t plane_index = UINT32_MAX;

  for (uint32_t p = 0; p < n_plane_props; p++) {

    uint32_t n_displays = 0;
    vkGetDisplayPlaneSupportedDisplaysKHR(gpu, p, &n_displays, 0);
    VkDisplayKHR displays[n_displays];
    vkGetDisplayPlaneSupportedDisplaysKHR(gpu, p, &n_displays, displays);
    log_write("found %d display%s for plane %d\n", n_displays, n_displays==1?"":"s", p);

    for (uint32_t d = 0; d < n_displays; d++) {
      if(display == displays[d]) {

        log_write("found chosen display in plane %d display %d\n", p, d);

        if(plane_index == UINT32_MAX){
          log_write("using this plane\n");
          plane_index = p;
        }
        // .. but keep looping to log all the other options we had
      }
    }
  }
  if(plane_index == UINT32_MAX) {
      ONL_VK_ERR_EXIT("no plane found for chosen display");
  }

  VkDisplayPlaneCapabilitiesKHR plane_capab;
  vkGetDisplayPlaneCapabilitiesKHR(gpu, display_mode, plane_index, &plane_capab);

  VkDisplayPlaneAlphaFlagBitsKHR alpha_mode=0;

  for(uint32_t a = 0; a < ARRAY_SIZE(alpha_modes_order); a++) {

    log_write("checking alpha mode %b (%d of %d) is in %b\n",
               alpha_modes_order[a],
               a+1, ARRAY_SIZE(alpha_modes_order),
               plane_capab.supportedAlpha);

    if(alpha_modes_order[a] & plane_capab.supportedAlpha) {

      log_write("alpha mode is supported by plane\n");

      if(!alpha_mode){

        log_write("using that alpha mode\n");

        alpha_mode = alpha_modes_order[a];
      }
      // .. but keep looping to log all the other options we had
    }
  }
  if(!alpha_mode){
    ONL_VK_ERR_EXIT("no alpha mode found from preferred list");
  }

  VkDisplaySurfaceCreateInfoKHR drm_surface_ci = {
      .sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR,
      .flags = 0,
      .displayMode = display_mode,
      .planeIndex = plane_index,
      .planeStackIndex = plane_props[plane_index].currentStackIndex,
      .transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
      .alphaMode = alpha_mode,
      .globalAlpha = 1.0f,
      .imageExtent = { swap_width, swap_height },
      .pNext = 0,
  };

  ONL_VK_CHECK_EXIT(vkCreateDisplayPlaneSurfaceKHR(inst, &drm_surface_ci, 0, surface));
}

static void event_loop() {

    while (!quit){

        onl_vk_loop(true);

        libinput_process_events();
    }
    onl_vk_loop(false);
}

void onl_vk_finish() {

  viture_end();

  libinput_end();
}

int main() {

  log_write("----------------------\nDRM Vulkan\n-----------------------\n");

  event_loop();
}

void onl_vk_quit(){
  quit=true;
}

