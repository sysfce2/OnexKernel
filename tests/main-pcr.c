
#include <onex-kernel/log.h>
#include <onex-kernel/time.h>
#include <onex-kernel/random.h>
#include <onn.h>

int main(int argc, char *argv[]) {

  properties* config = properties_new(32);
  properties_set(config, "dbpath", value_new("pcr.ondb"));
  properties_set(config, "channels", list_new_from("ipv6", 2));
  properties_set(config, "ipv6_groups", list_new_from("ff12::1234 ff12::4321", 2));
  properties_set(config, "test-uid-prefix", value_new("pcr"));

  time_init();
  log_init(config);
  random_init();


  log_write("\n------Starting PCR Test Server-----\n");

  onex_init(config);

  while(true){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }
  time_end();
}

// --------------------------------------------------------------------


