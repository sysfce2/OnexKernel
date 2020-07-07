#ifndef SPI_H
#define SPI_H

nrfx_err_t spi_init();
void       spi_tx(uint16_t transfer_size, uint8_t *tx_data, void (*cb)());
void       spi_sleep();
void       spi_wake();

#endif
