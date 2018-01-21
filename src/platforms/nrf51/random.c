
#include <onex-kernel/random.h>

#include <nrf51.h>
#include <nrf51_bitfields.h>

uint8_t random_byte()
{
  volatile uint8_t random_value;

  NRF_RNG->CONFIG = RNG_CONFIG_DERCEN_Enabled << RNG_CONFIG_DERCEN_Pos;
  NRF_RNG->SHORTS = RNG_SHORTS_VALRDY_STOP_Enabled << RNG_SHORTS_VALRDY_STOP_Pos;
  NRF_RNG->EVENTS_VALRDY = 0;
  NRF_RNG->TASKS_START = 1;
  while (NRF_RNG->EVENTS_VALRDY == 0);
  NRF_RNG->EVENTS_VALRDY = 0;
  random_value = NRF_RNG->VALUE;
  return random_value;
}
