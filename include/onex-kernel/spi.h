#ifndef SPI_H
#define SPI_H

nrfx_err_t spi_init();
void       spi_tx(uint8_t *data, uint16_t len, void (*cb)());
void       spi_sleep();
void       spi_wake();

#endif
