
#include <onex-kernel/time.h>

static bool initialised=false;

#if defined(NRF51)
#include <nrf51.h>
#include <nrf51_bitfields.h>
#endif

#if defined(NRF52)
#include <nrf52.h>
#include <nrf52_bitfields.h>
#endif

#define LFCLK_FREQUENCY    32768UL
#define TICK_HZ            1000

static void rtc_initialise() __attribute__((constructor));
static void rtc_initialise()
{
    NRF_CLOCK->LFCLKSRC            = (CLOCK_LFCLKSRC_SRC_Synth << CLOCK_LFCLKSRC_SRC_Pos);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_LFCLKSTART    = 1;
    while(!NRF_CLOCK->EVENTS_LFCLKSTARTED);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NVIC_EnableIRQ(RTC0_IRQn);
    NRF_RTC0->PRESCALER = (LFCLK_FREQUENCY / TICK_HZ) - 1;
    NRF_RTC0->EVTENSET = RTC_EVTENSET_TICK_Msk;
    NRF_RTC0->INTENSET = RTC_INTENSET_TICK_Msk;
    NRF_RTC0->TASKS_CLEAR = 1;
    NRF_RTC0->TASKS_START = 1;
}

void time_init()
{
  if(initialised) return;
  time_delay_ms(50);
  initialised=true;
}

static volatile bool     done  = false;
static volatile uint64_t end_time_delay = 0;
static volatile uint64_t clock = 0;

void time_delay_ms(uint32_t ms)
{
    end_time_delay = clock + ms;
/*
    NRF_RTC0->CC[0] = ms;
    NRF_RTC0->EVTENSET = RTC_EVTEN_COMPARE0_Msk;
    NRF_RTC0->INTENSET = RTC_INTENSET_COMPARE0_Msk;
*/
    done=false; while(!done) __WFE();
    end_time_delay = 0;
}

void RTC0_IRQHandler()
{
    if(NRF_RTC0->EVENTS_TICK && (NRF_RTC0->INTENSET & RTC_INTENSET_TICK_Msk))
    {
        NRF_RTC0->EVENTS_TICK = 0;
        clock++;
        if(end_time_delay && clock >= end_time_delay) done=true;
    }
/*
    if(NRF_RTC0->EVENTS_COMPARE[0] && (NRF_RTC0->INTENSET & RTC_INTENSET_COMPARE0_Msk))
    {
        NRF_RTC0->TASKS_CLEAR = 1;
        NRF_RTC0->EVENTS_COMPARE[0] = 0;
        NRF_RTC0->TASKS_START = 1;
        done=true;
    }
*/
}

uint32_t time_ms(){ return clock; }

uint32_t time_us(){ return clock*1000; }

static void __INLINE nrf_delay_us(uint32_t volatile number_of_us) __attribute__((always_inline));
static void __INLINE nrf_delay_us(uint32_t volatile number_of_us)
{
register uint32_t delay __ASM ("r0") = number_of_us;
__ASM volatile (
    ".syntax unified\n"
    "1:\n"
    " SUBS %0, %0, #1\n"
    " NOP\n"
    " NOP\n"
    " NOP\n"
    " NOP\n"
    " NOP\n"
    " NOP\n"
    " NOP\n"
    " NOP\n"
    " NOP\n"
    " NOP\n"
    " NOP\n"
    " BNE 1b\n"
    ".syntax divided\n"
    : "+r" (delay));
}

void time_delay_us(uint32_t us)
{
  nrf_delay_us(us);
}

void time_delay_s( uint32_t s)
{
  time_delay_ms(1000*s);
}

