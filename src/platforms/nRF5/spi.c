
#include <boards.h>
#include <nrfx_spim.h>
#include "onex-kernel/log.h"
#include <onex-kernel/spi.h>

static nrfx_spim_t spim_inst0 = NRFX_SPIM_INSTANCE(0);

static volatile bool     sending=false;
static volatile uint8_t* curr_data;
static volatile uint16_t curr_len;
static          void     (*spi_done_cb)();

void next_block_of_255()
{
    if(!curr_len){
      if(spi_done_cb) spi_done_cb();
      spi_done_cb=0;
      nrf_gpio_pin_set(SPIM0_SS_PIN);
      sending=false;
      return;
    }

    uint8_t m=curr_len>255? 255: curr_len;

    nrfx_spim_xfer_desc_t xfer_desc = NRFX_SPIM_XFER_TRX(curr_data, m, 0, 0);

    curr_data+=m;
    curr_len-=m;

    nrfx_spim_xfer(&spim_inst0, &xfer_desc, 0);
}

void spim_event_handler(nrfx_spim_evt_t const* p_event, void* p_context)
{
    next_block_of_255();
}

nrfx_err_t spi_init()
{
    nrf_gpio_cfg_output(SPIM0_SS_PIN);
    nrf_gpio_pin_set(SPIM0_SS_PIN);

    nrfx_spim_config_t config = NRFX_SPIM_DEFAULT_CONFIG;

    config.frequency = NRF_SPIM_FREQ_8M;
    config.mode      = NRF_SPIM_MODE_3;
    config.bit_order = NRF_SPIM_BIT_ORDER_MSB_FIRST;

    config.miso_pin  = SPIM0_MISO_PIN;
    config.mosi_pin  = SPIM0_MOSI_PIN;
    config.sck_pin   = SPIM0_SCK_PIN;

    nrfx_spim_init(&spim_inst0, &config, spim_event_handler, 0);

    return 0;
}

void spi_tx(uint8_t* data, uint16_t len, void (*cb)())
{
    if(sending){
      log_write("spi_tx already sending");
      while(sending);
      log_write("spi_tx ready");
    }
    sending=true;
    curr_data=data;
    curr_len =len;
    spi_done_cb=cb;

    nrf_gpio_pin_clear(SPIM0_SS_PIN);
    spi_wake();

    next_block_of_255();

    if(!cb) while(sending);
}

static bool sleeping=false;
void spi_sleep()
{
  if(sleeping || sending) return;
  sleeping=true;
  NRF_SPIM0->ENABLE=(SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos);
}

void spi_wake()
{
  if(!sleeping) return;
  sleeping=false;
  NRF_SPIM0->ENABLE=(SPIM_ENABLE_ENABLE_Enabled  << SPIM_ENABLE_ENABLE_Pos);
}

