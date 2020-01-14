#ifndef CHANNEL_SERIAL_H
#define CHANNEL_SERIAL_H

typedef void (*channel_serial_connect_cb)(char*);

void channel_serial_init(channel_serial_connect_cb cb);
int  channel_serial_recv(char* b, int l);
int  channel_serial_send(char* b, int n);

#endif
