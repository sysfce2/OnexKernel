#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(NRF51)
#include <nrf51.h>
#include <nrf51_bitfields.h>
#endif

#if defined(NRF52)
#include <nrf52.h>
#include <nrf52_bitfields.h>
#endif

#include <variant.h>

#include <onex-kernel/serial.h>
#include <onex-kernel/radio.h>

#define NRF51_RADIO_BASE_ADDRESS             0x75626974
#define NRF51_RADIO_DEFAULT_GROUP            0
#define NRF51_RADIO_DEFAULT_FREQUENCY        7
#define NRF51_RADIO_MAX_PACKET_SIZE          32
#define NRF51_RADIO_HEADER_SIZE              4
#define NRF51_RADIO_MAXIMUM_RX_BUFFERS       4
#define NRF51_RADIO_STATUS_INITIALISED       0x0001
#define NRF51_RADIO_DEFAULT_TX_POWER         6
#define NRF51_RADIO_POWER_LEVELS             8
#define NRF51_RADIO_MAXIMUM_RX_BUFFERS       4

const int8_t NRF51_RADIO_POWER_LEVEL[] = {-30, -20, -16, -12, -8, -4, 0, 4};

uint8_t group = NRF51_RADIO_DEFAULT_GROUP;
uint8_t queue_depth = 0;
 int8_t rssi = 0;
uint8_t status = 0;

typedef struct FrameBuffer FrameBuffer;

struct FrameBuffer
{
  uint8_t      length;  // remaining bytes in the packet; includes protocol/version/group fields, excluding the length field itself
  uint8_t      payload[NRF51_RADIO_MAX_PACKET_SIZE];
  uint8_t      version;
  uint8_t      group;
  uint8_t      protocol;
  FrameBuffer* next;
   int8_t      rssi;
};

FrameBuffer* rx_queue = 0;
FrameBuffer* rx_buf = 0;

static bool set_transmit_power(int power)
{
  if (power < 0 || power >= NRF51_RADIO_POWER_LEVELS) return false;
  NRF_RADIO->TXPOWER = (uint32_t)NRF51_RADIO_POWER_LEVEL[power];
  return true;
}

static bool set_frequency_band(int band)
{
  if (band < 0 || band > 100) return false;
  NRF_RADIO->FREQUENCY = (uint32_t)band;
  return true;
}

static void set_group(uint8_t g)
{
  group = g;
  NRF_RADIO->PREFIX0 = (uint32_t)group;
}

bool radio_init(uint8_t ss_pin, uint8_t interrupt_pin, bool swspi) // args not used
{
  rx_buf = (FrameBuffer*)malloc(sizeof(FrameBuffer));

  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  while(!NRF_CLOCK->EVENTS_HFCLKSTARTED);

  set_transmit_power(NRF51_RADIO_DEFAULT_TX_POWER);
  set_frequency_band(NRF51_RADIO_DEFAULT_FREQUENCY);

  NRF_RADIO->MODE = RADIO_MODE_MODE_Nrf_1Mbit;
  NRF_RADIO->BASE0 = NRF51_RADIO_BASE_ADDRESS;
  set_group(group);
  NRF_RADIO->TXADDRESS = 0;
  NRF_RADIO->RXADDRESSES = 1;
  NRF_RADIO->PCNF0 = 0x00000008;
  NRF_RADIO->PCNF1 = 0x02040000 | NRF51_RADIO_MAX_PACKET_SIZE;

  NRF_RADIO->CRCCNF = RADIO_CRCCNF_LEN_Two;
  NRF_RADIO->CRCINIT = 0xFFFF;
  NRF_RADIO->CRCPOLY = 0x11021;
  NRF_RADIO->DATAWHITEIV = 0x18;

  NRF_RADIO->PACKETPTR = (uint32_t)rx_buf;

  NRF_RADIO->INTENSET = 0x00000008;
  NVIC_ClearPendingIRQ(RADIO_IRQn);
  NVIC_EnableIRQ(RADIO_IRQn);
  NRF_RADIO->SHORTS |= RADIO_SHORTS_ADDRESS_RSSISTART_Msk;

  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->TASKS_RXEN = 1;
  while(!NRF_RADIO->EVENTS_READY);
  NRF_RADIO->EVENTS_END = 0;
  NRF_RADIO->TASKS_START = 1;

  status |= NRF51_RADIO_STATUS_INITIALISED;
  serial_printf("radio init\n");
  return true;
}

bool radio_set_frequency(float freq)
{
  return false;
}

void radio_set_encryption_key(uint8_t* key)
{
}

static bool send(FrameBuffer* buffer)
{
  if(!buffer) return false;
  if (buffer->length > NRF51_RADIO_MAX_PACKET_SIZE + NRF51_RADIO_HEADER_SIZE - 1) return false;

  NVIC_DisableIRQ(RADIO_IRQn);
  NRF_RADIO->EVENTS_DISABLED = 0;
  NRF_RADIO->TASKS_DISABLE = 1;
  while(!NRF_RADIO->EVENTS_DISABLED);

  NRF_RADIO->PACKETPTR = (uint32_t)buffer;

  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->TASKS_TXEN = 1;
  while(!NRF_RADIO->EVENTS_READY);

  NRF_RADIO->TASKS_START = 1;
  NRF_RADIO->EVENTS_END = 0;
  while(!NRF_RADIO->EVENTS_END);

  NRF_RADIO->PACKETPTR = (uint32_t)rx_buf;

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

void radio_send(uint8_t* sendmsg, size_t len)
{
  FrameBuffer b;
  b.length = len;
  memcpy(b.payload, sendmsg, len);
  serial_printf("sending: %s %d\n", b.payload, b.length);
  send(&b);
}

bool radio_wait_packet_sent(uint16_t timeout)
{
  return true;
}

int frames_available()
{
    return queue_depth;
}

bool radio_available()
{
  return !!queue_depth;
}

static FrameBuffer* recv()
{
  FrameBuffer* p = rx_queue;
  if(!p) return 0;
  NVIC_DisableIRQ(RADIO_IRQn);  // not in latest sources?
  rx_queue = p->next;
  queue_depth--;
  NVIC_EnableIRQ(RADIO_IRQn);
  return p;
}

bool radio_recv(uint8_t* recvmsg, uint8_t* len)
{
  FrameBuffer* b = recv();
  serial_printf("received %s %d %d\n", b->payload, b->length, b->rssi);
  memcpy(recvmsg, b->payload, *len);
  return true;
}

int8_t radio_last_rssi()
{
  return rssi;
}

bool queue_rx_buf()
{
  if (!rx_buf) return false;
  if (queue_depth >= NRF51_RADIO_MAXIMUM_RX_BUFFERS) return false;
  rx_buf->rssi = rssi;
  FrameBuffer* b = (FrameBuffer*)malloc(sizeof(FrameBuffer));
  if (!b) return false;
  rx_buf->next = 0;
  if (!rx_queue) rx_queue = rx_buf;
  else {
    FrameBuffer* p = rx_queue;
    while (p->next) p = p->next;
    p->next = rx_buf;
  }
  queue_depth++;
  rx_buf = b;
  return true;
}

void RADIO_IRQHandler(void)
{
  if(NRF_RADIO->EVENTS_READY) {
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->TASKS_START = 1;
  }
  if(NRF_RADIO->EVENTS_END) {
    NRF_RADIO->EVENTS_END = 0;
    if(NRF_RADIO->CRCSTATUS == 1) {
      rssi= -NRF_RADIO->RSSISAMPLE;
      queue_rx_buf();
      NRF_RADIO->PACKETPTR = (uint32_t)rx_buf;
    } else {
      rssi=0;
    }
    NRF_RADIO->TASKS_START = 1;
  }
}

