
#include <app_timer.h>
#include <nrf_delay.h>
#include <nrf_drv_clock.h>

#include <onex-kernel/time.h>

static bool initialised=false;

#define EFFECTIVE_TIMER_CLOCK_FREQ (APP_TIMER_CLOCK_FREQ/(APP_TIMER_CONFIG_RTC_FREQUENCY+1))
#define TICKS_TO_MS(ticks) (((ticks)*1000)/EFFECTIVE_TIMER_CLOCK_FREQ)

static uint32_t seconds=0;
static uint32_t ticks_at_second=0;

static void every_second(void* p)
{
  seconds++;
  ticks_at_second = app_timer_cnt_get();
}

APP_TIMER_DEF(m_timer_0);

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

uint32_t time_s(){
  if(!initialised) return 0;
  return seconds;
}

uint64_t time_ms(){
  if(!initialised) return 0;
  NVIC_DisableIRQ(RTC1_IRQn);
    uint32_t tix=app_timer_cnt_get();
    uint32_t dif=app_timer_cnt_diff_compute(tix, ticks_at_second);
    uint64_t r=((uint64_t)(seconds*1000))+TICKS_TO_MS(dif);
  NVIC_EnableIRQ(RTC1_IRQn);
  return r;
}

uint64_t time_us(){
  if(!initialised) return 0;
  return time_ms()*1000;
}

void time_delay_us(uint32_t us)
{
  nrf_delay_us(us*(APP_TIMER_CONFIG_RTC_FREQUENCY+1));
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

