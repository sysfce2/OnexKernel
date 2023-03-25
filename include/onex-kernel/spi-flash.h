#ifndef SPI_FLASH_H
#define SPI_FLASH_H

#if defined(NRF52840_XXAA)

#include <nrfx_qspi.h>

#define SPI_FLASH_ERASE_LEN_4KB   NRF_QSPI_ERASE_LEN_4KB
#define SPI_FLASH_ERASE_LEN_64KB  NRF_QSPI_ERASE_LEN_64KB
#define SPI_FLASH_ERASE_LEN_ALL   NRF_QSPI_ERASE_LEN_ALL
typedef nrf_qspi_erase_len_t spi_flash_erase_len;

/** init SPI flash and fill supplied buffer with chip ids.
  * returns 0 or error string */
char* spi_flash_init(char* allids);

/** erase one of the above chunk lengths: 4K, 64K or all.
  * get a cb() when done, or set to 0 to block
  * returns 0 or error string */
char* spi_flash_erase(uint32_t            address,
                      spi_flash_erase_len len,
                      void (*cb)());

/** write buffer of length to address.
  * get a cb() when done, or set to 0 to block
  * returns 0 or error string */
char* spi_flash_write(uint32_t address,
                      uint8_t* data,
                      uint32_t len,
                      void (*cb)());

/** read buffer of length from address.
  * get a cb() when done, or set to 0 to block
  * returns 0 or error string */
char* spi_flash_read(uint32_t address,
                     uint8_t* buf,
                     uint32_t len,
                     void (*cb)());

/** is spi flash still doing erase/read or write
  * and we're waiting for a callback */
bool spi_flash_busy();

#endif

#endif
