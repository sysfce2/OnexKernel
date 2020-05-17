#include <nrfx_twi.h>
#include <boards.h>

#include <onex-kernel/log.h>
#include <onex-kernel/i2c.h>

nrfx_twi_t twi = NRFX_TWI_INSTANCE(1);

bool initialized=false;

void* i2c_init(int speedk)
{
  if(initialized) return (void*)&twi;
  nrfx_twi_config_t twiConfig;
  twiConfig.frequency = speedk==400? NRF_TWI_FREQ_400K: speedk==250? NRF_TWI_FREQ_250K: NRF_TWI_FREQ_100K;
  twiConfig.sda = SDA_PIN;
  twiConfig.scl = SCL_PIN;
  twiConfig.interrupt_priority = NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY;
  twiConfig.hold_bus_uninit = NRFX_TWI_DEFAULT_CONFIG_HOLD_BUS_UNINIT;
  nrfx_twi_init(&twi, &twiConfig, 0, 0);
  nrfx_twi_enable(&twi);
  initialized=true;
  return (void*)&twi;
}

uint8_t i2c_read(void* twip, uint8_t address, uint8_t* buf, uint16_t len)
{
  ret_code_t e=nrfx_twi_rx((nrfx_twi_t*)twip, address, buf, len);
  return e? 1: 0;
}

uint8_t i2c_write(void* twip, uint8_t address, uint8_t* buf, uint16_t len)
{
  ret_code_t e=nrfx_twi_tx((nrfx_twi_t*)twip, address, buf, len, false);
  return e? 1: 0;
}

void i2c_disable(void* twip)
{
  nrfx_twi_disable((nrfx_twi_t*)twip);
  nrf_gpio_cfg_default(SDA_PIN);
  nrf_gpio_cfg_default(SCL_PIN);
}
