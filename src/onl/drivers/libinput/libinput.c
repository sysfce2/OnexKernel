#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <libudev.h>
#include <libinput.h>
#include <linux/input-event-codes.h>

#include <onl.h>
#include "onl/drivers/libinput/libinput.h"

static struct udev*     udev;
static struct libinput* libin;

static int open_restricted(const char *path, int flags, void *user_data) {
  int fd = open(path, flags);
  return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *user_data) {
  close(fd);
}

static const struct libinput_interface interface = {
  .open_restricted = open_restricted,
  .close_restricted = close_restricted,
};

static iostate_change_cb_t iostate_change_cb=0;

int libinput_init(iostate_change_cb_t cb) {

  iostate_change_cb = cb;

  udev = udev_new();
  if (!udev) {
      fprintf(stderr, "Failed to create udev\n");
      return -1;
  }

  libin = libinput_udev_create_context(&interface, 0, udev);
  if (!libin) {
      fprintf(stderr, "Failed to create libinput context\n");
      udev_unref(udev);
      return -1;
  }

  if (libinput_udev_assign_seat(libin, "seat0") != 0) {
      fprintf(stderr, "Failed to assign seat\n");
      libinput_unref(libin);
      udev_unref(udev);
      return -1;
  }

  printf("libinput looking for devices...\n");

  libinput_dispatch(libin);

  struct libinput_event* event;
  while((event = libinput_get_event(libin))){

    uint32_t evt = libinput_event_get_type(event);
    switch(evt) {

      case LIBINPUT_EVENT_DEVICE_ADDED: {

        struct libinput_device* device  = libinput_event_get_device(event);
        struct udev_device*     udevice = libinput_device_get_udev_device(device);
        const char*             devpath = udev_device_get_devnode(udevice);

        if(libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_POINTER)){

          const char* p = udev_device_get_property_value(udevice, "ID_INPUT_MOUSE");

          printf("found mouse %s, ID_INPUT_MOUSE=%s\n", devpath, p? p: "none");
        }
        else
        if(libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_TOUCH)){

          const char* p = udev_device_get_property_value(udevice, "ID_INPUT_TOUCHSCREEN");

          printf("found touchscreen %s, ID_INPUT_TOUCHSCREEN=%s\n", devpath, p? p: "none");
        }
        break;
      }
      default: {
        printf("event that isn't device added!\n");
        break;
      }
    }
    libinput_event_destroy(event);
    libinput_dispatch(libin);
  }
  return 0;
}

void libinput_end() {

  if(libin) libinput_unref(libin);

  udev_unref(udev);

  iostate_change_cb = 0;
}

static int32_t touch_positions[16][2];

static void handle_libinput_event(struct libinput_event* event) {

  if(!iostate_change_cb) return;

  uint32_t evt = libinput_event_get_type(event);
  switch(evt) {

    case LIBINPUT_EVENT_POINTER_BUTTON: {

      struct libinput_event_pointer *m = libinput_event_get_pointer_event(event);
      uint32_t btn = libinput_event_pointer_get_button(m);
      uint32_t stt = libinput_event_pointer_get_button_state(m);

      switch(btn){
        case BTN_LEFT: {

          io.mouse_left = (stt==LIBINPUT_BUTTON_STATE_PRESSED);

          iostate_change_cb();

          break;
        }
        case BTN_MIDDLE: {

          io.mouse_middle= (stt==LIBINPUT_BUTTON_STATE_PRESSED);

          iostate_change_cb();

          break;
        }
        case BTN_RIGHT: {

          io.mouse_right = (stt==LIBINPUT_BUTTON_STATE_PRESSED);

          iostate_change_cb();

          break;
        }
      }
      break;
    }
    case LIBINPUT_EVENT_POINTER_SCROLL_WHEEL: {

      struct libinput_event_pointer *m = libinput_event_get_pointer_event(event);
      double sv = libinput_event_pointer_get_scroll_value_v120(m,
                                           LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL);

      io.mouse_scroll = io.mouse_scroll + (uint32_t)sv;

      iostate_change_cb();

      break;
    }
    case LIBINPUT_EVENT_POINTER_MOTION: {

      struct libinput_event_pointer *m = libinput_event_get_pointer_event(event);

      int32_t dx = (int32_t)libinput_event_pointer_get_dx(m);
      int32_t dy = (int32_t)libinput_event_pointer_get_dy(m);

      io.mouse_x = io.mouse_x + dx;
      io.mouse_y = io.mouse_y + dy;

      iostate_change_cb();

      break;
    }
    case LIBINPUT_EVENT_TOUCH_DOWN: {

      struct libinput_event_touch *t = libinput_event_get_touch_event(event);

      int32_t s = (int32_t)libinput_event_touch_get_slot(t);
      int32_t x = (int32_t)libinput_event_touch_get_x(t);
      int32_t y = (int32_t)libinput_event_touch_get_y(t);

      touch_positions[s][0]=x;
      touch_positions[s][1]=y;

      io.mouse_x = x;
      io.mouse_y = y;

      io.mouse_left=true;

      iostate_change_cb();

      break;
    }
    case LIBINPUT_EVENT_TOUCH_MOTION: {

      struct libinput_event_touch *t = libinput_event_get_touch_event(event);

      int32_t s = (int32_t)libinput_event_touch_get_slot(t);
      int32_t x = (int32_t)libinput_event_touch_get_x(t);
      int32_t y = (int32_t)libinput_event_touch_get_y(t);

      if(touch_positions[s][0] == x &&
         touch_positions[s][1] == y   ) break;

      touch_positions[s][0]=x;
      touch_positions[s][1]=y;

      io.mouse_x = x;
      io.mouse_y = y;

      iostate_change_cb();

      break;
    }
    case LIBINPUT_EVENT_TOUCH_UP: {

      struct libinput_event_touch *t = libinput_event_get_touch_event(event);

      int32_t s = libinput_event_touch_get_slot(t);

      io.mouse_left=false;

      iostate_change_cb();

      break;
    }
    case LIBINPUT_EVENT_KEYBOARD_KEY: {
/*

      struct libinput_event_keyboard* k       = libinput_event_get_keyboard_event(event);
      uint32_t                        key     = libinput_event_keyboard_get_key(k);
      enum libinput_key_state         state   = libinput_event_keyboard_get_key_state(k);
      const char*                     keyname = libevdev_event_code_get_name(EV_KEY, key);
      printf("%d %s\n", key, state==LIBINPUT_KEY_STATE_PRESSED? "pressed": "released");
*/
      break;
    }
    default: {

      if(!(evt == LIBINPUT_EVENT_POINTER_AXIS ||
           evt == LIBINPUT_EVENT_TOUCH_FRAME     )){

        printf("unhandled event from libinput: %d\n", evt);
      }
    }
  }
}

void libinput_process_events() {

  if(!libin) return;

  libinput_dispatch(libin);

  struct libinput_event* event;
  while((event = libinput_get_event(libin))){

    handle_libinput_event(event);

    libinput_event_destroy(event);
    libinput_dispatch(libin);
  }
}

