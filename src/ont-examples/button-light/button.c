
#if defined(TARGET_MCU_NRF51822)
#include <variant.h>
#include <onex-kernel/gpio.h>
#endif
#include <onex-kernel/log.h>
#include <onex-kernel/time.h>
#include <onf.h>

object* button;

bool evaluate_button(object* button, void* pressed);

int main()
{
  time_init();
  onex_init("");
#if defined(TARGET_MCU_NRF51822)
  gpio_mode(BUTTON_A, INPUT_PULLUP);
#endif

  time_delay_s(1);
  log_init(9600);
  log_write("\n------Starting Button Test-----\n");

  onex_set_evaluator("evaluate_button", evaluate_button);
  button=object_new("uid-1-2-3", "evaluate_button", "button", 4);
  object_property_set(button, "name", "£€§");

#if defined(TARGET_MCU_NRF51822)
#else
  int lasttime=0;
#endif
  bool button_pressed=false;

  while(1){

    onex_loop();

    time_delay_ms(1);
#if defined(TARGET_MCU_NRF51822)
    if(button_pressed != !gpio_get(BUTTON_A)){
      button_pressed = !gpio_get(BUTTON_A);
      onex_run_evaluator("uid-1-2-3", (void*)button_pressed, 0, 0);
    }
#else
    if(time_ms() > lasttime+1000){
       lasttime=time_ms();
       button_pressed=!button_pressed;
       onex_run_evaluator("uid-1-2-3", (void*)button_pressed, 0, 0);
    }
#endif
  }
}

bool evaluate_button(object* button, void* pressed)
{
  char* s=(char*)(pressed? "down": "up");
  object_property_set(button, "state", s);
#if defined(TARGET_MCU_NRF51822)
#else
  log_write("evaluate_button: "); object_log(button);
#endif
  return true;
}


