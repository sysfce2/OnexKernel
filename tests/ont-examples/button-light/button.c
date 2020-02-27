
#if defined(NRF5)
#include <boards.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/serial.h>
#include <onex-kernel/blenus.h>
#endif
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onf.h>

object* button;
char* buttonuid;

void button_1_change_cb(int);
bool evaluate_button(object* button, void* pressed);

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
  gpio_mode_cb(BUTTON_1, INPUT_PULLUP, button_1_change_cb);
#endif

  log_init(9600);
  log_write("\n------Starting Button Test-----\n");

  onex_set_evaluators("evaluate_device", evaluate_device_logic, 0);
  onex_set_evaluators("evaluate_button", evaluate_button, 0);

  object_set_evaluator(onex_device_object, (char*)"evaluate_device");

  button=object_new(0, "evaluate_button", "editable button", 4);
  object_property_set(button, "name", "£€§");

  buttonuid=object_property(button, "UID");
  object_property_add(onex_device_object, (char*)"io", buttonuid);

#if !defined(NRF5)
  uint32_t lasttime=0;
  bool button_pressed=false;
#endif

  while(1){

    onex_loop();

    time_delay_ms(1);
#if !defined(NRF5)
    if(time_ms() > lasttime+1000u){
      lasttime=time_ms();
      button_pressed=!button_pressed;
      button_1_change_cb(button_pressed);
    }
#endif
  }
}

void button_1_change_cb(int button_pressed)
{
  onex_run_evaluators(buttonuid, (void*)(bool)button_pressed);
}

bool evaluate_button(object* button, void* pressed)
{
  char* s=(char*)(pressed? "down": "up");
  object_property_set(button, "state", s);
#if !defined(NRF5)
  log_write("evaluate_button: "); object_log(button);
#endif
  return true;
}


