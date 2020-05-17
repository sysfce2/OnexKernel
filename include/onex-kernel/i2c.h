#ifndef I2C_H
#define I2C_H

void*   i2c_init(int speedk);
uint8_t i2c_read(void* twip, uint8_t address, uint8_t* buf, uint16_t len);
uint8_t i2c_write(void* twi, uint8_t address, uint8_t* buf, uint16_t len);
void    i2c_disable(void* twi);

#endif
