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

#define SYS_CLASS_NET "/sys/class/net/"

#include <onex-kernel/ipv6.h>

#define PORT 4321
#define MULTICAST_GROUP "ff12::1234"

int sock;
struct sockaddr_in6 multicastAddr;

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

char* get_interface_name() {

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
      name = strdup(entry->d_name);
      printf("Using Wi-Fi: %s\n", name);
      break;
    }
    if(is_usb(entry->d_name)) {
      name = strdup(entry->d_name);
      printf("Using USB Network: %s\n", name);
      break;
    }
    {
      name = strdup(entry->d_name);
      printf("Using Ethernet: %s\n", name);
      break;
    }
  }
  closedir(dir);
  return name;
}

bool ipv6_init(ipv6_recv_cb cb){

  if(initialised) return true;

  struct sockaddr_in6 addr;
  struct ipv6_mreq group;
  int reuse = 1, loopback = 0;

  if ((sock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
    perror("Socket creation failed");
    return false;
  }

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(PORT);
  addr.sin6_addr = in6addr_any;

  if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("Bind failed");
    close(sock);
    return false;
  }

  memset(&multicastAddr, 0, sizeof(multicastAddr));
  multicastAddr.sin6_family = AF_INET6;
  multicastAddr.sin6_port = htons(PORT);
  inet_pton(AF_INET6, MULTICAST_GROUP, &multicastAddr.sin6_addr);

  char* interface_name = get_interface_name();
  unsigned int if_index = if_nametoindex(interface_name);
  free(interface_name);
  if (if_index == 0) {
    perror("Invalid interface");
    return false;
  }

  group.ipv6mr_interface = if_index;
  inet_pton(AF_INET6, MULTICAST_GROUP, &group.ipv6mr_multiaddr);

  if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &if_index, sizeof(if_index)) < 0) {
    perror("Setting multicast interface failed");
    close(sock);
    return false;
  }

  if (setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &group, sizeof(group)) < 0) {
    perror("Multicast group join failed");
    close(sock);
    return false;
  }
  printf("Joined multicast group: %s\n", MULTICAST_GROUP);
/*
  if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &loopback, sizeof(loopback)) < 0) {
    perror("Disabling multicast loopback failed");
    close(sock);
    return false;
  }
*/
  printf("Listening and broadcasting on port %d...\n", PORT);

  initialised=true;

  return true;
}

uint16_t ipv6_recv(char* buf){
  struct sockaddr_in6 addr;
  socklen_t addrLen = sizeof(addr);
  return recvfrom(sock, buf, 256, 0, (struct sockaddr*)&addr, &addrLen);
}

size_t ipv6_printf(const char* fmt, ...){
  if(!initialised) ipv6_init(0);
  va_list args;
  va_start(args, fmt);
  size_t r=ipv6_vprintf(fmt,args);
  va_end(args);
  return r;
}

#define PRINT_BUF_SIZE 2048
static char print_buf[PRINT_BUF_SIZE];

size_t ipv6_vprintf(const char* fmt, va_list args){
  size_t r=vsnprintf((char*)print_buf, PRINT_BUF_SIZE, fmt, args);
  if(r>=PRINT_BUF_SIZE) r=PRINT_BUF_SIZE-1;
  return ipv6_write(print_buf, r)? r: 0;
}

bool ipv6_write(char* buf, uint16_t len){
  if(!buf || len > 2048) return false;

  if(sendto(sock, buf, len, 0, &multicastAddr, sizeof(multicastAddr)) >= 0) {
    return true;
  }
  perror("sendto failed");
  return false;
}

//  close(sock);

