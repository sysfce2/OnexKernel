// --------------------------------------------------------------------

#include <stdint.h>
#include <variant.h>

#include <onex-kernel/gpio.h>
#include <onex-kernel/serial.h>
#include <onex-kernel/radio.h>

static void serial_received(char* ch)
{
}

uint8_t recvmsg[16];

void receive_data()
{
  serial_printf("receive_data..\n");
  uint8_t len = sizeof(recvmsg);
  if(radio_recv(recvmsg, &len)){
     serial_printf("RSSI: %d\n", radio_last_rssi());
  }
}

int main()
{
  serial_init(serial_received, 9600);
  radio_init(0,0,0);
  gpio_mode(BUTTON_A, INPUT_PULLUP);

  for(;;){
    if(radio_available()) receive_data();
    if(!gpio_get(BUTTON_A)) radio_send("button down", 12);
  }
}

// --------------------------------------------------------------------
