
#include <items.h>
#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>
#include <persistence.h>

properties* persistence_objects_text=0;


void persistence_init(char* db) {

#if defined(BOARD_MAGIC3)
#endif

  persistence_objects_text=properties_new(MAX_OBJECTS);
}

void persistence_put(char* uid, char* text) {

#if defined(BOARD_MAGIC3)
#else
  log_write("=> %s\n", text);
#endif
}





