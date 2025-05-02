#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <onex-kernel/log.h>
#include <onex-kernel/ipv6.h>

#define SYS_CLASS_NET "/sys/class/net/"
#define PORT 4321
#define INTERFACE "wifi"

static bool initialised=false;

bool is_interface_up(const char *ifname) {

  char path[256];
  snprintf(path, sizeof(path), SYS_CLASS_NET "%s/operstate", ifname);

  FILE *file = fopen(path, "r");
  if (!file) return false;

  char state[16];
  bool result = (fgets(state, sizeof(state), file) && strncmp(state, "up", 2) == 0);
  fclose(file);
  return result;
}

bool is_wireless(const char *ifname) {
  char path[256];
  snprintf(path, sizeof(path), SYS_CLASS_NET "%s/wireless", ifname);
  struct stat st;
  return (stat(path, &st) == 0);
}

bool is_usb(const char *ifname) {
  char path[256];
  snprintf(path, sizeof(path), SYS_CLASS_NET "%s/device/modalias", ifname);

  FILE *file = fopen(path, "r");
  if (!file) return false;

  char modalias[256];
  bool result = (fgets(modalias, sizeof(modalias), file) && strstr(modalias, "usb:") != NULL);
  fclose(file);
  return result;
}

char* get_interface_name(char* interface) {

  DIR *dir = opendir(SYS_CLASS_NET);
  if (!dir) {
    perror("opendir");
    return 0;
  }
  char* name=0;
  struct dirent *entry;
  while ((entry = readdir(dir))) {

    if (entry->d_name[0] == '.') continue;
    if (!strncmp(entry->d_name, "lo",     2) ||
        !strncmp(entry->d_name, "docker", 6) ||
        !strncmp(entry->d_name, "veth",   4) ||
        !strncmp(entry->d_name, "br",     2)   ){

        continue;
    }
    if(!is_interface_up(entry->d_name)) continue;

    if(is_wireless(entry->d_name)) {
      if(strcmp(interface, "wifi")) continue;
      name = strdup(entry->d_name);
      log_write("Using Wi-Fi: %s\n", name);
      break;
    }
    if(is_usb(entry->d_name)) {
      if(strcmp(interface, "usb")) continue;
      name = strdup(entry->d_name);
      log_write("Using USB Network: %s\n", name);
      break;
    }
    {
      if(strcmp(interface, "eth")) continue;
      name = strdup(entry->d_name);
      log_write("Using Ethernet: %s\n", name);
      break;
    }
  }
  closedir(dir);
  return name;
}


static properties* group_to_sock_addr=0;

typedef struct {
  int                 sock;
  struct sockaddr_in6 mc_addr;
} sock_addr;

static bool ipv6_init_a_group(char* group){

  int sock;
  struct sockaddr_in6 addr;
  struct ipv6_mreq mc_group;
  struct sockaddr_in6 mc_addr;
  int reuse = 1, loopback = 0;

  if ((sock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
    perror("Socket creation failed");
    return false;
  }
  fcntl(sock, F_SETFL, O_NONBLOCK);

  if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    perror("setsockopt(SO_REUSEADDR)");
    return false;
  }
  int mc_all = 0;
  if(setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_ALL, &mc_all, sizeof(mc_all)) < 0) {
    perror("setsockopt(IPV6_MULTICAST_ALL)");
    return false;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(PORT);
  addr.sin6_addr = in6addr_any;

  if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("Bind failed");
    close(sock);
    return false;
  }

  memset(&mc_addr, 0, sizeof(mc_addr));
  mc_addr.sin6_family = AF_INET6;
  mc_addr.sin6_port = htons(PORT);
  inet_pton(AF_INET6, group, &mc_addr.sin6_addr);

  char* interface_name = get_interface_name(INTERFACE);
  if (!interface_name) {
    log_write("No interface for " INTERFACE);
    return false;
  }
  unsigned int if_index = if_nametoindex(interface_name);
  free(interface_name);
  if (if_index == 0) {
    perror("if_nametoindex()");
    return false;
  }

  mc_group.ipv6mr_interface = if_index;
  inet_pton(AF_INET6, group, &mc_group.ipv6mr_multiaddr);

  if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &if_index, sizeof(if_index)) < 0) {
    perror("Setting multicast interface failed");
    close(sock);
    return false;
  }

  if (setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mc_group, sizeof(mc_group)) < 0) {
    perror("Multicast group join failed");
    close(sock);
    return false;
  }

  sock_addr* gi = malloc(sizeof(sock_addr));
  gi->sock=sock;
  gi->mc_addr=mc_addr;
  properties_set(group_to_sock_addr, group, gi);

  log_write("Listening and broadcasting on multicast group: %s port %d...\n", group, PORT);

  return true;
}

static ipv6_recv_cb recv_cb;

bool ipv6_init(list* groups, ipv6_recv_cb cb){
  if(initialised) return true;
  initialised = true;
  recv_cb = cb;
  group_to_sock_addr=properties_new(MAX_GROUPS);
  for(int i=1; i<=list_size(groups); i++){
    char* group = value_string(list_get_n(groups, i));
    initialised=initialised && ipv6_init_a_group(group);
    char channel[256]; snprintf(channel, 256, "ipv6-%s", group);
    if(recv_cb) recv_cb(true, channel);
  }
  return initialised;
}

uint16_t ipv6_read(char* group, char* buf, uint16_t size){
  struct sockaddr_in6 addr;
  socklen_t addrLen = sizeof(addr);
  sock_addr* gi = (sock_addr*)properties_get(group_to_sock_addr, group);
  ssize_t n = recvfrom(gi->sock, buf, size, 0, (struct sockaddr*)&addr, &addrLen);
  return (n>0)? n: 0;
}

static uint16_t ipv6_write_gi(sock_addr* gi, char* buf, uint16_t size);

uint16_t ipv6_write(char* group, char* buf, uint16_t size){
  if(!buf || size > 2048) return 0;

  sock_addr* gi = (sock_addr*)properties_get(group_to_sock_addr, group);
  if(gi) return ipv6_write_gi(gi, buf, size);

  // given group not found, so just do the lot...
  bool ok = false;
  for(int i=1; i<=properties_size(group_to_sock_addr); i++){
    char* gp = properties_key_n(group_to_sock_addr,i);
    sock_addr* gi = (sock_addr*)properties_get(group_to_sock_addr, gp);
    ok = !!ipv6_write_gi(gi, buf, size) || ok;
  }
  return ok? size: 0;
}

static uint16_t ipv6_write_gi(sock_addr* gi, char* buf, uint16_t size){
  const struct sockaddr_in6 mc_addr=gi->mc_addr;
  if(sendto(gi->sock, buf, size, 0, (const struct sockaddr*)&mc_addr, sizeof(mc_addr)) >= 0) {
    return size;
  }
  perror("sendto failed");
  return 0;
}

//  close(sock);

