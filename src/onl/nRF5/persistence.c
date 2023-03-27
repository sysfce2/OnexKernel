
#include <items.h>
#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>
#include <persistence.h>

properties* persistence_objects_text=0;

void persistence_init(char* f) {

}

void persistence_put(char* uid, char* text) {

  // while we're keeping an in-mem db!
  mem_freestr(properties_delete(persistence_objects_text, uid));
  properties_set(persistence_objects_text, uid, mem_strdup(text));

  log_write("%0.35s", strstr(text, "is: "));
}





