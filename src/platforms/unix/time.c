
#include <onex-kernel/time.h>

static bool initialised=false;

uint64_t startus;

uint64_t get_time_us()
{
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return t.tv_sec*1000000+t.tv_nsec/1000;
}

void time_init()
{
  if(initialised) return;
  startus=get_time_us();
  initialised=true;
}

uint32_t time_us()
{
  time_init();
  return get_time_us()-startus;
}

uint32_t time_ms()
{
  time_init();
  return time_us()/1000;
}


