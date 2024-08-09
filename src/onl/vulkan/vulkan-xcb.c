
/* Wiring between Vulkan and XCB */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include <xcb/xcb.h>

#include <onex-kernel/log.h>

#include <onl-vk.h>
#include "onl/drivers/libevdev/libevdev.h"
#include "onl/drivers/viture/viture_imu.h"
#include "onl/vulkan/vk.h"

// -----------------------------------------

bool quit=false;

xcb_connection_t *connection;
xcb_screen_t *    screen;
xcb_window_t      window;

xcb_intern_atom_reply_t *atom_wm_delete_window;

static inline xcb_intern_atom_reply_t* intern_atom_helper(xcb_connection_t *conn, bool only_if_exists, const char *str)
{
  xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, only_if_exists, strlen(str), str);
  return xcb_intern_atom_reply(conn, cookie, NULL);
}

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

static void xcb_init(){

  const xcb_setup_t *setup;
  xcb_screen_iterator_t iter;
  int scr;

  connection = xcb_connect(NULL, &scr);

  if(xcb_connection_has_error(connection)){
      log_write("Cannot connect to XCB\n");
      fflush(stdout);
      exit(1);
  }
  setup = xcb_get_setup(connection);
  iter = xcb_setup_roots_iterator(setup);
  while(scr--) xcb_screen_next(&iter);
  screen = iter.data;

  xcb_flush(connection);
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

  int r=libevdev_init(onl_vk_iostate_changed);
  if(r) log_write("failed to initialise libevdev (%d)\n", r);

  viture_init(head_rotated);

  xcb_init();
}

void onl_vk_create_window() {

  swap_width =screen->width_in_pixels;
  swap_height=screen->height_in_pixels;

  uint32_t value_mask, value_list[32];

  window = xcb_generate_id(connection);

  value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  value_list[0] = screen->black_pixel;
  value_list[1] =
    XCB_EVENT_MASK_KEY_RELEASE |
    XCB_EVENT_MASK_KEY_PRESS |
    XCB_EVENT_MASK_EXPOSURE |
    XCB_EVENT_MASK_STRUCTURE_NOTIFY |
    XCB_EVENT_MASK_POINTER_MOTION |
    XCB_EVENT_MASK_BUTTON_PRESS |
    XCB_EVENT_MASK_BUTTON_RELEASE;

  xcb_create_window(connection,
    XCB_COPY_FROM_PARENT,
    window, screen->root,
    0, 0, swap_width, swap_height, 0,
    XCB_WINDOW_CLASS_INPUT_OUTPUT,
    screen->root_visual,
    value_mask, value_list);

  /* Magic code that will send notification when window is destroyed */
  xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, 1, 12, "WM_PROTOCOLS");
  xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookie, 0);
  xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(connection, 0, 16, "WM_DELETE_WINDOW");
  atom_wm_delete_window = xcb_intern_atom_reply(connection, cookie2, 0);

  xcb_change_property(connection, XCB_PROP_MODE_REPLACE,
    window, (*reply).atom, 4, 32, 1,
    &(*atom_wm_delete_window).atom);

  free(reply);

  xcb_intern_atom_reply_t *atom_wm_state      = intern_atom_helper(connection, false, "_NET_WM_STATE");
  xcb_intern_atom_reply_t *atom_wm_fullscreen = intern_atom_helper(connection, false, "_NET_WM_STATE_FULLSCREEN");

  xcb_change_property(connection,
                      XCB_PROP_MODE_REPLACE,
                      window, atom_wm_state->atom,
                      XCB_ATOM_ATOM, 32, 1,
                      &(atom_wm_fullscreen->atom));

  free(atom_wm_fullscreen);
  free(atom_wm_state);

  xcb_map_window(connection, window);
}

void onl_vk_create_surface(VkInstance inst, VkSurfaceKHR* surface) {

    VkResult err;

    VkXcbSurfaceCreateInfoKHR xcb_surface_ci = {
      .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
      .flags = 0,
      .connection = connection,
      .window = window,
    };

    ONL_VK_CHECK_EXIT(vkCreateXcbSurfaceKHR(inst, &xcb_surface_ci, 0, surface));
}

static void handle_xcb_event(const xcb_generic_event_t *event) {

    switch(event->response_type & 0x7f) {

        case XCB_CONFIGURE_NOTIFY: {
            xcb_configure_notify_event_t* cne=(xcb_configure_notify_event_t*)event;
            uint32_t w=cne->width;
            uint32_t h=cne->height;
            if ((swap_width != w) || (swap_height != h)) {

                swap_width = w;
                swap_height = h;

                onl_vk_iostate_changed();
            }
            break;
        }
        case XCB_CLIENT_MESSAGE:{
            if ((*(xcb_client_message_event_t *)event).data.data32[0] ==
                                              (*atom_wm_delete_window).atom) {
                quit = true;
            }
            break;
        }
        case XCB_EXPOSE: {
            break;
        }
        default: {
            break;
        }
    }
}

static void event_loop() {
/*
  struct pollfd fds = {
    .fd = ,
    .events = POLLIN,
    .revents = 0,
  };
  poll(&fds, 1, -1) > -1
*/
    while(!quit){

        onl_vk_loop(true);

        libevdev_process_events();

        xcb_generic_event_t *event;

        while((event = xcb_poll_for_event(connection))) {

            handle_xcb_event(event);

            free(event);
        }
    }
    onl_vk_loop(false);
}

void onl_vk_finish() {

  xcb_destroy_window(connection, window);
  xcb_disconnect(connection);
  free(atom_wm_delete_window);

  viture_end();

  libevdev_end();
}

int main() {

  log_write("----------------------\nXCB Vulkan\n-----------------------\n");

  event_loop();
}

void onl_vk_quit(){
  quit=true;
}



