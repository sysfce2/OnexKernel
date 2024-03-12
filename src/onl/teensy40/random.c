
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <onex-kernel/random.h>

static bool initialised=false;

void random_init()
{
  if(initialised) return;
  srand(time_ms(0));
  initialised=true;
}

uint8_t random_ish_byte()
{
  int r = rand();
  return *((uint8_t*)&r);
}

