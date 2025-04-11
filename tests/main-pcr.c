
#include <onex-kernel/log.h>
#include <onex-kernel/time.h>
#include <onn.h>

int main() {

  log_init();
  time_init();

  onex_init_ipv6("pcr.db", list_new_from("ff12::1234 ff12::4321", 2));

  log_write("\n------Starting PCR Test Server-----\n");

  while(true){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }
  time_end();
}

// --------------------------------------------------------------------


