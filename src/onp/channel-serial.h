#ifndef CHANNEL_SERIAL_H
#define CHANNEL_SERIAL_H

void channel_serial_init();
int  channel_serial_recv(char* b, int l);
int  channel_serial_send(char* b, int n);

#endif
