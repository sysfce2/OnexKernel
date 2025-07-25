
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
#include <onex-kernel/time.h>

static int init_serial(char* devtty, int b){

    int fd=open(devtty, O_RDWR | O_NOCTTY | O_SYNC | O_NDELAY);
    if(fd< 0) return -1;

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

    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if(tcgetattr(fd, &tty)){ log_write("error %d %s in tcgetattr\n", errno, strerror(errno)); return -1; }

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

    if(tcsetattr(fd, TCSANOW, &tty)){ log_write("error %d %s in tcsetattr\n", errno, strerror(errno)); return -1; }
    return fd;
}

static list*           serial_ttys=0;
static uint32_t        baudrate;
static channel_recv_cb recv_cb;

static uint32_t nextupdate=0;

static bool initialised=false;

bool serial_init(list* ttys, uint32_t br, channel_recv_cb cb) {
  if(initialised) return true; initialised = true;
  serial_ttys=ttys;
  baudrate=br;
  recv_cb=cb;
  return true;
}

static bool connected=false;

uint8_t serial_ready_state(){ return SERIAL_READY; }
uint8_t serial_status(){      return SERIAL_READY; }
bool    serial_connected(){   return connected; }

#define MAX_TTYS 3
int fds[MAX_TTYS]  = {-1,-1,-1};

void update_connected_serials() {

  if(time_ms() < nextupdate) return;
  nextupdate=time_ms()+250;

  for(uint8_t t=0; t<list_size(serial_ttys) && t<MAX_TTYS; t++){
    if(fds[t]!= -1) continue;
    char* tty = value_string(list_get_n(serial_ttys, t+1));
    fds[t]=init_serial(tty, baudrate);
    char channel[256]; snprintf(channel, 256, "serial-%s", tty+strlen("/dev/"));
    if(fds[t]!= -1){
      connected=true;
      if(recv_cb) recv_cb(true, channel);
    }
  }
}

static char nl_delim = '\n';

#define SERIAL_MAX_LENGTH 4096

static uint16_t ser_index[MAX_TTYS]={0,0,0};
static char     ser_buff[MAX_TTYS][SERIAL_MAX_LENGTH];
static uint8_t  nt=0;

uint16_t serial_available(){
  uint16_t available=0;
  for(int n=0; n<MAX_TTYS; n++){
    int fd=fds[nt];
    if(fd== -1) continue;
    int bytes_available_for_reading=0;
    ioctl(fd, FIONREAD, &bytes_available_for_reading);
    available+=bytes_available_for_reading;
  }
  return available;
}

int16_t serial_read(char* buf, uint16_t size) {
  update_connected_serials();
  for(int n=0; n<MAX_TTYS; n++){
    int fd=fds[nt];
    if(fd!= -1){
      int bytes_available_for_reading=0;
      ioctl(fd, FIONREAD, &bytes_available_for_reading);
      if(bytes_available_for_reading){
        for(; read(fd, ser_buff[nt]+ser_index[nt], 1)==1; ser_index[nt]++){
          if(ser_index[nt]==SERIAL_MAX_LENGTH-1 || ser_buff[nt][ser_index[nt]]==nl_delim){
            ser_buff[nt][ser_index[nt]]=0;
            uint16_t ss = ser_index[nt]+1;
            ser_index[nt]=0;
            uint16_t s=size<ss? size: ss;
            memcpy(buf, ser_buff[nt], s);
            nt++; if(nt==MAX_TTYS) nt=0;
            return s-1;
          }
        }
      }
    }
    nt++; if(nt==MAX_TTYS) nt=0;
  }
  return 0;
}

static uint16_t serial_write_delim(char* tty, char* buf, uint16_t size, bool delim);

uint16_t serial_write(char* tty, char* buf, uint16_t size){
  return serial_write_delim(tty, buf, size, true);
}

int16_t serial_printf(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int16_t size=serial_vprintf(fmt,args);
  va_end(args);
  return size;
}

#define PRINT_BUF_SIZE 4096
char print_buf[PRINT_BUF_SIZE];

int16_t serial_vprintf(const char* fmt, va_list args) {
  int16_t size=vsnprintf(print_buf, PRINT_BUF_SIZE, fmt, args);
  if(size>=PRINT_BUF_SIZE) return 0;
  return serial_write_delim("all", print_buf, size, false);
}

static uint16_t serial_write_delim(char* tty, char* buf, uint16_t size, bool delim){
  uint16_t i=0;
  for(uint8_t t=0; t< MAX_TTYS; t++){
    int fd=fds[t]; if(fd== -1) continue;
    int16_t j=write(fd, buf, size);
    if(j<0) continue; // REVISIT!
    i=j;
    if(delim){
      int16_t k=write(fd, &nl_delim, 1);
      if(k<0) continue; // REVISIT!
    }
  }
  return i; // REVISIT: returns last tty chars written
}

