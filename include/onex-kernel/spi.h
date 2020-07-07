#ifndef SPI_H
#define SPI_H

nrfx_err_t spi_init();
void       spi_tx(uint8_t *tx_data, uint16_t transfer_size, void (*cb)());
void       spi_sleep();
void       spi_wake();

#endif
