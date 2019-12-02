
#include <stdio.h>
#include <stdarg.h>

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

extern int _write(int file, const char* buf, int len);

static bool initialised=false;

static uart_rx_handler_t rx_handler;

void UART0_IRQHandler(void)
{
    NRF_UART0->EVENTS_RXDRDY = 0;
    char buf[1]={NRF_UART0->RXD};
    if(rx_handler) rx_handler(buf);
}

void serial_init(uart_rx_handler_t cb, uint32_t baudrate)
{
    if(initialised) return;
    initialised=true;

    rx_handler = cb;

    uint32_t              baud=UART_BAUDRATE_BAUDRATE_Baud9600;
    if(baudrate==   1200) baud=UART_BAUDRATE_BAUDRATE_Baud1200; else
    if(baudrate==   2400) baud=UART_BAUDRATE_BAUDRATE_Baud2400; else
    if(baudrate==   4800) baud=UART_BAUDRATE_BAUDRATE_Baud4800; else
    if(baudrate==   9600) baud=UART_BAUDRATE_BAUDRATE_Baud9600; else
    if(baudrate==  14400) baud=UART_BAUDRATE_BAUDRATE_Baud14400; else // not available in unix
    if(baudrate==  19200) baud=UART_BAUDRATE_BAUDRATE_Baud19200; else
    if(baudrate==  28800) baud=UART_BAUDRATE_BAUDRATE_Baud28800; else // not available in unix
    if(baudrate==  38400) baud=UART_BAUDRATE_BAUDRATE_Baud38400; else
    if(baudrate==  57600) baud=UART_BAUDRATE_BAUDRATE_Baud57600; else
    if(baudrate==  76800) baud=UART_BAUDRATE_BAUDRATE_Baud76800; else // not available in unix
    if(baudrate== 115200) baud=UART_BAUDRATE_BAUDRATE_Baud115200; else
    if(baudrate== 230400) baud=UART_BAUDRATE_BAUDRATE_Baud230400; else
    if(baudrate== 250000) baud=UART_BAUDRATE_BAUDRATE_Baud250000; else // not available in unix
    if(baudrate== 460800) baud=UART_BAUDRATE_BAUDRATE_Baud460800; else
    if(baudrate== 921600) baud=UART_BAUDRATE_BAUDRATE_Baud921600; else
    if(baudrate==1000000) baud=UART_BAUDRATE_BAUDRATE_Baud1M;

    NRF_UART0->PSELTXD = TX_PIN_NUMBER;
    NRF_UART0->PSELRXD = RX_PIN_NUMBER;

    NRF_UART0->BAUDRATE = (baud << UART_BAUDRATE_BAUDRATE_Pos);
    NRF_UART0->ENABLE   = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);
    NRF_UART0->INTENSET = (UART_INTENSET_RXDRDY_Enabled << UART_INTENSET_RXDRDY_Pos);

    NVIC_ClearPendingIRQ(UART0_IRQn);
    NVIC_SetPriority(UART0_IRQn, 3);
    NVIC_EnableIRQ(UART0_IRQn);

    NRF_UART0->TASKS_STARTTX = 1;
    NRF_UART0->TASKS_STARTRX = 1;
    NRF_UART0->EVENTS_RXDRDY = 0;
}

void serial_cb(uart_rx_handler_t cb)
{
    rx_handler = cb;
}

void serial_putchar(uint32_t ch)
{
  if(!initialised) serial_init(0,0);
  _write(0, (const char*)&ch, 1);
}

int serial_printf(const char* fmt, ...)
{
  if(!initialised) serial_init(0,0);
  va_list args;
  va_start(args, fmt);
  int r=vfprintf(stdout, fmt, args);
  va_end(args);
  return r;
}

