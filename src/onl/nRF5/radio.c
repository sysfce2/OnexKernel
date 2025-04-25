
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <onex-kernel/log.h>
#include <onex-kernel/radio.h>

#define RADIO_TXPOWER                  RADIO_TXPOWER_TXPOWER_0dBm
#define RADIO_CHANNEL                  7 // 2.407GHz

#define RADIO_BASE_ADDRESS             0x75626974
#define RADIO_DEFAULT_GROUP            0

#define RADIO_MAX_PACKET_SIZE          252

static char rx_buffer[256];

static volatile bool initialised=false;

static radio_recv_cb recv_cb = 0;

bool radio_init(radio_recv_cb cb){

  recv_cb = cb;

  if(initialised) return true;

  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  while(!NRF_CLOCK->EVENTS_HFCLKSTARTED);

  NRF_RADIO->MODECNF0 = (RADIO_MODECNF0_DTX_B0 << RADIO_MODECNF0_DTX_Pos) |
                        (RADIO_MODECNF0_RU_Fast << RADIO_MODECNF0_RU_Pos);

  NRF_RADIO->TXPOWER   = RADIO_TXPOWER;
  NRF_RADIO->FREQUENCY = RADIO_CHANNEL;

  NRF_RADIO->MODE  = RADIO_MODE_MODE_Nrf_1Mbit;  // 2Mbit?!

  NRF_RADIO->BASE0 = RADIO_BASE_ADDRESS;      // REVISIT don't need address filter!
  NRF_RADIO->PREFIX0 = RADIO_DEFAULT_GROUP;
  NRF_RADIO->TXADDRESS = 0;
  NRF_RADIO->RXADDRESSES = 1;

  NRF_RADIO->PCNF0 = (8 << RADIO_PCNF0_LFLEN_Pos) |
                     (0 << RADIO_PCNF0_S0LEN_Pos) |
                     (0 << RADIO_PCNF0_S1LEN_Pos);

  NRF_RADIO->PCNF1 = (RADIO_MAX_PACKET_SIZE       << RADIO_PCNF1_MAXLEN_Pos) |
                     (0                           << RADIO_PCNF1_STATLEN_Pos) |
                     (4                           << RADIO_PCNF1_BALEN_Pos) |  // REVISIT no
                     (RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos);

  NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Two << RADIO_CRCCNF_LEN_Pos);
  NRF_RADIO->CRCINIT = 0xFFFF;
  NRF_RADIO->CRCPOLY = 0x11021;

  NRF_RADIO->DATAWHITEIV = 0x18;

  NRF_RADIO->PACKETPTR = (uint32_t)rx_buffer;

  NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk;
  NVIC_SetPriority(RADIO_IRQn, 0); // REVISIT
  NVIC_ClearPendingIRQ(RADIO_IRQn);
  NVIC_EnableIRQ(RADIO_IRQn); // RADIO_IRQHandler()

  NRF_RADIO->SHORTS |=
      //   (RADIO_SHORTS_READY_START_Enabled       << RADIO_SHORTS_READY_START_Pos      )|
      //   (RADIO_SHORTS_END_DISABLE_Enabled       << RADIO_SHORTS_END_DISABLE_Pos      )|
           (RADIO_SHORTS_ADDRESS_RSSISTART_Enabled << RADIO_SHORTS_ADDRESS_RSSISTART_Pos);

  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->TASKS_RXEN = 1;
  while(!NRF_RADIO->EVENTS_READY);

  NRF_RADIO->EVENTS_END = 0;
  NRF_RADIO->TASKS_START = 1;

  initialised=true;

  return true;
}

uint16_t radio_write(char* buf, uint16_t size) {

  if(!buf || len > RADIO_MAX_PACKET_SIZE){
    log_write("radio_write overloaded: %d '%s'\n", len, buf);
    return false;
  }

  NVIC_DisableIRQ(RADIO_IRQn);

  // turn off the radio
  NRF_RADIO->EVENTS_DISABLED = 0;
  NRF_RADIO->TASKS_DISABLE = 1;
  while(!NRF_RADIO->EVENTS_DISABLED);

  // hijack the rx_buffer for tx!
  rx_buffer[0]=size;               // bit shite, but first byte is the size
  memcpy(rx_buffer+1, buf, size);

  // turn on the transmitter, and wait for it ready to use
  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->TASKS_TXEN = 1;
  while(!NRF_RADIO->EVENTS_READY);

  // start transmission and wait for end of packet
  NRF_RADIO->EVENTS_END = 0;
  NRF_RADIO->TASKS_START = 1;
  while(!NRF_RADIO->EVENTS_END);

  // turn off the radio
  NRF_RADIO->EVENTS_DISABLED = 0;
  NRF_RADIO->TASKS_DISABLE = 1;
  while(!NRF_RADIO->EVENTS_DISABLED);

  // start listening for the next packet
  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->TASKS_RXEN = 1;
  while(!NRF_RADIO->EVENTS_READY);

  NRF_RADIO->EVENTS_END = 0;
  NRF_RADIO->TASKS_START = 1;

//NVIC_ClearPendingIRQ(RADIO_IRQn); // REVISIT
  NVIC_EnableIRQ(RADIO_IRQn);

  return true;
}

int16_t radio_printf(const char* fmt, ...){

  if(!initialised) radio_init(0);
  va_list args;
  va_start(args, fmt);
  int16_t r=radio_vprintf(fmt,args);
  va_end(args);
  return r;
}

#define PRINT_BUF_SIZE 255
static char print_buf[PRINT_BUF_SIZE];

int16_t radio_vprintf(const char* fmt, va_list args){

  int16_t r=vsnprintf(print_buf, PRINT_BUF_SIZE, fmt, args);
  if(r>=PRINT_BUF_SIZE) r=PRINT_BUF_SIZE-1;
  return radio_write(print_buf, r)? r: 0;
}

uint8_t radio_recv(char* buf) {
    uint8_t size = rx_buffer[0];
    if(size > 0) memcpy(buf, rx_buffer+1, size);
    if(size < 256) buf[size]=0;
    return size;
} // REVISIT relies on being called in the irq

void RADIO_IRQHandler(void){

  if(NRF_RADIO->EVENTS_READY) {
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->TASKS_START = 1;
  }
  if(NRF_RADIO->EVENTS_END) {
    NRF_RADIO->EVENTS_END = 0;

    if(NRF_RADIO->CRCSTATUS == 1) {
      int8_t rssi = -NRF_RADIO->RSSISAMPLE;
      // REVISIT copy quickly to a queue/buffer here!
      if(recv_cb) recv_cb(rssi);
    }
    NRF_RADIO->TASKS_START = 1;
  }
}


