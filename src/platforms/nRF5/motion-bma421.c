
#include <math.h>

#include <boards.h>

#include <onex-kernel/log.h>
#include <onex-kernel/i2c.h>
#include <onex-kernel/time.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/motion.h>

#define BMA421_REG_CHIP_ID               0x00
#define BMA421_VAL_CHIP_ID_BMA421        0x11

#define BMA421_REG_DATA_8                0X12

#define BMA421_REG_ACCEL_CONFIG          0x40
#define BMA421_VAL_ODR_12P5HZ            0x05
#define BMA421_VAL_ODR_25HZ              0x06
#define BMA421_VAL_ODR_50HZ              0x07
#define BMA421_VAL_BWP_NORM_AVG4         0x02
#define BMA421_VAL_BWP_CIC_AVG8          0x03
#define BMA421_VAL_PERF_MODE_CIC_AVG     0x00
#define BMA421_VAL_PERF_MODE_CONT        0x01

#define BMA421_REG_ACCEL_RANGE           0x41
#define BMA421_VAL_ACCEL_RANGE_2G        0x00
#define BMA421_VAL_ACCEL_RANGE_4G        0x01

#define BMA421_VAL_ACCEL_ODR_MSK         0x0F
#define BMA421_VAL_ACCEL_BW_POS             4
#define BMA421_VAL_ACCEL_PERFMODE_POS       7
#define BMA421_VAL_ACCEL_RANGE_MSK       0x03

#define BMA421_REG_INT1_IO_CONTROL       0x53
#define BMA421_VAL_INT_ACTIVE_LOW        0x0D

#define BMA421_REG_INT_MAP_DATA          0x58
#define BMA421_VAL_INT_MAP_ON            0x01
#define BMA421_VAL_INT1_DATA_READY_POS      2

#define BMA421_REG_POWER_CONF            0x7C
#define BMA421_VAL_APS_OFF               0x00
#define BMA421_VAL_APS_ON                0x01

#define BMA421_REG_POWER_CONTROL         0x7D
#define BMA421_VAL_ACCEL_ON              0x01
#define BMA421_VAL_ACCEL_ENABLE_MSK      0x04
#define BMA421_VAL_ACCEL_ENABLE_POS         2

#define BMA421_REG_COMMAND               0x7E
#define BMA421_VAL_SOFT_RESET            0xB6

static motion_change_cb motion_cb = 0;

static void* twip;

static motion_info_t mi={0};

static void moved(uint8_t pin, uint8_t type) {
  motion_get_info();
  if(motion_cb) motion_cb(mi);
}

static void show_reg(char* name, uint8_t reg)
{
  uint8_t val;
  i2c_read_register(twip, MOTION_ADDRESS, reg, &val, 1);
  log_write("%s %x", name, val);
}

int motion_init(motion_change_cb cb)
{
  motion_cb = cb;

  twip=i2c_init(400);

  uint8_t e;

  uint8_t chip_id=0;
  e=i2c_read_register(twip, MOTION_ADDRESS, BMA421_REG_CHIP_ID, &chip_id, 1);
  if(e || chip_id!=BMA421_VAL_CHIP_ID_BMA421) { log_write("chip id err"); return 1; }

  uint8_t reset_command=BMA421_VAL_SOFT_RESET;
  e=i2c_write_register_byte(twip, MOTION_ADDRESS, BMA421_REG_COMMAND, reset_command);
  if(e) { log_write("soft reset err"); return 1; }

  time_delay_ms(50);

  uint8_t odr       = BMA421_VAL_ODR_12P5HZ;
  uint8_t bandwidth = BMA421_VAL_BWP_CIC_AVG8;
  uint8_t perf_mode = BMA421_VAL_PERF_MODE_CIC_AVG;
  uint8_t range     = BMA421_VAL_ACCEL_RANGE_4G;
  uint8_t accel_config;
  uint8_t accel_range;
  accel_config  = odr & BMA421_VAL_ACCEL_ODR_MSK;
  accel_config |= (uint8_t)(bandwidth << BMA421_VAL_ACCEL_BW_POS);
  accel_config |= (uint8_t)(perf_mode << BMA421_VAL_ACCEL_PERFMODE_POS);
  accel_range   = range & BMA421_VAL_ACCEL_RANGE_MSK;
  e=i2c_write_register_byte(twip, MOTION_ADDRESS, BMA421_REG_ACCEL_CONFIG, accel_config);
  if(e) { log_write("accel config err"); return 1; }
  e=i2c_write_register_byte(twip, MOTION_ADDRESS, BMA421_REG_ACCEL_RANGE, accel_range);
  if(e) { log_write("accel range err"); return 1; }

  uint8_t power_conf=BMA421_VAL_APS_ON;
  e=i2c_write_register_byte(twip, MOTION_ADDRESS, BMA421_REG_POWER_CONF, power_conf);
  if(e) { log_write("power conf err"); return 1; }

  uint8_t int_conf=BMA421_VAL_INT_ACTIVE_LOW;
  e=i2c_write_register_byte(twip, MOTION_ADDRESS, BMA421_REG_INT1_IO_CONTROL, int_conf);
  if(e) { log_write("interrupt conf err"); return 1; }

  uint8_t int_map=(BMA421_VAL_INT_MAP_ON<<BMA421_VAL_INT1_DATA_READY_POS);
  e=i2c_write_register_byte(twip, MOTION_ADDRESS, BMA421_REG_INT_MAP_DATA, int_map);
  if(e) { log_write("interrupt map err"); return 1; }

  uint8_t power_control;
  e=i2c_read_register(twip, MOTION_ADDRESS, BMA421_REG_POWER_CONTROL, &power_control, 1);
  if(e) { log_write("read power control err"); return 1; }
  power_control = ((power_control & ~BMA421_VAL_ACCEL_ENABLE_MSK) | ((BMA421_VAL_ACCEL_ON<<BMA421_VAL_ACCEL_ENABLE_POS) & BMA421_VAL_ACCEL_ENABLE_MSK));
  e=i2c_write_register_byte(twip, MOTION_ADDRESS, BMA421_REG_POWER_CONTROL, power_control);
  if(e) { log_write("write power control err"); return 1; }

  show_reg("chip id",       BMA421_REG_CHIP_ID);
  show_reg("accel conf",    BMA421_REG_ACCEL_CONFIG);
  show_reg("accel range",   BMA421_REG_ACCEL_RANGE);
  show_reg("power control", BMA421_REG_POWER_CONTROL);
  show_reg("power conf",    BMA421_REG_POWER_CONF);

  gpio_mode_cb(MOTION_IRQ_PIN, INPUT_PULLUP, FALLING, moved);

  return 0;
}

#define ONE_G 1060

motion_info_t motion_get_info()
{
  uint8_t e;
  uint8_t xyz[6]={0};
  e=i2c_read_register(twip, MOTION_ADDRESS, BMA421_REG_DATA_8, xyz, sizeof(xyz));
  if(e) return mi;

  uint16_t lsb = 0;
  uint16_t msb = 0;

  msb = xyz[1];
  lsb = xyz[0];

  mi.x = (int16_t)((msb << 8) | lsb);

  msb = xyz[3];
  lsb = xyz[2];

  mi.y = (int16_t)((msb << 8) | lsb);

  msb = xyz[5];
  lsb = xyz[4];

  mi.z = (int16_t)((msb << 8) | lsb);

  // 16bit +-32768=+-4g so /8=1024 per g (except 1g=ONE_G..?)
  mi.x /= 8;
  mi.y /= 8;
  mi.z /= 8;

  mi.m = ((int16_t)sqrtf(mi.x*mi.x+mi.y*mi.y+mi.z*mi.z))-ONE_G;

  return mi;
}
