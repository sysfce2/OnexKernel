#ifndef ONL_H
#define ONL_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct onl_iostate {

  bool     d_pad_left;
  bool     d_pad_right;
  bool     d_pad_up;
  bool     d_pad_down;

  float    yaw;
  float    pitch;
  float    roll;

  uint32_t mouse_x;
  uint32_t mouse_y;
  uint32_t mouse_scroll;

  bool     mouse_left;
  bool     mouse_middle;
  bool     mouse_right;

  char     key;

} onl_iostate;

extern onl_iostate io;

#endif
