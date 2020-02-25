
#include <app_timer.h>
#include <nrf_delay.h>
#include <nrf_drv_clock.h>

#include <onex-kernel/time.h>

static bool initialised=false;

void time_init()
{
  if(initialised) return;
  ret_code_t e = nrf_drv_clock_init(); APP_ERROR_CHECK(e);
  nrf_drv_clock_lfclk_request(NULL);
  while(!nrf_drv_clock_lfclk_is_running());
  e = app_timer_init(); APP_ERROR_CHECK(e);
  initialised=true;
}

uint32_t time_ms(){
  return 0;
}

uint32_t time_us(){
  return 0;
}

void time_delay_us(uint32_t us)
{
  nrf_delay_us(us);
}

void time_delay_ms(uint32_t ms)
{
  if(ms<=0) return;
  for(int t=0; t<ms; t++) time_delay_us(999);
}

void time_delay_s(uint32_t s)
{
  time_delay_ms(1000*s);
}

