#ifndef RADIO_H
#define RADIO_H

bool   radio_init(uint8_t ss_pin, uint8_t interrupt_pin, bool swspi);
bool   radio_set_frequency(float freq);
void   radio_set_encryption_key(uint8_t* key);
void   radio_send(uint8_t* sendmsg, size_t len);
bool   radio_wait_packet_sent(uint16_t timeout);
bool   radio_available();
bool   radio_recv(uint8_t* recvmsg, uint8_t* len);
int8_t radio_last_rssi();

#endif
