
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

uint16_t time_timeout(time_up_cb cb)
{
  return 1;
}

void time_start_timer(uint16_t id, uint32_t timeout)
{
}

void time_end(){
}

