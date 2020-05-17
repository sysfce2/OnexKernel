
#include <boards.h>

#include <nrfx_gpiote.h>

#include <onex-kernel/log.h>
#include <onex-kernel/i2c.h>
#include <onex-kernel/time.h>
#include <onex-kernel/motion.h>

#define BMA421_REG_CHIP_ID               0x00
#define BMA421_VAL_CHIP_ID               0x11

#define BMA421_REG_COMMAND               0x7E
#define BMA421_VAL_SOFT_RESET            0xB6

#define BMA421_REG_ACCEL_CONFIG          0X40
#define BMA421_VAL_OUTPUT_DATA_RATE_50HZ 0x07
#define BMA421_VAL_ACCEL_NORMAL_AVG4     0x02
#define BMA421_VAL_CONTINUOUS_MODE       0x01
#define BMA421_VAL_ACCEL_RANGE_4G        0x01

#define BMA421_VAL_ACCEL_ODR_MSK         0x0F
#define BMA421_VAL_ACCEL_BW_POS          0x04
#define BMA421_VAL_ACCEL_PERFMODE_POS    0x07
#define BMA421_VAL_ACCEL_RANGE_MSK       0x03

#define BMA421_REG_POWER_CONTROL         0x7D
#define BMA421_VAL_ACCEL_ENABLE          0x01
#define BMA421_VAL_ACCEL_ENABLE_MSK      0x04
#define BMA421_VAL_ACCEL_ENABLE_POS      0x02

#define BMA421_REG_POWER_CONF            0x7C

#define BMA421_REG_DATA_8                0X12

static motion_change_cb motion_cb = 0;

static void* twip;

static void nrfx_gpiote_evt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  NRF_LOG_DEBUG("nrfx_gpiote_evt_handler");
}

static void show_reg(char* name, uint8_t reg)
{
  uint8_t val;
  i2c_read_register(twip, MOTION_ADDRESS, reg, &val, 1);
  NRF_LOG_DEBUG("%s %x", name, val);
}

int motion_init(motion_change_cb cb)
{
  motion_cb = cb;

  nrf_gpio_cfg_sense_input(MOTION_IRQ_PIN, (nrf_gpio_pin_pull_t)GPIO_PIN_CNF_PULL_Pullup, (nrf_gpio_pin_sense_t)GPIO_PIN_CNF_SENSE_Low);

  nrfx_gpiote_in_config_t pinConfig;
  pinConfig.skip_gpio_setup = true;
  pinConfig.hi_accuracy = false;
  pinConfig.is_watcher = false;
  pinConfig.sense = (nrf_gpiote_polarity_t)NRF_GPIOTE_POLARITY_HITOLO;
  pinConfig.pull = (nrf_gpio_pin_pull_t)GPIO_PIN_CNF_PULL_Pullup;
  nrfx_gpiote_in_init(MOTION_IRQ_PIN, &pinConfig, nrfx_gpiote_evt_handler);

  twip=i2c_init(400);

  uint8_t e;

  uint8_t chip_id=0;
  e=i2c_read_register(twip, MOTION_ADDRESS, BMA421_REG_CHIP_ID, &chip_id, 1);
  if(e || chip_id!=BMA421_VAL_CHIP_ID) { NRF_LOG_DEBUG("chip id err"); return 1; }

  uint8_t reset_command=BMA421_VAL_SOFT_RESET;
  e=i2c_write_register_byte(twip, MOTION_ADDRESS, BMA421_REG_COMMAND, reset_command);
  if(e) { NRF_LOG_DEBUG("soft reset err"); return 1; }

  time_delay_ms(50);

  uint8_t odr       = BMA421_VAL_OUTPUT_DATA_RATE_50HZ;
  uint8_t bandwidth = BMA421_VAL_ACCEL_NORMAL_AVG4;
  uint8_t perf_mode = BMA421_VAL_CONTINUOUS_MODE;
  uint8_t range     = BMA421_VAL_ACCEL_RANGE_4G;
  uint8_t accel_config1;
  uint8_t accel_config2;
  accel_config1  = odr & BMA421_VAL_ACCEL_ODR_MSK;
  accel_config1 |= (uint8_t)(bandwidth << BMA421_VAL_ACCEL_BW_POS);
  accel_config1 |= (uint8_t)(perf_mode << BMA421_VAL_ACCEL_PERFMODE_POS);
  accel_config2  = range & BMA421_VAL_ACCEL_RANGE_MSK;
  e=i2c_write_register_byte(twip, MOTION_ADDRESS, BMA421_REG_ACCEL_CONFIG,   accel_config1);
  if(e) { NRF_LOG_DEBUG("accel config 1 err"); return 1; }
  e=i2c_write_register_byte(twip, MOTION_ADDRESS, BMA421_REG_ACCEL_CONFIG+1, accel_config2);
  if(e) { NRF_LOG_DEBUG("accel config 2 err"); return 1; }

  uint8_t power_control;
  e=i2c_read_register(twip, MOTION_ADDRESS, BMA421_REG_POWER_CONTROL, &power_control, 1);
  if(e) { NRF_LOG_DEBUG("read power control err"); return 1; }
  power_control = ((power_control & ~BMA421_VAL_ACCEL_ENABLE_MSK) | ((BMA421_VAL_ACCEL_ENABLE<<BMA421_VAL_ACCEL_ENABLE_POS) & BMA421_VAL_ACCEL_ENABLE_MSK));
  e=i2c_write_register_byte(twip, MOTION_ADDRESS, BMA421_REG_POWER_CONTROL, power_control);
  if(e) { NRF_LOG_DEBUG("write power control err"); return 1; }

  uint8_t power_conf;
  e=i2c_read_register(twip, MOTION_ADDRESS, BMA421_REG_POWER_CONF, &power_conf, 1);
  if(e) { NRF_LOG_DEBUG("read power conf err"); return 1; }
  power_conf=0; // ..
  e=i2c_write_register_byte(twip, MOTION_ADDRESS, BMA421_REG_POWER_CONF, power_conf);
  if(e) { NRF_LOG_DEBUG("write power conf err"); return 1; }

  show_reg("chip id",       BMA421_REG_CHIP_ID);
  show_reg("accel1",        BMA421_REG_ACCEL_CONFIG);
  show_reg("accel2",        BMA421_REG_ACCEL_CONFIG+1);
  show_reg("power control", BMA421_REG_POWER_CONTROL);
  show_reg("power conf",    BMA421_REG_POWER_CONF);

  return 0;
}

motion_info_t motion_get_info() {

  uint8_t e;
  motion_info_t info = {0};

  uint8_t data[6] = {0};
  e=i2c_read_register(twip, MOTION_ADDRESS, BMA421_REG_DATA_8 , data, 6);
  if(e) { NRF_LOG_DEBUG("read power conf err"); return info; }

  uint16_t lsb = 0;
  uint16_t msb = 0;

  msb = data[1];
  lsb = data[0];

  info.x = (int16_t)((msb << 8) | lsb);

  msb = data[3];
  lsb = data[2];

  info.y = (int16_t)((msb << 8) | lsb);

  msb = data[5];
  lsb = data[4];

  info.z = (int16_t)((msb << 8) | lsb);

  info.x = info.x / 0x10;
  info.y = info.y / 0x10;
  info.z = info.z / 0x10;

  return info;
}
