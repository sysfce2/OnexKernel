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

  float    yaw;
  float    pitch;
  float    roll;

  bool     left_pressed;
  bool     middle_pressed;
  bool     right_pressed;

  char     key;

} onl_iostate;

extern onl_iostate io;

#endif
