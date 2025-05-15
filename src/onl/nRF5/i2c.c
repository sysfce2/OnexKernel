#include <nrfx_twi.h>
#include <boards.h>

#include <onex-kernel/log.h>
#include <onex-kernel/time.h>
#include <onex-kernel/i2c.h>

nrfx_twi_t twi = NRFX_TWI_INSTANCE(1);

bool initialized=false;

void* i2c_init(uint16_t speedk) {
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
  i2c_wake();
  ret_code_t e=nrfx_twi_rx((nrfx_twi_t*)twip, address, buf, len);
  return e? 1: 0;
}

uint8_t i2c_write(void* twip, uint8_t address, uint8_t* buf, uint16_t len)
{
  i2c_wake();
  ret_code_t e=nrfx_twi_tx((nrfx_twi_t*)twip, address, buf, len, false);
  return e? 1: 0;
}

uint8_t i2c_read_register(void* twip, uint8_t address, uint8_t reg, uint8_t* buf, uint16_t len)
{
  ret_code_t e;

  i2c_wake();

  e=nrfx_twi_tx((nrfx_twi_t*)twip, address, &reg, 1, true);
  if(e) return 1;

  time_delay_us(800);  // costs cpu and battery .. maybe use a timer?

  e=nrfx_twi_rx((nrfx_twi_t*)twip, address, buf, len);
  if(e) return 1;

  time_delay_us(300);

  return 0;
}

uint8_t i2c_write_register(void* twip, uint8_t address, uint8_t reg, uint8_t* buf, uint16_t len)
{
  ret_code_t e;

  i2c_wake();
  e=nrfx_twi_tx((nrfx_twi_t*)twip, address, &reg, 1, true);
  if(e) return 1;

  time_delay_us(800);  // costs cpu and battery .. maybe use a timer?

  e=nrfx_twi_tx((nrfx_twi_t*)twip, address, buf, len, false);
  if(e) return 1;

  time_delay_us(300);

  return 0;
}

uint8_t i2c_write_register_byte(void* twip, uint8_t address, uint8_t reg, uint8_t val)
{
  uint8_t buf[2] = { reg, val };

  ret_code_t e;

  i2c_wake();
  e=nrfx_twi_tx((nrfx_twi_t*)twip, address, buf, 2, false);
  if(e) return 1;

  time_delay_us(300);

  return 0;
}

uint8_t i2c_read_register_hi_lo(void* twip, uint8_t address, uint8_t reg_hi,
                                                             uint8_t reg_lo, uint8_t* buf, uint16_t len){
  ret_code_t e;

  i2c_wake();

  uint8_t reg[] = { reg_hi, reg_lo };

  e=nrfx_twi_tx((nrfx_twi_t*)twip, address, reg, 2, false); // REVISIT: why not ", true);"?
  if(e) return 1;

  time_delay_us(250);

  e=nrfx_twi_rx((nrfx_twi_t*)twip, address, buf, len);
  if(e) return 1;

  return 0;
}

uint8_t i2c_write_register_hi_lo(void* twip, uint8_t address, uint8_t reg_hi,
                                                              uint8_t reg_lo, uint8_t* buf, uint16_t len){
  ret_code_t e;

  i2c_wake();

  uint8_t reg[] = { reg_hi, reg_lo };

  e=nrfx_twi_tx((nrfx_twi_t*)twip, address, reg, 2, false); // REVISIT: why not ", true);"?
  if(e) return 1;

  time_delay_us(250);

  e=nrfx_twi_tx((nrfx_twi_t*)twip, address, buf, len, false);
  if(e) return 1;

  return 0;
}

static bool sleeping=false;
void i2c_sleep()
{
  if(sleeping) return;
  sleeping=true;
  NRF_TWI1->ENABLE=(TWI_ENABLE_ENABLE_Disabled << TWI_ENABLE_ENABLE_Pos);
}

void i2c_wake()
{
  if(!sleeping) return;
  sleeping=false;
  NRF_TWI1->ENABLE=(TWI_ENABLE_ENABLE_Enabled << TWI_ENABLE_ENABLE_Pos);
}

//if(e) NRF_LOG_DEBUG("nrfx_twi addr=%x reg=%x val=%x err=%s", address, reg, val, nrf_strerror_get(e)); log_loop();

