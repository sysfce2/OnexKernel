
#include <math.h>

#include <boards.h>

#include <onex-kernel/log.h>
#include <onex-kernel/i2c.h>
#include <onex-kernel/time.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/compass.h>

#define COMPASS_ADDRESS 0x1C

#define LIS3MDL_REG_WHO_AM_I         0x0F // 0011 1101 = 3D

#define LIS3MDL_REG_CTRL_REG1        0x20 // TEMP_EN   | OM1  | OM0  | DO2 | DO1    | DO0      | FAST_ODR |  ST
#define LIS3MDL_REG_CTRL_REG2        0x21 // 0         | FS1  | FS0  | 0   | REBOOT | SOFT_RST | 0        |  0
#define LIS3MDL_REG_CTRL_REG3        0x22 // 0         | 0    | LP   | 0   | 0      | SIM      | MD1      | MD0
#define LIS3MDL_REG_CTRL_REG4        0x23 // 0         | 0    | 0    | 0   | OMZ1   | OMZ0     | BLE      |   0
#define LIS3MDL_REG_CTRL_REG5        0x24 // FAST_READ | BDU  | 0    | 0   | 0      | 0        | 0        |   0

#define LIS3MDL_REG_STATUS_REG       0x27 // ZYXOR     | ZOR  | YOR  | XOR | ZYXDA  | ZDA      | YDA      | XDA
#define LIS3MDL_REG_OUT_X_L          0x28 // read XYZ data up from here x6
#define LIS3MDL_REG_TEMP_OUT_L       0x2E // read temperature data up from here x2

#define LIS3MDL_REG_INT_CFG          0x30 // XIEN  | YIEN  | ZIEN  | 0     | 0     | IEA   | LIR   | IEN
#define LIS3MDL_REG_INT_SRC          0x31 // PTH_X | PTH_Y | PTH_Z | NTH_X | NTH_Y | NTH_Z | MROI  | INT
#define LIS3MDL_REG_INT_THS_L        0x32 // IRQ threshold
#define LIS3MDL_REG_INT_THS_H        0x33 //

#define CTRL_REG1_TEMP_EN__LP__80HZ 0b10011100 // 0x9C
#define CTRL_REG1_TEMP_EN_UHP__80HZ 0b11111100 // 0xFC
#define CTRL_REG1_TEMP_EN_UHP_155HZ 0b11100010 // 0xE2
#define CTRL_REG3_CONTINUOUS_OPERTN 0b00000000 // 0x00
#define CTRL_REG4_________UHP_ZAXIS 0b00001100 // 0x0C
#define INT_CFG_NO_INTERRUPT_ENABLE 0b00000000 // 0x00

#define SCALE_4_GAUSS 6842

#define ERRCHK(s,r) \
  do{ \
    uint8_t e=(s); \
    if(e){ \
      log_write("%s:%d register read/write err\n", __FILE__, __LINE__); \
      return r; \
    } \
  }while(0)

static void* i2c;

static bool initialised=false;
static compass_info_t ci={0};

bool compass_init(){

  if(initialised) return true;

  i2c=i2c_init(400);

  uint8_t chip_id=0;
  ERRCHK(i2c_read_register(i2c, COMPASS_ADDRESS, LIS3MDL_REG_WHO_AM_I, &chip_id, 1), false);
  if(chip_id!=0x3d){ log_write("chip id %x!=0x3d err\n", chip_id); return false; }

  ERRCHK(i2c_write_register_byte(i2c, COMPASS_ADDRESS, LIS3MDL_REG_CTRL_REG1, CTRL_REG1_TEMP_EN_UHP__80HZ), false);
  ERRCHK(i2c_write_register_byte(i2c, COMPASS_ADDRESS, LIS3MDL_REG_CTRL_REG4, CTRL_REG4_________UHP_ZAXIS), false);
  ERRCHK(i2c_write_register_byte(i2c, COMPASS_ADDRESS, LIS3MDL_REG_CTRL_REG3, CTRL_REG3_CONTINUOUS_OPERTN), false);
  ERRCHK(i2c_write_register_byte(i2c, COMPASS_ADDRESS, LIS3MDL_REG_INT_CFG,   INT_CFG_NO_INTERRUPT_ENABLE), false);

  time_delay_ms(10);

  initialised=true;

  return true;
}

static int16_t heading_from_xyz(int16_t x, int16_t y, int16_t z){

  // REVISIT: use of float?
  float fx = (float)x;
  float fy = (float)y;
  float fz = (float)z;

  float norm = sqrtf(fx*fx + fy*fy + fz*fz);
  if(norm < 1e-6f) return 0;

  fx /= norm;
  fy /= norm;
//fz /= norm;  // not used

  float heading_rad = atan2f(fy, fx);
  float heading_deg = heading_rad * (180.0f / M_PI);

  if(heading_deg < -180.0f) heading_deg += 360.0f;
  if(heading_deg >  180.0f) heading_deg -= 360.0f;

  return (int16_t)(heading_deg);
}

compass_info_t compass_direction(){

  if(!initialised) return ci;

  uint8_t status;
  ERRCHK(i2c_read_register(i2c, COMPASS_ADDRESS, LIS3MDL_REG_STATUS_REG, &status, 1), ci);
  if(!(status & 0x0f)) return ci;

  int16_t xyz[3];
  ERRCHK(i2c_read_register(i2c, COMPASS_ADDRESS, LIS3MDL_REG_OUT_X_L, (uint8_t*)xyz, 6), ci);

  int32_t x = xyz[0];
  int32_t y = xyz[1];
  int32_t z = xyz[2];

  x = (x * 1000) / SCALE_4_GAUSS;
  y = (y * 1000) / SCALE_4_GAUSS;
  z = (z * 1000) / SCALE_4_GAUSS;

  static int16_t x_min= 10000;
  static int16_t x_max=-10000;
  static int16_t y_min= 10000;
  static int16_t y_max=-10000;
  static int16_t z_min= 10000;
  static int16_t z_max=-10000;

  if(x < x_min) x_min = x;
  if(x > x_max) x_max = x;
  if(y < y_min) y_min = y;
  if(y > y_max) y_max = y;
  if(z < z_min) z_min = z;
  if(z > z_max) z_max = z;

  int16_t x_range = (x_max - x_min);
  int16_t y_range = (y_max - y_min);
  int16_t z_range = (z_max - z_min);

  int16_t x_offset = (x_max + x_min) / 2;
  int16_t y_offset = (y_max + y_min) / 2;
  int16_t z_offset = (z_max + z_min) / 2;

  ci.x = (x_range > 300)? ((x - x_offset) * 800) / x_range: x;
  ci.y = (y_range > 300)? ((y - y_offset) * 800) / y_range: y;
  ci.z = (z_range > 300)? ((z - z_offset) * 800) / z_range: z;

  ci.o = heading_from_xyz(ci.x, ci.y, ci.z);

  return ci;
}

uint8_t compass_temperature(){
  if(!initialised) return -127;
  uint16_t data;
  ERRCHK(i2c_read_register(i2c, COMPASS_ADDRESS, LIS3MDL_REG_TEMP_OUT_L, (uint8_t*)&data, 2), -127);
  return data/8+25; // "8 LSB/°C and 0 output means T=25 °C"
}












