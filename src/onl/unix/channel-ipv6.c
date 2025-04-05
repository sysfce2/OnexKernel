
#include <onex-kernel/ipv6.h>
#include <onex-kernel/log.h>

#include <channel-ipv6.h>

static bool initialised=false;

static channel_ipv6_connect_cb connect_cb;

void channel_ipv6_on_recv(unsigned char* ch, size_t len) {
  if(!ch){
    if(connect_cb) connect_cb("ipv6");
    return;
  }
}

void channel_ipv6_init(channel_ipv6_connect_cb cb) {
  connect_cb=cb;
  initialised=ipv6_init(channel_ipv6_on_recv);
}

uint16_t channel_ipv6_recv(char* b, uint16_t l) {
  if(!initialised) return 0;
  return ipv6_recv(b,l);
}

uint16_t channel_ipv6_send(char* b, uint16_t n) {
  if(!initialised) return 0;
  return ipv6_printf("%s\n", b);
}


