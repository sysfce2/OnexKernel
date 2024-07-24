
#include <stdbool.h>

#include <onex-kernel/log.h>
#include <onl.h>

bool quit = false;

iostate io;

void set_io_mouse(int32_t x, int32_t y){

  io.mouse_x = x;
  io.mouse_y = y;
}

