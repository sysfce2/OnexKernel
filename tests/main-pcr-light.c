
#include <onex-kernel/lib.h>
#include <onex-kernel/random.h>
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onex-kernel/ipv6.h>
#include <onn.h>
#include <tests.h>

int number_up = 0;
int number_down = 0;
bool keep_going = true;

bool evaluate_light(object* light, void* d) {
  if(object_property_is(light, "button:state", "up"  )) number_up++;
  if(object_property_is(light, "button:state", "down")) number_down++;
  log_write("evaluate_light %d %d: ", number_up, number_down); object_log(light);
  return true;
}

int main(void) {

  log_init();
  time_init();
  random_init();

  log_write("-----------------OnexKernel PCR tests------------------------\n");

// [ UID: uid-l  is: light  light: off  button: uid-b ]
//                 | IPv6 |
//                 V      V
// [ UID: uid-b  is: button  state: up ]

  onex_init_ipv6("light.db", "ff12::1234");

  onex_set_evaluators("evaluate_light",  evaluate_light, 0);

  object* light=object_new("uid-l", "evaluate_light",  "light", 4);

  object_property_set(light,  "light", "off");
  object_property_set(light,  "button", "uid-b");

  onex_run_evaluators("uid-l", 0);

  while((number_up < 4 || number_down < 4) && keep_going){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }

  onex_assert_summary();

  time_end();
}

// --------------------------------------------------------------------

