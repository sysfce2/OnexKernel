#ifndef SEESAW_H
#define SEESAW_H

#include <stdint.h>
#include <stdbool.h>

uint16_t seesaw_status_version_hi(uint16_t i2c_address);
uint16_t seesaw_status_version_lo(uint16_t i2c_address);
void     seesaw_gpio_input_pullup(uint16_t i2c_address, uint8_t pin);
bool     seesaw_gpio_read(        uint16_t i2c_address, uint8_t pin);
int32_t  seesaw_encoder_position( uint16_t i2c_address);

#endif
