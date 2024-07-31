
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

void onl_create_window() {
  io.swap_width =640;
  io.swap_height=480;
}

void onl_create_surface(VkInstance inst, VkSurfaceKHR* surface) {

    VkResult err;

    VkDisplaySurfaceCreateInfoKHR drm_surface_ci = {
      .sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR,
      .flags = 0,
      .displayMode = mode_props.displayMode,
      .planeIndex = plane_index,
      .planeStackIndex = plane_props[plane_index].currentStackIndex,
      .transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
      .alphaMode = alphaMode,
      .globalAlpha = 1.0f,
      .imageExtent = image_extent,
      .pNext = 0,
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

