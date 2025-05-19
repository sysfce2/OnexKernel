
#include <onex-kernel/config.h>
#include <onex-kernel/log.h>
#if defined(NRF5)
#include <onex-kernel/serial.h>
#endif
#include <onex-kernel/time.h>
#include <onex-kernel/random.h>
#include <onn.h>

int main(int argc, char *argv[]) {

#if defined(NRF5)
  properties* config = properties_new(32);
  properties_set(config, "channels", list_new_from_fixed("radio serial"));
  properties_set(config, "flags", list_new_from_fixed("debug-on-serial log-to-led"));
  properties_set(config, "test-uid-prefix", value_new("pcr"));
#else
  properties* config = get_config(argc, argv, "pcr", "log-onp");
  if(!config) return -1;
#endif

  time_init();
  log_init(config);
  random_init();

  onex_init(config);

  log_write("\n------Starting PCR Test Server-----\n");

  while(true){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }
  time_end();
}

// --------------------------------------------------------------------


