
#include <app_timer.h>
#include <nrf_delay.h>
#include <nrf_drv_clock.h>

#include <onex-kernel/time.h>

static volatile bool initialised=false;

#define EFFECTIVE_TIMER_CLOCK_FREQ (APP_TIMER_CLOCK_FREQ/(APP_TIMER_CONFIG_RTC_FREQUENCY+1))
#define TICKS_TO_MS(ticks) (((ticks)*1000)/EFFECTIVE_TIMER_CLOCK_FREQ)

static volatile uint32_t seconds=0;
static volatile uint64_t epoch_seconds=1675959628;
static volatile uint32_t ticks_at_second=0;

static void every_second(void*) {
  seconds++;
  epoch_seconds++;
  ticks_at_second = app_timer_cnt_get();
}

APP_TIMER_DEF(m_timer_0);
APP_TIMER_DEF(m_timer_1);
APP_TIMER_DEF(m_timer_2);
APP_TIMER_DEF(m_timer_3);
APP_TIMER_DEF(m_timer_4);
APP_TIMER_DEF(m_timer_5);
APP_TIMER_DEF(m_timer_6);

static app_timer_id_t timer_ids[7];

void time_init_set(uint64_t es)
{
  if(es) epoch_seconds=es;
  time_init();
}

void time_init() {

  if(initialised) return;
  ret_code_t e = nrf_drv_clock_init(); APP_ERROR_CHECK(e);

  nrf_drv_clock_lfclk_request(NULL);
  while(!nrf_drv_clock_lfclk_is_running());

  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  while(!NRF_CLOCK->EVENTS_HFCLKSTARTED);

  e = app_timer_init(); APP_ERROR_CHECK(e);
  e = app_timer_create(&m_timer_0, APP_TIMER_MODE_REPEATED, every_second); APP_ERROR_CHECK(e);
  e = app_timer_start(m_timer_0, APP_TIMER_TICKS(1000), NULL); APP_ERROR_CHECK(e);

  timer_ids[0]=m_timer_0;
  timer_ids[1]=m_timer_1;
  timer_ids[2]=m_timer_2;
  timer_ids[3]=m_timer_3;
  timer_ids[4]=m_timer_4;
  timer_ids[5]=m_timer_5;
  timer_ids[6]=m_timer_6;

  initialised=true;
}

uint32_t time_s()
{
  if(!initialised) return 0;
  return seconds;
}

uint64_t time_es()
{
  if(!initialised) return 0;
  return epoch_seconds;
}

void time_es_set(uint64_t es)
{
  epoch_seconds=es;
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


static time_up_cb up_cb_1=0;
static time_up_cb up_cb_2=0;
static time_up_cb up_cb_3=0;
static time_up_cb up_cb_4=0;
static time_up_cb up_cb_5=0;
static time_up_cb up_cb_6=0;

static void* up_arg_1=0;
static void* up_arg_2=0;
static void* up_arg_3=0;
static void* up_arg_4=0;
static void* up_arg_5=0;
static void* up_arg_6=0;

static void time_up_1(void*) { if(up_cb_1) up_cb_1(up_arg_1); }
static void time_up_2(void*) { if(up_cb_2) up_cb_2(up_arg_2); }
static void time_up_3(void*) { if(up_cb_3) up_cb_3(up_arg_3); }
static void time_up_4(void*) { if(up_cb_4) up_cb_4(up_arg_4); }
static void time_up_5(void*) { if(up_cb_5) up_cb_5(up_arg_5); }
static void time_up_6(void*) { if(up_cb_6) up_cb_6(up_arg_6); }

static uint8_t volatile topid=1;

uint16_t time_ticker(time_up_cb cb, void* arg, uint32_t every) {
  ret_code_t e;
  switch(topid){
    case 1: {
      up_cb_1=cb;
      up_arg_1=arg;
      e = app_timer_create(&m_timer_1, APP_TIMER_MODE_REPEATED, time_up_1); APP_ERROR_CHECK(e);
      e = app_timer_start(m_timer_1, every? APP_TIMER_TICKS(every): 1, NULL); APP_ERROR_CHECK(e);
      break;
    }
    case 2: {
      up_cb_2=cb;
      up_arg_2=arg;
      e = app_timer_create(&m_timer_2, APP_TIMER_MODE_REPEATED, time_up_2); APP_ERROR_CHECK(e);
      e = app_timer_start(m_timer_2, every? APP_TIMER_TICKS(every): 1, NULL); APP_ERROR_CHECK(e);
      break;
    }
    case 3: {
      up_cb_3=cb;
      up_arg_3=arg;
      e = app_timer_create(&m_timer_3, APP_TIMER_MODE_REPEATED, time_up_3); APP_ERROR_CHECK(e);
      e = app_timer_start(m_timer_3, every? APP_TIMER_TICKS(every): 1, NULL); APP_ERROR_CHECK(e);
      break;
    }
    case 4: {
      up_cb_4=cb;
      up_arg_4=arg;
      e = app_timer_create(&m_timer_4, APP_TIMER_MODE_REPEATED, time_up_4); APP_ERROR_CHECK(e);
      e = app_timer_start(m_timer_4, every? APP_TIMER_TICKS(every): 1, NULL); APP_ERROR_CHECK(e);
      break;
    }
    case 5: {
      up_cb_5=cb;
      up_arg_5=arg;
      e = app_timer_create(&m_timer_5, APP_TIMER_MODE_REPEATED, time_up_5); APP_ERROR_CHECK(e);
      e = app_timer_start(m_timer_5, every? APP_TIMER_TICKS(every): 1, NULL); APP_ERROR_CHECK(e);
      break;
    }
    case 6: {
      up_cb_6=cb;
      up_arg_6=arg;
      e = app_timer_create(&m_timer_6, APP_TIMER_MODE_REPEATED, time_up_6); APP_ERROR_CHECK(e);
      e = app_timer_start(m_timer_6, every? APP_TIMER_TICKS(every): 1, NULL); APP_ERROR_CHECK(e);
      break;
    }
  }
  return topid++;
}

uint16_t time_timeout(time_up_cb cb, void* arg) {
  ret_code_t e;
  switch(topid){
    case 1: {
      up_cb_1=cb;
      up_arg_1=arg;
      e = app_timer_create(&m_timer_1, APP_TIMER_MODE_SINGLE_SHOT, time_up_1); APP_ERROR_CHECK(e);
      break;
    }
    case 2: {
      up_cb_2=cb;
      up_arg_2=arg;
      e = app_timer_create(&m_timer_2, APP_TIMER_MODE_SINGLE_SHOT, time_up_2); APP_ERROR_CHECK(e);
      break;
    }
    case 3: {
      up_cb_3=cb;
      up_arg_3=arg;
      e = app_timer_create(&m_timer_3, APP_TIMER_MODE_SINGLE_SHOT, time_up_3); APP_ERROR_CHECK(e);
      break;
    }
    case 4: {
      up_cb_4=cb;
      up_arg_4=arg;
      e = app_timer_create(&m_timer_4, APP_TIMER_MODE_SINGLE_SHOT, time_up_4); APP_ERROR_CHECK(e);
      break;
    }
    case 5: {
      up_cb_5=cb;
      up_arg_5=arg;
      e = app_timer_create(&m_timer_5, APP_TIMER_MODE_SINGLE_SHOT, time_up_5); APP_ERROR_CHECK(e);
      break;
    }
    case 6: {
      up_cb_6=cb;
      up_arg_6=arg;
      e = app_timer_create(&m_timer_6, APP_TIMER_MODE_SINGLE_SHOT, time_up_6); APP_ERROR_CHECK(e);
      break;
    }
  }
  return topid++;
}

void time_start_timer(uint16_t id, uint32_t timeout)
{
  ret_code_t e;
  switch(id){
    case 1: {
      e = app_timer_start(m_timer_1, timeout? APP_TIMER_TICKS(timeout): 1, NULL); APP_ERROR_CHECK(e);
      break;
    }
    case 2: {
      e = app_timer_start(m_timer_2, timeout? APP_TIMER_TICKS(timeout): 1, NULL); APP_ERROR_CHECK(e);
      break;
    }
    case 3: {
      e = app_timer_start(m_timer_3, timeout? APP_TIMER_TICKS(timeout): 1, NULL); APP_ERROR_CHECK(e);
      break;
    }
    case 4: {
      e = app_timer_start(m_timer_4, timeout? APP_TIMER_TICKS(timeout): 1, NULL); APP_ERROR_CHECK(e);
      break;
    }
    case 5: {
      e = app_timer_start(m_timer_5, timeout? APP_TIMER_TICKS(timeout): 1, NULL); APP_ERROR_CHECK(e);
      break;
    }
    case 6: {
      e = app_timer_start(m_timer_6, timeout? APP_TIMER_TICKS(timeout): 1, NULL); APP_ERROR_CHECK(e);
      break;
    }
  }
}

void time_stop_timer(uint16_t id)
{
  if(id<1 || id>6) return;
  app_timer_stop(timer_ids[id]);
}

void time_end()
{
  app_timer_stop_all();
}

void time_delay_ns(uint32_t ns){

    // REVISIT: can't get this right!
    uint32_t cycles = (ns * 10 / 47 + 62) / 63;

    __asm__ volatile (
        "1: \n"
        "nop \n"
        "subs %[cnt], %[cnt], #1 \n"
        "bne 1b \n"
        : [cnt] "+r" (cycles)
        :
        : "cc"
    );
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

