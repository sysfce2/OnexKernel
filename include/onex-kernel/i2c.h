#ifndef I2C_H
#define I2C_H

#include <stdint.h>

void*   i2c_init(uint16_t speedk);
uint8_t i2c_read(               void* twip, uint8_t address,              uint8_t* buf, uint16_t len);
uint8_t i2c_write(              void* twip, uint8_t address,              uint8_t* buf, uint16_t len);
uint8_t i2c_read_register(      void* twip, uint8_t address, uint8_t reg, uint8_t* buf, uint16_t len);
uint8_t i2c_write_register(     void* twip, uint8_t address, uint8_t reg, uint8_t* buf, uint16_t len);
uint8_t i2c_write_register_byte(void* twip, uint8_t address, uint8_t reg, uint8_t val);
void    i2c_sleep();
void    i2c_wake();

#endif
