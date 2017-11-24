
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#include <onex-kernel/log.h>
#include <onex-kernel/serial.h>

int serialfd;

static bool init_serial(char* devtty, int b){

    int baud=B9600;
    if(b==   1200) baud=   B1200; else
    if(b==   1800) baud=   B1800; else // not available in nrf51
    if(b==   2400) baud=   B2400; else
    if(b==   4800) baud=   B4800; else
    if(b==   9600) baud=   B9600; else
    if(b==  19200) baud=  B19200; else
    if(b==  38400) baud=  B38400; else
    if(b==  57600) baud=  B57600; else
    if(b== 115200) baud= B115200; else
    if(b== 230400) baud= B230400; else
    if(b== 460800) baud= B460800; else
    if(b== 500000) baud= B500000; else // not available in nrf51
    if(b== 576000) baud= B576000; else // not available in nrf51
    if(b== 921600) baud= B921600; else
    if(b==1000000) baud=B1000000; else
    if(b==1152000) baud=B1152000; else // not available in nrf51
    if(b==1500000) baud=B1500000; else // not available in nrf51
    if(b==2000000) baud=B2000000; else // not available in nrf51
    if(b==2500000) baud=B2500000; else // not available in nrf51
    if(b==3000000) baud=B3000000; else // not available in nrf51
    if(b==3500000) baud=B3500000; else // not available in nrf51
    if(b==4000000) baud=B4000000; else log_write("speed %d not found: using default 9600 baud!\n", b);

    log_write("opening %s @ %d\n", devtty, b);

    serialfd=open(devtty, O_RDWR | O_NOCTTY | O_SYNC | O_NDELAY);
    if(serialfd< 0){ log_write("error %d %s opening %s\n", errno, strerror(errno), devtty); return false; }

    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if(tcgetattr(serialfd, &tty)){ log_write("error %d %s in tcgetattr\n", errno, strerror(errno)); return false; }

    cfsetospeed(&tty, baud);
    cfsetispeed(&tty, baud);

    tty.c_iflag &= ~IGNBRK;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    tty.c_lflag = 0;
    tty.c_oflag = 0;

    tty.c_cc[VMIN]  = 0;  // http://www.unixwiz.net/techtips/termios-vmin-vtime.html
    tty.c_cc[VTIME] = 5;

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if(tcsetattr(serialfd, TCSANOW, &tty)){ log_write("error %d %s in tcsetattr\n", errno, strerror(errno)); return false; }
    return true;
}

static bool initialised=false;

void serial_init(uart_rx_handler_t cb, uint32_t baudrate)
{
  if(initialised) return;
  char* devtty = "/dev/ttyACM0";
  init_serial(devtty, baudrate);
  initialised=true;
}

#define SERIAL_MAX_LENGTH 128

int  i=0;
char ser_buff[SERIAL_MAX_LENGTH];
int  ser_size=0;

int serial_recv(char* b, int l)
{
  if(!initialised) return -1;
  int bytes_available_for_reading=0;
  ioctl(serialfd, FIONREAD, &bytes_available_for_reading);
  if(!bytes_available_for_reading) return -1;
  for(; read(serialfd, ser_buff+i, 1)==1; i++){
    if(i==SERIAL_MAX_LENGTH-1 || ser_buff[i]=='\n'){
      ser_buff[i]=0;
      ser_size = i+1;
      i=0;
      int size=l<ser_size? l: ser_size;
      memcpy(b, ser_buff, size);
      ser_size=0;
      return size;
    }
  }
  return -1;
}

int serial_printf(const char* fmt, ...)
{
  if(!initialised) return -1;
  va_list args;
  va_start(args, fmt);
  char b[256];
  int n=vsnprintf(b, 256, fmt, args);
  int i=write(serialfd, b, n); if(i<0) return -1;
  va_end(args);
  return i;
}


