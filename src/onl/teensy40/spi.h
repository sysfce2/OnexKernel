#ifndef SPI_H
#define SPI_H

#include <stdint.h>

#define LSBFIRST 0
#define MSBFIRST 1

#define SPI_MODE0 0x00
#define SPI_MODE1 0x01
#define SPI_MODE2 0x02
#define SPI_MODE3 0x03

void     spi_begin(uint8_t sclk, uint8_t mosi, uint8_t miso);
void     spi_end();
void     spi_set_bit_order(uint8_t bit_order);
void     spi_set_data_mode(uint8_t data_mode);
void     spi_set_speed(uint32_t kbps);
uint8_t  spi_transfer(uint8_t data);
uint8_t  spi_transfer_8( uint8_t data);
uint16_t spi_transfer_16(uint16_t data);
uint32_t spi_transfer_32(uint32_t data);
void     spi_transfer_buf_16(uint16_t* buf, size_t n);
void     spi_set_clock_div(uint32_t div);

void    spi_sw_begin(uint8_t sclk, uint8_t mosi, uint8_t miso);
void    spi_sw_set_bit_order(uint8_t bit_order);
void    spi_sw_set_data_mode(uint8_t data_mode);
uint8_t spi_sw_transfer(uint8_t data);

#endif
