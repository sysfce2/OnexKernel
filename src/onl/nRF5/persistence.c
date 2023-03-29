
#include <items.h>
#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>
#include <persistence.h>

properties* persistence_objects_text=0;

void persistence_init(char* f) {

}

void persistence_put(char* uid, char* text) {

  log_write("%0.35s", strstr(text, "is: "));
}





