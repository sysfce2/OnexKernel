
#include <time.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <mathlib.h>

#include <onex-kernel/log.h>
#include <onex-kernel/time.h>

#include <items.h>
#include <onn.h>

bool evaluate_device_logic(object* o, void* d) {
  if(object_property_contains(o, "Alerted:is", "device")){
    char* devuid=object_property(o, "Alerted");
    // REVISIT: no notification of local to remote if "already seen" it
    if(!object_property_contains(o, "peers", devuid)){
      object_property_add(o, "peers", devuid);
    }
  }
  return true;
}


