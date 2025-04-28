
#include <boards.h>
#include <nrfx_spim.h>
#include "onex-kernel/log.h"
#include <onex-kernel/spi.h>
#include <onex-kernel/gpio.h>

#if defined(NRF52840_XXAA)
static nrfx_spim_t spim_inst = NRFX_SPIM_INSTANCE(3);
#else
static nrfx_spim_t spim_inst = NRFX_SPIM_INSTANCE(0);
#endif

static volatile bool     sending=false;
static volatile uint8_t* curr_data;
static volatile uint16_t curr_len;
static          void     (*spi_done_cb)();

void next_block_of_255()
{
    if(!curr_len){
      if(spi_done_cb) spi_done_cb();
      spi_done_cb=0;
      nrf_gpio_pin_set(SPIM_SS_PIN);
      sending=false;
      return;
    }

    uint8_t m=curr_len>255? 255: curr_len;

    nrfx_spim_xfer_desc_t xfer_desc = NRFX_SPIM_XFER_TRX(curr_data, m, 0, 0);

    curr_data+=m;
    curr_len-=m;

    nrfx_spim_xfer(&spim_inst, &xfer_desc, 0);

#if defined(SPI_BLOCKING)
    nrf_gpio_pin_set(SPIM_SS_PIN);
    sending=false;
#endif
}

void spim_event_handler(nrfx_spim_evt_t const* p_event, void* p_context)
{
    next_block_of_255();
}

// REVISIT: initialised?
nrfx_err_t spi_init()
{
    nrf_gpio_cfg_output(SPIM_SS_PIN);
    nrf_gpio_pin_set(SPIM_SS_PIN);

    nrfx_spim_config_t config = NRFX_SPIM_DEFAULT_CONFIG;

    config.frequency = SPIM_FREQ;
    config.mode      = SPIM_MODE;
    config.bit_order = NRF_SPIM_BIT_ORDER_MSB_FIRST;

    config.miso_pin  = SPIM_MISO_PIN;
    config.mosi_pin  = SPIM_MOSI_PIN;
    config.sck_pin   = SPIM_SCK_PIN;

#if defined(SPI_BLOCKING)
    nrfx_spim_init(&spim_inst, &config, 0, 0);
#else
    nrfx_spim_init(&spim_inst, &config, spim_event_handler, 0);
#endif

    return 0;
}

bool spi_sending()
{
  return sending;
}

void spi_tx(uint8_t* data, uint16_t len, void (*cb)())
{
    sending=true;
    curr_data=data;
    curr_len =len;
    spi_done_cb=cb;

    nrf_gpio_pin_clear(SPIM_SS_PIN);
    spi_wake();

    next_block_of_255();

    if(!cb) while(sending);
}

static bool sleeping=false;
void spi_sleep()
{
  if(sleeping || sending) return;
  sleeping=true;
#if defined(NRF52840_XXAA)
  NRF_SPIM3->ENABLE=(SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos);
#else
  NRF_SPIM0->ENABLE=(SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos);
#endif
}

void spi_wake()
{
  if(!sleeping) return;
  sleeping=false;
#if defined(NRF52840_XXAA)
  NRF_SPIM3->ENABLE=(SPIM_ENABLE_ENABLE_Enabled  << SPIM_ENABLE_ENABLE_Pos);
#else
  NRF_SPIM0->ENABLE=(SPIM_ENABLE_ENABLE_Enabled  << SPIM_ENABLE_ENABLE_Pos);
#endif
}

#if defined(NRF52840_XXAA)

void spi_fast_init() {

  gpio_mode(SPIM_SCK_PIN, OUTPUT);
  gpio_mode(SPIM_MOSI_PIN, OUTPUT);
  gpio_mode(SPIM_SS_PIN, OUTPUT);

  gpio_set(SPIM_SCK_PIN, 1);
  gpio_set(SPIM_MOSI_PIN, 1);
  gpio_set(SPIM_SS_PIN, 1);

  NRF_SPIM3->PSELSCK  = SPIM_SCK_PIN;
  NRF_SPIM3->PSELMOSI = SPIM_MOSI_PIN;
  NRF_SPIM3->PSELMISO = SPIM_MISO_PIN;
  NRF_SPIM3->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M32; // nRF52840 only then
  NRF_SPIM3->INTENSET = 0;
  NRF_SPIM3->ORC = 255;
  NRF_SPIM3->CONFIG = 0;  // MSB first; Mode 0 - see sdk/modules/nrfx/hal/nrf_spim.h
}

void spi_fast_enable(bool state) {
  if (state) NRF_SPIM3->ENABLE = 7;
  else       NRF_SPIM3->ENABLE = 0;
}

static void enable_workaround(NRF_SPIM_Type * spim, uint32_t ppi_channel, uint32_t gpiote_channel)
{
  NRF_GPIOTE->CONFIG[gpiote_channel] = (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
                                       (spim->PSEL.SCK << GPIOTE_CONFIG_PSEL_Pos) |
                                       (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos);

  NRF_PPI->CH[ppi_channel].EEP = (uint32_t) &NRF_GPIOTE->EVENTS_IN[gpiote_channel];
  NRF_PPI->CH[ppi_channel].TEP = (uint32_t) &spim->TASKS_STOP;
  NRF_PPI->CHENSET = 1U << ppi_channel;
}

static void disable_workaround(NRF_SPIM_Type * spim, uint32_t ppi_channel, uint32_t gpiote_channel)
{
  NRF_GPIOTE->CONFIG[gpiote_channel] = 0;
  NRF_PPI->CH[ppi_channel].EEP = 0;
  NRF_PPI->CH[ppi_channel].TEP = 0;
  NRF_PPI->CHENSET = ppi_channel;
}

void spi_fast_write(const uint8_t *ptr, uint32_t len)
{
  if(len == 1) enable_workaround(NRF_SPIM3, 8, 8);
  else         disable_workaround(NRF_SPIM3, 8, 8);

  uint32_t offset = 0;
  do {
    NRF_SPIM3->EVENTS_END = 0;
    NRF_SPIM3->EVENTS_ENDRX = 0;
    NRF_SPIM3->EVENTS_ENDTX = 0;

    NRF_SPIM3->TXD.PTR = (uint32_t)ptr + offset;
    if(len <= 0xFF){
      NRF_SPIM3->TXD.MAXCNT = len;
      offset += len;
      len = 0;
    } else {
      NRF_SPIM3->TXD.MAXCNT = 255;
      offset += 255;
      len -= 255;
    }
    NRF_SPIM3->RXD.PTR = 0;
    NRF_SPIM3->RXD.MAXCNT = 0;

    NRF_SPIM3->TASKS_START = 1;
    while (NRF_SPIM3->EVENTS_END == 0);
    NRF_SPIM3->EVENTS_END = 0;

  } while(len);
}

void spi_fast_read(uint8_t *ptr, uint32_t len)
{
  if(len == 1) enable_workaround(NRF_SPIM3, 8, 8);
  else         disable_workaround(NRF_SPIM3, 8, 8);

  uint32_t offset = 0;
  do {
    NRF_SPIM3->EVENTS_END = 0;
    NRF_SPIM3->EVENTS_ENDRX = 0;
    NRF_SPIM3->EVENTS_ENDTX = 0;

    NRF_SPIM3->TXD.PTR = 0;
    NRF_SPIM3->TXD.MAXCNT = 0;

    NRF_SPIM3->RXD.PTR = (uint32_t)ptr + offset;
    if(len <= 0xFF){
      NRF_SPIM3->RXD.MAXCNT = len;
      offset += len;
      len = 0;
    } else {
      NRF_SPIM3->RXD.MAXCNT = 255;
      offset += 255;
      len -= 255;
    }
    NRF_SPIM3->TASKS_START = 1;
    while (NRF_SPIM3->EVENTS_END == 0);
    NRF_SPIM3->EVENTS_END = 0;

  } while(len);
}

#endif // NRF52840_XXAA

