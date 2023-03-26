
#include <items.h>
#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>
#include <persistence.h>

properties* objects_text=0;

static properties* objects_to_save=0;

void persistence_init(char* f) {

  objects_to_save=properties_new(MAX_OBJECTS);
}

static uint32_t lasttime=0;

#define FLUSH_RATE_MS 1000

bool persistence_loop() {

  if(!objects_to_save) return false;

  uint64_t curtime = time_ms();
  if(curtime > lasttime+FLUSH_RATE_MS){
    persistence_flush();
    lasttime = curtime;
  }
  return false;
}

char* persistence_get(char* uid) {
  return 0;
}

void persistence_put(object* o) {

  if(!objects_to_save) return;

  char* uid=object_property(o, "UID");
  properties_set(objects_to_save, uid, uid);
}

void persistence_flush() {

  if(!objects_to_save) return;

  uint16_t sz=properties_size(objects_to_save);
  if(!sz) return;
  for(int j=1; j<=sz; j++){
    char* uid=properties_get_n(objects_to_save, j);
    object* o=onex_get_from_cache(uid);
    char buff[MAX_TEXT_LEN];
    char* text=object_to_text(o,buff,MAX_TEXT_LEN,OBJECT_TO_TEXT_PERSIST);
  }
  properties_clear(objects_to_save, false);
}





