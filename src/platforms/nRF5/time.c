
#include <app_timer.h>
#include <nrf_delay.h>
#include <nrf_drv_clock.h>

#include <onex-kernel/time.h>

static bool initialised=false;

APP_TIMER_DEF(m_timer_0);


#define APP_TIMER_PRESCALER 1 // where is this defined?
#define EFFECTIVE_TIMER_CLOCK_FREQ (APP_TIMER_CLOCK_FREQ/(APP_TIMER_PRESCALER+1))
#define OVERFLOW_TICKS 16777216

static uint64_t seconds=0;
static uint32_t ticks_at_second=0;

static void every_second(void* p)
{
  seconds++;
  ticks_at_second = app_timer_cnt_get();
}

void time_init()
{
  if(initialised) return;
  ret_code_t e = nrf_drv_clock_init(); APP_ERROR_CHECK(e);
  nrf_drv_clock_lfclk_request(NULL);
  while(!nrf_drv_clock_lfclk_is_running());
  e = app_timer_init(); APP_ERROR_CHECK(e);
  e = app_timer_create(&m_timer_0, APP_TIMER_MODE_REPEATED, every_second); APP_ERROR_CHECK(e);
  e = app_timer_start(m_timer_0, APP_TIMER_TICKS(1000), NULL); APP_ERROR_CHECK(e);
  initialised=true;
}

uint64_t time_s(){
  return seconds;
}

uint64_t time_ms(){
  int32_t ticks = app_timer_cnt_get()-ticks_at_second;
  if(ticks<0) ticks+=OVERFLOW_TICKS;
  return (seconds*1000)+(ticks*1000)/EFFECTIVE_TIMER_CLOCK_FREQ;
}

uint64_t time_us(){
  return time_ms()*1000;
}

void time_delay_us(uint32_t us)
{
  nrf_delay_us(us*(APP_TIMER_PRESCALER+1));
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

