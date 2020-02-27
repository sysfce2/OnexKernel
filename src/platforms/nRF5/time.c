
#include <app_timer.h>
#include <nrf_delay.h>
#include <nrf_drv_clock.h>

#include <onex-kernel/time.h>

static bool initialised=false;

APP_TIMER_DEF(m_timer_0);

uint32_t counter;

static void timer_handle(void* p)
{
  counter++;
}

void time_init()
{
  if(initialised) return;
  ret_code_t e = nrf_drv_clock_init(); APP_ERROR_CHECK(e);
  nrf_drv_clock_lfclk_request(NULL);
  while(!nrf_drv_clock_lfclk_is_running());
  e = app_timer_init(); APP_ERROR_CHECK(e);
  e = app_timer_create(&m_timer_0, APP_TIMER_MODE_REPEATED, timer_handle); APP_ERROR_CHECK(e);
  e = app_timer_start(m_timer_0, APP_TIMER_TICKS(1000), NULL); APP_ERROR_CHECK(e);
  initialised=true;
}

#define APP_TIMER_PRESCALER 0 // not sure what this is

uint32_t time_ms(){
  uint32_t ticks=app_timer_cnt_get();
  return ticks*((APP_TIMER_PRESCALER+1)*1000)/APP_TIMER_CLOCK_FREQ;
}

uint32_t time_us(){
  uint32_t ticks=app_timer_cnt_get();
  return ticks*((APP_TIMER_PRESCALER+1)*1000000L)/APP_TIMER_CLOCK_FREQ;
}

void time_delay_us(uint32_t us)
{
  nrf_delay_us(us*2); // *2??
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

