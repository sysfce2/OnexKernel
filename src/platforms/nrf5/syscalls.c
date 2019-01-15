
#include <stdint.h>
#include <nrf.h>

int _write(int file, const char* buf, int len)
{
    for(int i=0; i< len; ++i){
        NRF_UART0->EVENTS_TXDRDY = 0;
        NRF_UART0->TXD = buf[i];
        while(!NRF_UART0->EVENTS_TXDRDY) __NOP();
    }
    return len;
}

