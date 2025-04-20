
#include <string.h>

#include <onex-kernel/log.h>
#include <onex-kernel/time.h>
#include <onex-kernel/random.h>
#include <onn.h>

static bool button_pressed=false;

static void every_tick(void*){
  button_pressed=!button_pressed;
  onex_run_evaluators("uid-b", (void*)button_pressed);
}

bool evaluate_button(object* button, void* pressed) {
  char* s=(char*)(pressed? "down": "up");
  object_property_set(button, "state", s);
  log_write("evaluate_button: "); object_log(button);
  return true;
}

int main(int argc, char *argv[]) {

  properties* config = properties_new(32);
  properties_set(config, "dbpath", value_new("button.ondb"));

  if(argc==1){
    log_write("Usage: %s <serial ttys: /dev/ttyACM0 /dev/ttyACM1\n", argv[0]);
    log_write("Usage: %s <mcast groups: ff12::1234 ff12::4321>\n", argv[0]);
    return -1;
  }

  list* chans  = list_new(2);
  list* groups = 0;
  list* ttys   = 0;

  for(int a=1; a<argc; a++){
    char* arg = argv[a];
    if(strchr(arg, ':')){
      if(!groups) groups=list_new(4);
      list_add_value(groups, arg);
      list_add_setwise(chans, "ipv6");
    }
    else
    if(strchr(arg, '/')){
      if(!ttys) ttys=list_new(4);
      list_add_value(ttys, arg);
      list_add_setwise(chans, "serial");
    }
  }
  if(list_size(chans))  properties_set(config, "channels",    chans);
  if(list_size(groups)) properties_set(config, "ipv6_groups", groups);
  if(list_size(ttys  )) properties_set(config, "serial_ttys", ttys);

  properties_set(config, "test-uid-prefix", value_new("button"));

  time_init();
  log_init(config);
  random_init();

  onex_init(config);

  log_write("\n------Starting Button Test Server-----\n");

  onex_set_evaluators("evaluate_button", evaluate_button, 0);
  object* button=object_new("uid-b", "evaluate_button", "button", 4);
  object_property_set(button, "state", "up");
  onex_run_evaluators("uid-b", 0);

  time_ticker(every_tick, 0, 2000);

  while(1){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }
}

