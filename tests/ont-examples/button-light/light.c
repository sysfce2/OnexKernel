
#if defined(NRF5)
#include <boards.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/serial.h>
#include <onex-kernel/blenus.h>
#endif

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onf.h>

object* light;

bool evaluate_light(object* light, void* d);

// Copied from ONR Behaviours
bool evaluate_device_logic(object* o, void* d)
{
  if(object_property_contains(o, (char*)"Alerted:is", (char*)"device")){
    char* devuid=object_property(o, (char*)"Alerted");
    if(!object_property_contains(o, (char*)"connected-devices", devuid)){
      object_property_add(o, (char*)"connected-devices", devuid);
    }
  }
  return true;
}

int main()
{
  log_init();
  time_init();
#if defined(NRF5)
  serial_init(0,0);
  blenus_init(0);
#endif
  onex_init("");

#if defined(NRF5)
  gpio_mode(LED1_G, OUTPUT);
  gpio_mode(LED2_B, OUTPUT);
#else
  time_delay_s(2);
  log_write("\n------Starting Light Test-----\n");
#endif

  onex_set_evaluators("evaluate_device", evaluate_device_logic, 0);
  onex_set_evaluators("evaluate_light", evaluate_light, 0);

  object_set_evaluator(onex_device_object, (char*)"evaluate_device");

  light=object_new(0, "evaluate_light", "editable light", 4);

  char* lightuid=object_property(light, "UID");
  object_property_add(onex_device_object, (char*)"io", lightuid);

  char* deviceuid=object_property(onex_device_object, "UID");
  object_property_set(light, "device", deviceuid);
  object_property_set(light, "light", "off");

#if defined(NRF5)
  gpio_set(LED1_G, 0);
  gpio_set(LED2_B, 1);
#endif
  uint16_t todo=0;
  while(1){

    onex_loop();

    if(todo<2 && time_ms() >1000u+2000u*todo){  todo++;
      onex_run_evaluators(lightuid, 0);
    }
  }
}

bool evaluate_light(object* light, void* d)
{
  char* buttonuid=object_property(light, "device:connected-devices:io");
  if(buttonuid) object_property_set(light, "button", buttonuid);

  bool buttonpressed=object_property_is(light, "button:state", "down");
  char* s=(char*)(buttonpressed? "on": "off");
  object_property_set(light, "light", s);
#if defined(NRF5)
  if(buttonpressed){
    gpio_set(LED2_B, 0);
  } else {
    gpio_set(LED2_B, 1);
  }
#else
  log_write("evaluate_light: "); object_log(light);
#endif
  return true;
}

