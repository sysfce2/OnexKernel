
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

static bool init_serial(char* devtty, int s){

    int speed=(s==115200? B115200: (s==57600? B57600: B9600));

    log_write("opening %s @ %d\n", devtty, s);

    serialfd=open(devtty, O_RDWR | O_NOCTTY | O_SYNC | O_NDELAY);
    if(serialfd< 0){ log_write("error %d %s opening %s\n", errno, strerror(errno), devtty); return false; }

    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if(tcgetattr(serialfd, &tty)){ log_write("error %d %s in tcgetattr\n", errno, strerror(errno)); return false; }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

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
//ioctl(serialfd, FIONREAD, &bytes_available_for_reading);
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

