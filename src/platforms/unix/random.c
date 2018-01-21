
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <onex-kernel/random.h>

static bool initialised=false;

uint8_t random_ish_byte()
{
  if(!initialised){ srand(time(0)); initialised=true; }
  int r = rand();
  return *((uint8_t*)&r);
}

