
#include <onex-kernel/i2c.h>
#include <onex-kernel/seesaw.h>
#include <onex-kernel/log.h>

#define SEESAW_HI_STATUS         0x00
#define SEESAW_HI_GPIO           0x01
#define SEESAW_HI_SERCOM0        0x02
#define SEESAW_HI_TIMER          0x08
#define SEESAW_HI_ADC            0x09
#define SEESAW_HI_DAC            0x0A
#define SEESAW_HI_INTERRUPT      0x0B
#define SEESAW_HI_DAP            0x0C
#define SEESAW_HI_EEPROM         0x0D
#define SEESAW_HI_NEOPIXEL       0x0E
#define SEESAW_HI_TOUCH          0x0F
#define SEESAW_HI_KEYPAD         0x10
#define SEESAW_HI_ENCODER        0x11
#define SEESAW_HI_SPECTRUM       0x12

#define SEESAW_LO_STATUS_HW_ID   0x01
#define SEESAW_LO_STATUS_VERSION 0x02
#define SEESAW_LO_STATUS_OPTIONS 0x03
#define SEESAW_LO_STATUS_TEMP    0x04
#define SEESAW_LO_STATUS_SWRST   0x7F

#define SEESAW_LO_ENCODER_STATUS   0x00
#define SEESAW_LO_ENCODER_INTENSET 0x10
#define SEESAW_LO_ENCODER_INTENCLR 0x20
#define SEESAW_LO_ENCODER_POSITION 0x30
#define SEESAW_LO_ENCODER_DELTA    0x40

static void* i2c=0;

static uint32_t seesaw_status_version(uint16_t i2c_address){

  if(!i2c) i2c=i2c_init(400);

  uint8_t e;

  uint8_t data[4];
  e=i2c_read_register_hi_lo(i2c, i2c_address, SEESAW_HI_STATUS, SEESAW_LO_STATUS_VERSION, data, 4);
  if(e) return 0;

  uint32_t version = ((uint32_t)data[0] << 24) |
                     ((uint32_t)data[1] << 16) |
                     ((uint32_t)data[2] <<  8) |
                     ((uint32_t)data[3]      );
  return version;
}

uint16_t seesaw_status_version_hi(uint16_t i2c_address){
  uint32_t version = seesaw_status_version(i2c_address);
  uint16_t version_hi = ((version >> 16) & 0xFFFF);
  return version_hi;
}

uint16_t seesaw_status_version_lo(uint16_t i2c_address){
  uint32_t version = seesaw_status_version(i2c_address);
  uint16_t version_lo = (version & 0xFFFF);
  return version_lo;
}

int32_t seesaw_encoder_position(uint16_t i2c_address) {

  if(!i2c) i2c=i2c_init(400);

  uint8_t e;

  uint8_t data[4];
  e=i2c_read_register_hi_lo(i2c, i2c_address, SEESAW_HI_ENCODER, SEESAW_LO_ENCODER_POSITION, data, 4);
  if(e) return 0;

  int32_t position = ((uint32_t)data[0] << 24) |
                     ((uint32_t)data[1] << 16) |
                     ((uint32_t)data[2] <<  8) |
                     ((uint32_t)data[3]      );

  return position;
}













