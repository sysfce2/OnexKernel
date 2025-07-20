
#include <items.h>
#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>
#include <onex-kernel/spi-flash.h>
#include <persistence.h>

static properties* persistence_objects_text=0;

static volatile list* keep_actives = 0;

list* persistence_init(char* db) {

#if defined(BOARD_MAGIC3) && defined(NRF_DO_FLASH)
  char* err;

  char allids[64];
  err=spi_flash_init(allids);
  if(err){ log_write("db %s", err); return; }
#endif

  persistence_objects_text=properties_new(MAX_OBJECTS);
  keep_actives = list_new(64);

  return 0;
}

list* persistence_reload(){
  return keep_actives;
}

char* persistence_get(char* uid){

  if(!persistence_objects_text) return 0;

  return properties_get(persistence_objects_text, uid);
}

void persistence_put(char* uid, uint32_t ver, char* text) {

  if(!persistence_objects_text) return;

  mem_freestr(properties_delete(persistence_objects_text, uid));
  properties_set(persistence_objects_text, uid, mem_strdup(text));

  bool ka = strstr(text, "Cache: keep-active");
  if(ka) list_vals_set_add(keep_actives, uid);

#if defined(BOARD_MAGIC3) && defined(NRF_DO_FLASH)

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
#endif
}


