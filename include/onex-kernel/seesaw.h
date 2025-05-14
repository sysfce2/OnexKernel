#ifndef SEESAW_H
#define SEESAW_H

#include <stdint.h>

uint16_t seesaw_status_version_hi(uint16_t i2c_address);
uint16_t seesaw_status_version_lo(uint16_t i2c_address);

int32_t  seesaw_encoder_position(uint16_t i2c_address);

#endif
