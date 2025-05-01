
#include <onex-kernel/log.h>
#include <onex-kernel/ipv6.h>
#include <channel-ipv6.h>

static bool initialised=false;

void channel_ipv6_init(list* groups, connect_cb ipv6_connect_cb) {

  initialised=ipv6_init(groups);
  if(!initialised) return;

  // REVISIT: would like to thread cb and make one loop not two
  for(int i=1; i<=list_size(groups); i++){
    char* group = value_string(list_get_n(groups, i));
    char channel[256]; snprintf(channel, 256, "ipv6-%s", group);
    if(ipv6_connect_cb) ipv6_connect_cb(channel);
  }
}

uint16_t channel_ipv6_recv(char* group, char* buf, uint16_t size) {
  if(!initialised) return 0;
  return ipv6_read(group, buf, size);
}

uint16_t channel_ipv6_send(char* group, char* buf, uint16_t size) {
  if(!initialised) return 0;
  return ipv6_write(group, buf, size);
}


