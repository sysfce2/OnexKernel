
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
}

void persistence_put(char* uid, char* text) {

  if(!persistence_objects_text) return;

#if defined(BOARD_MAGIC3)
#else
  log_write("=> %s\n", text);
#endif
}





