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

  uint32_t mouse_x;
  uint32_t mouse_y;
  uint32_t mouse_scroll;
  bool     mouse_left;
  bool     mouse_middle;
  bool     mouse_right;

  float    yaw;
  float    pitch;
  float    roll;

  char     key;

} onl_iostate;

extern onl_iostate io;

#endif
