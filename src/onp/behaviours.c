
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
  if(!object_property_contains(o, "Alerted:is", "device")) return true;
  char* devuid=object_property(o, "Alerted");
  object_property_setwise_insert(o, "peers", devuid);
  return true;
}


