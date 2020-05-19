#ifndef MOTION_H
#define MOTION_H

typedef struct motion_info_t {
  int16_t x;
  int16_t y;
  int16_t z;
  int16_t m;
} motion_info_t;

typedef void (*motion_change_cb)(motion_info_t);

int           motion_init(motion_change_cb);
motion_info_t motion_get_info();

#endif
