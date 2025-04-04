
// --------------------------------------------------------------------

#include <onex-kernel/random.h>
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onex-kernel/ipv6.h>

#include <tests.h>

int main(void) {

  log_init();
  time_init();
  random_init();
  ipv6_init(0);

  uint16_t count=random_ish_byte();
  ipv6_printf("UID: uid-x  count: %d", count++);
  char inbuf[256];
  while (1) {
    time_delay_ms(400+random_ish_byte());
    uint16_t n=ipv6_recv(inbuf);
    if(n){
      inbuf[n]=0;
      printf("received: %s\n", inbuf);
      ipv6_printf("UID: uid-x  count: %d", count++);
    }
  }

  time_end();
}

// --------------------------------------------------------------------

