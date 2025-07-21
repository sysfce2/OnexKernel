
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <boards.h>

#include <onex-kernel/spi-flash.h>

#define SPI_NOR_CMD_RSTEN    0x66    /* reset enable */
#define SPI_NOR_CMD_RST      0x99    /* reset */
#define SPI_NOR_CMD_RDID     0x9F    /* read JEDEC ID */
#define SPI_NOR_CMD_REMS     0x90    /* read manuf IDs */
#define SPI_NOR_CMD_WRSR     0x01    /* write status register */
#define SPI_NOR_CMD_RDSR_LO  0x05    /* read status register low byte */
#define SPI_NOR_CMD_RDSR_HI  0x35    /* read status register high byte */
#define SPI_NOR_CMD_DPD      0xB9    /* deep power down */
#define SPI_NOR_CMD_RDPD     0xAB    /* release from deep power down */

static volatile bool busy=false;

static void (*spi_flash_done_cb)();

static void qspi_handler(nrfx_qspi_evt_t event, void * p_context) {
  if(event==NRFX_QSPI_EVENT_DONE && spi_flash_done_cb){
    spi_flash_done_cb();
    spi_flash_done_cb=0;
  }
  busy=false;
}

bool spi_flash_busy(){
  return busy;
}

static bool initialised=false;

char* spi_flash_init(char* allids) {

  *allids=0;

  if(initialised) return 0;

  uint32_t err_code;

  nrfx_qspi_config_t config = NRFX_QSPI_DEFAULT_CONFIG;
  if(nrfx_qspi_init(&config, qspi_handler, 0)) return "init!nrfx";

  initialised=true;

  nrf_qspi_cinstr_conf_t cinstr_cfg = {
      .io2_level = true,
      .io3_level = true,
      .wipwait   = true,
      .wren      = true
  };

  cinstr_cfg.opcode = SPI_NOR_CMD_RSTEN;
  cinstr_cfg.length = 1;
  if(nrfx_qspi_cinstr_xfer(&cinstr_cfg, 0, 0)) return "reset enable!cmd";

  cinstr_cfg.opcode = SPI_NOR_CMD_RST;
  cinstr_cfg.length = 1;
  if(nrfx_qspi_cinstr_xfer(&cinstr_cfg, 0, 0)) return "reset!cmd";

  cinstr_cfg.opcode = SPI_NOR_CMD_RDID;
  cinstr_cfg.length = 1+3;
  uint8_t idnums[3] = {0,0,0};
  if(nrfx_qspi_cinstr_xfer(&cinstr_cfg, 0, idnums)) return "manuf ids!cmd";

  cinstr_cfg.opcode = SPI_NOR_CMD_REMS;
  cinstr_cfg.length = 1+5;
  uint8_t idnums2[5] = {0,0,0,0,0}; // 0,1,2=tx 3,4=rx??
  if(nrfx_qspi_cinstr_xfer(&cinstr_cfg, idnums2, idnums2)) return "manuf ids 2!cmd";

  snprintf(allids, 64, "(%02x,%02x,%02x)(%02x,%02x)",
                       idnums[0], idnums[1], idnums[2],
                       idnums2[3], idnums2[4]);

  // Quad Enable!
  // Nordic DK: bit 6 of one-byte status register = 0x40
  // Magic 3:   bit 9 of two-byte status register = 0x0200
  // Rock:      none? of one-byte status register .. seems 0x00 OK / 0x02 dropped
  // we have mfr ids so can switch on them to give one byte/0x40 if needed
  cinstr_cfg.opcode = SPI_NOR_CMD_WRSR;
  cinstr_cfg.length = 1+2;
  uint8_t statusw[2] = {0x00,0x02}; // lo then hi
  if(nrfx_qspi_cinstr_xfer(&cinstr_cfg, statusw, 0)) return "quad enable!cmd";

#ifdef READ_STATUS_REGISTER
  cinstr_cfg.opcode = SPI_NOR_CMD_RDSR_LO;
  cinstr_cfg.length = 1+1;
  uint8_t statuslo;
  if(nrfx_qspi_cinstr_xfer(&cinstr_cfg, 0, &statuslo)) return "read lo status!cmd";

  cinstr_cfg.opcode = SPI_NOR_CMD_RDSR_HI;
  cinstr_cfg.length = 1+1;
  uint8_t statushi;
  if(nrfx_qspi_cinstr_xfer(&cinstr_cfg, 0, &statushi)) return "read hi status!cmd";

  snprintf(allids, 64, "sr: %02x %02x", statuslo, statushi);
#endif

  return 0;
}

char* spi_flash_erase(uint32_t            address,
                      spi_flash_erase_len len,
                      void (*cb)()){

  if(busy) return "busy!erase";
  busy=true;
  spi_flash_done_cb=cb;
  if(nrfx_qspi_erase(len, address)) return "erase!nrfx";
  if(!spi_flash_done_cb) while(busy);
  return 0;
}

char* spi_flash_write(uint32_t address,
                      uint8_t* data,
                      uint32_t len,
                      void (*cb)()){

  if(busy) return "busy!write";
  busy=true;
  spi_flash_done_cb=cb;
  if(nrfx_qspi_write(data, len, address)) return "write!nrfx";
  if(!spi_flash_done_cb) while(busy);
  return 0;
}

char* spi_flash_read(uint32_t address,
                     uint8_t* buf,
                     uint32_t len,
                     void (*cb)()){

  if(busy) return "busy!read";
  busy=true;
  spi_flash_done_cb=cb;
  if(nrfx_qspi_read(buf, len, address)) return "read!nrfx";
  if(!spi_flash_done_cb) while(busy);
  return 0;
}



