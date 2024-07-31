
/* Wiring between Vulkan and DRM */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include <onex-kernel/log.h>

#include <onl-vk.h>
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

static void drm_init(){

}

void onl_vk_init() {

  set_signal(SIGINT, sigint_handler);

  drm_init();
}

void onl_vk_create_window() {
  io.swap_width =3840;
  io.swap_height=1080;
}

/* Alpha modes in order of preference */
static VkDisplayPlaneAlphaFlagBitsKHR alpha_modes[] = {
    VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR,
    VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR,
    VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR,
    VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR,
};

void onl_vk_create_surface(VkInstance inst, VkSurfaceKHR* surface) {

  uint32_t n_disp_props = 0;
  vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &n_disp_props, NULL);
  VkDisplayPropertiesKHR disp_props[n_disp_props];
  vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &n_disp_props, disp_props);
  log_write("found %d displays\n", n_disp_props);

  VkDisplayKHR     display = 0;
  VkDisplayModeKHR display_mode = 0;

  for (uint32_t i = 0; i < n_disp_props; i++) {

		display = disp_props[i].display;

    uint32_t n_mode_props = 0;
    vkGetDisplayModePropertiesKHR(gpu, display, &n_mode_props, NULL);
    VkDisplayModePropertiesKHR mode_props[n_mode_props];
    vkGetDisplayModePropertiesKHR(gpu, display, &n_mode_props, mode_props);
    log_write("found %d modes for display %d\n", n_mode_props, i);

    for (uint32_t j = 0; j < n_mode_props; j++) {

			VkDisplayModePropertiesKHR* mode = &mode_props[j];
      log_write("display %d mode %d %d/%d\n", i, j, mode->parameters.visibleRegion.width,
                                                    mode->parameters.visibleRegion.height);
			if(mode->parameters.visibleRegion.width  == io.swap_width &&
         mode->parameters.visibleRegion.height == io.swap_height &&
         mode->parameters.refreshRate > 20 /* ! */  ){

				display_mode = mode->displayMode;
				break;
			}
    }
    if(display_mode) break;
  }
  if(!display_mode){
    ONL_VK_ERR_EXIT("no mode matches for any display");
  }

  uint32_t n_plane_props = 0;
  vkGetPhysicalDeviceDisplayPlanePropertiesKHR(gpu, &n_plane_props, NULL);
  VkDisplayPlanePropertiesKHR plane_props[n_plane_props];
  vkGetPhysicalDeviceDisplayPlanePropertiesKHR(gpu, &n_plane_props, plane_props);

  uint32_t best_plane_index = UINT32_MAX;

  for (uint32_t i = 0; i < n_plane_props; i++) {

    uint32_t n_displays = 0;
    vkGetDisplayPlaneSupportedDisplaysKHR(gpu, i, &n_displays, NULL);
    VkDisplayKHR displays[n_displays];
    vkGetDisplayPlaneSupportedDisplaysKHR(gpu, i, &n_displays, displays);

    best_plane_index = UINT32_MAX;
    for (uint32_t j = 0; j < n_displays; j++) {
      if(display == displays[j]) {
        best_plane_index = i;
        break;
      }
    }
    if(best_plane_index != UINT32_MAX) break;
  }

  if(best_plane_index == UINT32_MAX) {
      ONL_VK_ERR_EXIT("no plane found");
  }

  VkDisplayPlaneCapabilitiesKHR plane_capab;
  vkGetDisplayPlaneCapabilitiesKHR(gpu, display_mode, best_plane_index, &plane_capab);

  VkDisplayPlaneAlphaFlagBitsKHR alpha_mode=0;
  for(uint32_t i = 0; i < ARRAY_SIZE(alpha_modes); i++) {
    if(plane_capab.supportedAlpha & alpha_modes[i]) {
      alpha_mode = alpha_modes[i];
      break;
    }
  }

  VkDisplaySurfaceCreateInfoKHR drm_surface_ci = {
      .sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR,
      .flags = 0,
      .displayMode = display_mode,
      .planeIndex = best_plane_index,
      .planeStackIndex = plane_props[best_plane_index].currentStackIndex,
      .transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
      .alphaMode = alpha_mode,
      .globalAlpha = 1.0f,
      .imageExtent = { io.swap_width, io.swap_height },
      .pNext = NULL,
  };

  ONL_VK_CHECK_EXIT(vkCreateDisplayPlaneSurfaceKHR(inst, &drm_surface_ci, 0, surface));
}

static void handle_in_event() {

}

static void event_loop() {

    while (!quit){

        onl_vk_loop(true);

        while(0/* libinput, head tracking */){
            handle_in_event();
        }
    }
    onl_vk_loop(false);
}

void onl_vk_finish() {

}

int main() {

  log_write("----------------------\nDRM Vulkan\n-----------------------\n");

  event_loop();
}

void onl_vk_quit(){
  quit=true;
}

