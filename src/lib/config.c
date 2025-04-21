
#include <string.h>
#include <onex-kernel/config.h>
#include <onex-kernel/log.h>

properties* get_config(int argc, char *argv[], char* name){

  if(argc==1){
    log_write("Usage: %s <serial ttys: /dev/ttyACM0 /dev/ttyACM1\n", argv[0]);
    log_write("Usage: %s <mcast groups: ff12::1234 ff12::4321>\n", argv[0]);
    return 0;
  }

  properties* config = properties_new(32);

  char dbname[128]; snprintf(dbname, 128, "%s.ondb", name);
  properties_set(config, "dbpath", value_new(dbname));

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

  properties_set(config, "test-uid-prefix", value_new(name));

  return config;
}


