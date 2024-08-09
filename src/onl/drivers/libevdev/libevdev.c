#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <libevdev/libevdev.h>
#include <dirent.h>
#include <string.h>

#include <onl.h>
#include "onl/drivers/libevdev/libevdev.h"

#define MAX_DEVICES 32

struct device_context {
  int              fd;
  struct libevdev* dev;
  const char*      name;
};

static struct device_context devices[MAX_DEVICES];
static int                   device_count = 0;

static iostate_change_cb_t iostate_change_cb=0;

int libevdev_init(iostate_change_cb_t cb) {

  iostate_change_cb = cb;

  DIR *dir = opendir("/dev/input");

  if(!dir) {
    fprintf(stderr, "Failed to open /dev/input directory\n");
    return -1;
  }

  struct dirent* entry;
  while((entry = readdir(dir)) && device_count < MAX_DEVICES) {

      if(!strncmp(entry->d_name, "event", 5)){

          char device_path[PATH_MAX];
          snprintf(device_path, sizeof(device_path), "/dev/input/%s", entry->d_name);

          devices[device_count].fd = open(device_path, O_RDONLY | O_NONBLOCK);

          if (devices[device_count].fd < 0) {
            fprintf(stderr, "Failed to open device\n");
            continue;
          }

          int rc=libevdev_new_from_fd(devices[device_count].fd, &devices[device_count].dev);

          if (rc < 0) {
            fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
            close(devices[device_count].fd);
            continue;
          }
          devices[device_count].name=libevdev_get_name(devices[device_count].dev);

          printf("Found device %s: \"%s\"\n", device_path, devices[device_count].name);
          device_count++;
      }
  }
  closedir(dir);

  return 0;
}

void libevdev_end(){
  for (int i = 0; i < device_count; i++) {
      libevdev_free(devices[i].dev);
      close(devices[i].fd);
  }
  iostate_change_cb = 0;
}

static void handle_libevdev_event(const char* name, struct input_event *ev) {

  if(!iostate_change_cb) return;

  switch(ev->type){
    case EV_REL: {

      printf("Mouse movement \"%s\" %u value %d\n", name, ev->code, ev->value);

      break;
    }
    case EV_ABS: {

      printf("Touch or joystick event \"%s\" %u value %d\n", name, ev->code, ev->value);

      break;
    }
    case EV_KEY: {

      printf("\"%s\" Key %u %s\n", name, ev->code, ev->value ? "pressed" : "released");

      break;
    }
    default: {

      if(!(ev->type == 0 ||
           ev->type == 4    )){

        printf("unhandled event from libevdev, device \"%s\": %u\n", name, ev->type);
      }
    }
  }
}

void libevdev_process_events(){

  struct input_event ev;

  for (int i = 0; i < device_count; i++) {

    while(1){

      int rc = libevdev_next_event(devices[i].dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
      #define LIBEVDEV_NO_MORE -11
      if(rc == LIBEVDEV_NO_MORE){
        break;
      }
      if(rc){
        fprintf(stderr, "Failed to read next event on \"%s\" (%d %s)\n",
                         devices[i].name, rc, strerror(-rc));
        break;
      }
      handle_libevdev_event(devices[i].name, &ev);
    }
  }
}

