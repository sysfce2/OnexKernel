#ifndef SPI_H
#define SPI_H

nrfx_err_t spi_init();
void       spi_tx(uint8_t *data, uint16_t len, void (*cb)());
bool       spi_sending();
void       spi_sleep();
void       spi_wake();

// Fast SPIM3 @ 32Mhz with DMA and other goodies
// Thanks to the legendary ATC1441
#if defined(NRF52840_XXAA)
void spi_fast_init();
void spi_fast_enable(bool state);
void spi_fast_write(const uint8_t *ptr, uint32_t len);
void spi_fast_read(uint8_t *ptr, uint32_t len);
#endif

#endif
