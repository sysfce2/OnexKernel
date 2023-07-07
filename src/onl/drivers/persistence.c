
#include <items.h>
#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>
#include <onex-kernel/spi-flash.h>
#include <persistence.h>

properties* persistence_objects_text=0;

void persistence_init(char* db) {

#if defined(BOARD_MAGIC3)
  char* err;

  char allids[64];
  err=spi_flash_init(allids);
  if(err){ log_write("db %s", err); return; }
#endif

  persistence_objects_text=properties_new(MAX_OBJECTS);

  // : read it all in to persistence_objects_text
}

void persistence_put(char* uid, char* text) {

  if(!persistence_objects_text) return;

#if defined(BOARD_MAGIC3)

  char* err;

#define FLASH_START 0x3000

  err = spi_flash_erase(FLASH_START, SPI_FLASH_ERASE_LEN_4KB, 0);
  if(err){ log_write("db %s", err); return; }

  uint16_t len32=(strlen(text)/4)*4+4;

  uint8_t wrbuf[len32];
  mem_strncpy((char*)wrbuf, text, len32);
  // pads wrbuff with zeroes up to len32 if text short

  err = spi_flash_write(FLASH_START, wrbuf, len32, 0);
  if(err){ log_write("db %s", err); return; }
/*
  uint8_t verif[len32];

  err = spi_flash_read(FLASH_START, verif, len32, 0);
  if(err){ log_write("db %s", err); return; }

  bool ok=!memcmp(wrbuf, verif, len32);

  int16_t sn=strlen((char*)verif)-67;
  if(sn<0) sn=0;
  char* veris=(char*)verif+sn;
  char res[len32];
  snprintf(res, len32, ok? "=%s=": "#%s#", veris);
  log_write(res);
*/
#else
  log_write("=> %s\n", text);
#endif
}





