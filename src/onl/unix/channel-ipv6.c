
#include <onex-kernel/log.h>
#include <onex-kernel/ipv6.h>
#include <channel-ipv6.h>

static bool initialised=false;

void channel_ipv6_init(list* groups, channel_ipv6_connect_cb connect_cb) {

  initialised=ipv6_init(groups);
  if(!initialised) return;

  for(int i=1; i<=list_size(groups); i++){
    char* group = value_string(list_get_n(groups, i));
    char channel[256]; snprintf(channel, 256, "ipv6-%s", group);
    if(connect_cb) connect_cb(channel);
  }
}

uint16_t channel_ipv6_recv(char* group, char* b, uint16_t l) {
  if(!initialised) return 0;
  return ipv6_recv(group, b,l);
}

uint16_t channel_ipv6_send(char* group, char* b, uint16_t n) {
  if(!initialised) return 0;
  return ipv6_printf(group, "%s\n", b);
}


