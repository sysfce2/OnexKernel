
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "nrf52.h"
#include "nrf52_bitfields.h"

#include <onex-kernel/radio.h>

#define RADIO_TXPOWER                  RADIO_TXPOWER_TXPOWER_0dBm
#define RADIO_CHANNEL                  7 // 2.407GHz

#define RADIO_BASE_ADDRESS             0x75626974
#define RADIO_DEFAULT_GROUP            0

#define RADIO_MAX_PACKET_SIZE          252

static          uint8_t rx_buffer[256];
static volatile uint8_t rx_length = 0;

static volatile bool initialised=false;

static radio_recv_cb recv_cb = 0;

bool radio_init(radio_recv_cb cb){

  recv_cb = cb;

  if(initialised) return true;

  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  while(!NRF_CLOCK->EVENTS_HFCLKSTARTED);

  NRF_RADIO->TXPOWER   = RADIO_TXPOWER;
  NRF_RADIO->FREQUENCY = RADIO_CHANNEL;

  NRF_RADIO->MODE  = RADIO_MODE_MODE_Nrf_1Mbit;  // 2Mbit?!
  NRF_RADIO->BASE0 = RADIO_BASE_ADDRESS;

  NRF_RADIO->PREFIX0 = RADIO_DEFAULT_GROUP;

  NRF_RADIO->TXADDRESS = 0;
  NRF_RADIO->RXADDRESSES = 1;

  NRF_RADIO->PCNF0 = (8 << RADIO_PCNF0_LFLEN_Pos) |
                     (0 << RADIO_PCNF0_S1LEN_Pos);

  NRF_RADIO->PCNF1 = (RADIO_MAX_PACKET_SIZE       << RADIO_PCNF1_MAXLEN_Pos) |
                     (0                           << RADIO_PCNF1_STATLEN_Pos) |
                     (4                           << RADIO_PCNF1_BALEN_Pos) |
                     (RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos);

  NRF_RADIO->CRCCNF = RADIO_CRCCNF_LEN_Two;
  NRF_RADIO->CRCINIT = 0xFFFF;
  NRF_RADIO->CRCPOLY = 0x11021;

  NRF_RADIO->DATAWHITEIV = 0x18;

  NRF_RADIO->PACKETPTR = (uint32_t)rx_buffer;

  NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk;
  NVIC_ClearPendingIRQ(RADIO_IRQn);
  NVIC_EnableIRQ(RADIO_IRQn); // RADIO_IRQHandler()

  NRF_RADIO->SHORTS |= RADIO_SHORTS_ADDRESS_RSSISTART_Msk;

  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->TASKS_RXEN = 1;
  while(!NRF_RADIO->EVENTS_READY);
  NRF_RADIO->EVENTS_END = 0;
  NRF_RADIO->TASKS_START = 1;

  initialised=true;

  return true;
}

bool radio_write(unsigned char* buf, uint8_t len) {

  if(!buf || len > RADIO_MAX_PACKET_SIZE) return false;

  NRF_RADIO->PCNF1 = (len                         << RADIO_PCNF1_MAXLEN_Pos) |
                     (0                           << RADIO_PCNF1_STATLEN_Pos) |
                     (4                           << RADIO_PCNF1_BALEN_Pos) |
                     (RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos);

  NVIC_DisableIRQ(RADIO_IRQn);

  NRF_RADIO->EVENTS_DISABLED = 0;
  NRF_RADIO->TASKS_DISABLE = 1;
  while(!NRF_RADIO->EVENTS_DISABLED);

  NRF_RADIO->PACKETPTR = (uint32_t)buf;

  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->TASKS_TXEN = 1;
  while(!NRF_RADIO->EVENTS_READY);

  NRF_RADIO->TASKS_START = 1;
  NRF_RADIO->EVENTS_END = 0;
  while(!NRF_RADIO->EVENTS_END);

  NRF_RADIO->PACKETPTR = (uint32_t)rx_buffer;

  NRF_RADIO->EVENTS_DISABLED = 0;
  NRF_RADIO->TASKS_DISABLE = 1;
  while(!NRF_RADIO->EVENTS_DISABLED);

  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->TASKS_RXEN = 1;
  while(!NRF_RADIO->EVENTS_READY);

  NRF_RADIO->EVENTS_END = 0;
  NRF_RADIO->TASKS_START = 1;

  NVIC_ClearPendingIRQ(RADIO_IRQn);
  NVIC_EnableIRQ(RADIO_IRQn);

  return true;
}

uint16_t radio_recv(unsigned char* buf, size_t maxlen) {
    uint8_t len = rx_length;
    if (len > maxlen) len = maxlen;
    if (len > 0) {
        memcpy(buf, rx_buffer, len);
        rx_length = 0;
    }
    return len;
}

void RADIO_IRQHandler(void){

  if(NRF_RADIO->EVENTS_READY) {
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->TASKS_START = 1;
  }
  if(NRF_RADIO->EVENTS_END) {
    NRF_RADIO->EVENTS_END = 0;

    if(NRF_RADIO->CRCSTATUS == 1) {
      int8_t rssi = -NRF_RADIO->RSSISAMPLE;
      rx_length   =  NRF_RADIO->PCNF1 & 0xff;
      if(recv_cb) recv_cb(rx_length, rssi);
    }
    NRF_RADIO->TASKS_START = 1;
  }
}


