#define NON_BLOCKING

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <onex-kernel/log.h>
#include <onex-kernel/mem.h>
#include <onex-kernel/radio.h>
#include <onex-kernel/chunkbuf.h>

#define RADIO_READ_BUFFER_SIZE  2048
#define RADIO_WRITE_BUFFER_SIZE 2048

#define RADIO_TXPOWER                  RADIO_TXPOWER_TXPOWER_0dBm
#define RADIO_CHANNEL                  7 // 2.407GHz

#define RADIO_BASE_ADDRESS             0x75626974
#define RADIO_DEFAULT_GROUP            0

#define RADIO_MAX_PACKET_SIZE          252

static char rx_buffer[256];

static volatile bool initialised=false;
static volatile bool sleeping=false;

static volatile channel_recv_cb recv_cb = 0;

// REVISIT: when do I need chunkbuf_clear(radio_write_buf);

static volatile chunkbuf* radio_read_buf = 0;
static volatile chunkbuf* radio_write_buf = 0;

static void switch_to_tx();
static void switch_to_rx();
static bool write_a_packet(uint16_t size);

static volatile bool write_loop_in_progress=false;

static void do_tx_write_block(bool first_write){

  if(first_write && write_loop_in_progress) return;

  switch_to_tx();

  uint16_t size = chunkbuf_read(radio_write_buf, rx_buffer+1, RADIO_MAX_PACKET_SIZE, -1);

  if(!write_a_packet(size)){
    switch_to_rx();
  }
}

static void switch_to_tx(){

  if(write_loop_in_progress) return;
  write_loop_in_progress=true;

  NVIC_DisableIRQ(RADIO_IRQn);

  NRF_RADIO->EVENTS_DISABLED = 0;
  NRF_RADIO->TASKS_DISABLE = 1;
  while(!NRF_RADIO->EVENTS_DISABLED);

  // go to START per packet, not on READY, do; no RSSI
  NRF_RADIO->SHORTS = 0;

  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->TASKS_TXEN = 1;
  while(!NRF_RADIO->EVENTS_READY);

#ifdef NON_BLOCKING
//NVIC_ClearPendingIRQ(RADIO_IRQn); // REVISIT
  NVIC_EnableIRQ(RADIO_IRQn);
#endif
}

static void switch_to_rx(){

  if(!write_loop_in_progress) return;
  write_loop_in_progress = false;

#ifdef NON_BLOCKING
  NVIC_DisableIRQ(RADIO_IRQn);
#endif

  NRF_RADIO->EVENTS_DISABLED = 0;
  NRF_RADIO->TASKS_DISABLE = 1;
  while(!NRF_RADIO->EVENTS_DISABLED);

  NRF_RADIO->SHORTS =
           (RADIO_SHORTS_READY_START_Enabled       << RADIO_SHORTS_READY_START_Pos      )|
           (RADIO_SHORTS_ADDRESS_RSSISTART_Enabled << RADIO_SHORTS_ADDRESS_RSSISTART_Pos);

  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->TASKS_RXEN = 1;
  while(!NRF_RADIO->EVENTS_READY);

  NRF_RADIO->EVENTS_END = 0;

//NVIC_ClearPendingIRQ(RADIO_IRQn); // REVISIT
  NVIC_EnableIRQ(RADIO_IRQn);
}

static bool write_a_packet(uint16_t size){

  if(!size) return false;

  rx_buffer[0]=size;  // first byte is the size (!)

  NRF_RADIO->EVENTS_END = 0;
  NRF_RADIO->TASKS_START = 1;

#ifndef NON_BLOCKING
  while(!NRF_RADIO->EVENTS_END);
  do_tx_write_block(false);
#endif

  return true;
}

bool radio_init(list* bands, channel_recv_cb cb){

  recv_cb = cb;

  if(initialised) return true;

  radio_read_buf  = chunkbuf_new(RADIO_READ_BUFFER_SIZE);
  radio_write_buf = chunkbuf_new(RADIO_WRITE_BUFFER_SIZE);

  NRF_RADIO->MODECNF0 = (RADIO_MODECNF0_DTX_B0 << RADIO_MODECNF0_DTX_Pos) |
                        (RADIO_MODECNF0_RU_Fast << RADIO_MODECNF0_RU_Pos);

  NRF_RADIO->TXPOWER   = RADIO_TXPOWER;
  NRF_RADIO->FREQUENCY = RADIO_CHANNEL; // REVISIT read list* bands

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
  // REVISIT: call switch_to_rx here?
  NVIC_ClearPendingIRQ(RADIO_IRQn);
  NVIC_EnableIRQ(RADIO_IRQn); // RADIO_IRQHandler()

  NRF_RADIO->SHORTS =
           (RADIO_SHORTS_READY_START_Enabled       << RADIO_SHORTS_READY_START_Pos      )|
           (RADIO_SHORTS_ADDRESS_RSSISTART_Enabled << RADIO_SHORTS_ADDRESS_RSSISTART_Pos);

  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->TASKS_RXEN = 1;
  while(!NRF_RADIO->EVENTS_READY);

  NRF_RADIO->EVENTS_END = 0;

  initialised=true;

  if(recv_cb) recv_cb(true, "radio");

  return true;
}

#define NL_DELIM '\n'

uint16_t radio_read(char* buf, uint16_t size){
  if(!initialised) return 0;
  uint16_t r=chunkbuf_readable(radio_read_buf, NL_DELIM);
  if(!r) return 0;
  if(r > size){
    log_flash(1,0,0); // can fill whole buffer without seeing delim
    log_write("**** %d > %d\n", r, size);
    return 0;
  }
  return chunkbuf_read(radio_read_buf, buf, size, NL_DELIM);
}

uint16_t radio_write(char* band, char* buf, uint16_t size) {
  radio_wake();
  if(!chunkbuf_writable(radio_write_buf, size, NL_DELIM)){
    log_flash(1,0,0); // no room for this size
    return 0;
  }
  chunkbuf_write(radio_write_buf, buf, size, NL_DELIM);
  do_tx_write_block(true);
  return size;
}

static int8_t last_rssi = -127;

int8_t radio_last_rssi(){
  return last_rssi;
}

uint16_t radio_available(){
  if(!initialised) return 0;
  return chunkbuf_current_size(radio_read_buf);
}

static void received(char* buf, uint16_t size, int8_t rssi){
  last_rssi=rssi;
  if(!chunkbuf_writable(radio_read_buf, size, -1)){
    log_write("rrb full %d %d\n", size, chunkbuf_current_size(radio_read_buf));
    //log_flash(1,0,0);
    return;
  }
  chunkbuf_write(radio_read_buf, buf, size, -1);
  if(recv_cb) recv_cb(false, "radio");
}

void RADIO_IRQHandler(void){

#ifdef NON_BLOCKING
  if(write_loop_in_progress) {
    if(NRF_RADIO->EVENTS_END) {
      NRF_RADIO->EVENTS_END = 0;
      do_tx_write_block(false);
    }
    return;
  }
#endif

  if(NRF_RADIO->EVENTS_END) {
    NRF_RADIO->EVENTS_END = 0;
    NRF_RADIO->TASKS_START = 1;

    uint8_t size;
    if(NRF_RADIO->CRCSTATUS == 1) {
      size = rx_buffer[0];
    }
    else{
      #define CORRUPTION_MAGIC " 🍌 banana!\n"
      size = strlen(CORRUPTION_MAGIC);
      mem_strncpy(rx_buffer+1, CORRUPTION_MAGIC, size);
    }
    int8_t rssi = -NRF_RADIO->RSSISAMPLE;
    received(rx_buffer+1, size, rssi);
  }
}

void radio_sleep(){
  if(sleeping) return;
  sleeping=true;
}

void radio_wake(){
  if(!sleeping) return;
  sleeping=false;
}




