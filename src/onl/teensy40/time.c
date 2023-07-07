
#include <core_pins.h>
#include <time.h>

void time_init()
{
}

uint32_t time_ms()
{
  return millis();
}

uint32_t time_us()
{
  return 0;
}

void time_delay_s( uint32_t s)
{
  delay(s*1000);
}

void time_delay_ms(uint32_t ms)
{
  delay(ms);
}

void time_delay_us(uint32_t us)
{
  delayMicroseconds(us);
}


