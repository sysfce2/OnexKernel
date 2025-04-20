
#include <onex-kernel/log.h>
#include <onex-kernel/time.h>
#include <onex-kernel/random.h>
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

int main(int argc, char *argv[]){

  if(argc<=1){
    log_write("Usage: %s <mcast group: ff12::1234 / ff12::4321>\n", argv[0]);
    return -1;
  }
  properties* config = properties_new(32);
  properties_set(config, "dbpath", value_new("light.ondb"));
  properties_set(config, "channels", list_new_from("ipv6", 1));
  properties_set(config, "ipv6_groups", list_new_from(argv[1], 1));
  properties_set(config, "test-uid-prefix", value_new("light"));

  time_init();
  log_init(config);
  random_init();

  onex_init(config);

  log_write("\n------Starting Light Test Server-----\n");

  onex_set_evaluators("evaluate_light",  evaluate_light, 0);

  object* light=object_new("uid-l", "evaluate_light",  "light", 4);

  object_property_set(light,  "light", "off");
  object_property_set(light,  "button", "uid-b");

  onex_run_evaluators("uid-l", 0);

  #define LOOPS 50
  while((number_up < LOOPS || number_down < LOOPS) && keep_going){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }

  onex_assert_summary();

  time_end();
}

// --------------------------------------------------------------------

