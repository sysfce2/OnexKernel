
#include <stdio.h>
#include <stdarg.h>

#include "nrf_log_default_backends.h"

#include <nRF5/m-class-support.h>
#include <boards.h>

#include <onex-kernel/boot.h>
#include <onex-kernel/serial.h>
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onex-kernel/gpio.h>

#include <onn.h>

bool log_to_gfx=false;
bool log_to_rtt=false;
bool log_to_led=false;
bool debug_on_serial=false;

volatile char* event_log_buffer=0;

#define LOG_BUF_SIZE 1024
static volatile char log_buffer[LOG_BUF_SIZE];
static volatile list* saved_messages = 0;
static volatile char char_recvd=0;

static void serial_cb(bool connect, char* tty){
  if(connect) return;
  uint16_t size = serial_read(log_buffer, LOG_BUF_SIZE);
  if(debug_on_serial) log_debug_read(log_buffer, size);
}

bool log_debug_read(char* buf, uint16_t size){
  if(size>=2 && buf[0] == '#'){
    char_recvd=buf[1];
    return true;
  }
  return false;
}

uint16_t    flash_id=0;
static void flash_time_cb(void*);
static volatile bool initialised=false;

void log_init(properties* config) {

  if(initialised) return; initialised=true;

  log_to_gfx      = list_has_value(properties_get(config, "flags"), "log-to-gfx");
  log_to_rtt      = list_has_value(properties_get(config, "flags"), "log-to-rtt");
  log_to_led      = list_has_value(properties_get(config, "flags"), "log-to-led");
  debug_on_serial = list_has_value(properties_get(config, "flags"), "debug-on-serial");

  if(debug_on_serial) serial_init(0,0,serial_cb);

  if(log_to_led){
    gpio_init();
#if defined(BOARD_PCA10059)
    gpio_mode(LED2_R, OUTPUT);
    gpio_mode(LED2_G, OUTPUT);
    gpio_mode(LED2_B, OUTPUT);
    gpio_set(LED2_R, !LEDS_ACTIVE_STATE);
    gpio_set(LED2_G, !LEDS_ACTIVE_STATE);
    gpio_set(LED2_B, !LEDS_ACTIVE_STATE);
#elif defined(BOARD_FEATHER_SENSE)
    gpio_mode(LED_1, OUTPUT);
    gpio_set(LED_1,  !LEDS_ACTIVE_STATE);
#endif
    time_init();
    flash_id=time_timeout(flash_time_cb,0);
    log_flash(1,1,1);
  }

#if defined(NRF_LOG_ENABLED)
  NRF_LOG_INIT(NULL);
  NRF_LOG_DEFAULT_BACKENDS_INIT();
#endif
}

#define LOG_EARLY_MS 1000

static void flush_saved_messages(){
  if(time_ms() < LOG_EARLY_MS) return;
  for(uint8_t i=1; i<=list_size(saved_messages); i++){
    char* msg = list_get_n(saved_messages, i);
    serial_printf("%s", msg);
    free(msg);
  }
  list_clear(saved_messages, false);
}

bool log_loop() {

  flush_saved_messages();

  if(debug_on_serial){
    if(char_recvd){
      log_write(">%c<----------\n", char_recvd);
      if(char_recvd=='r') boot_reset(false);
      if(char_recvd=='b') boot_reset(true);
      if(char_recvd=='c') onex_show_cache();
      if(char_recvd=='v') value_dump();
      if(char_recvd=='h') log_write("r: reset; b: bootloader; c: object cache; v: values\n");
      char_recvd=0;
    }
  }

#if defined(NRF_LOG_ENABLED)
  return NRF_LOG_PROCESS();
#else
  return false;
#endif
}

#define LOGCHK if(ln >= LOG_BUF_SIZE){ log_flash(1,0,0); return 0; }
int16_t log_write_current_file_line(char* file, uint32_t line, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int16_t r=0;
  if(debug_on_serial){
    char* save_reason=0;
    if(in_interrupt_context())   save_reason="LOG IN INT ";
    if(time_ms() < LOG_EARLY_MS) save_reason="LOG EARLY ";
    if(save_reason){
      uint16_t ln=0;
      ln+=      snprintf(log_buffer+ln, LOG_BUF_SIZE, save_reason);                                          LOGCHK
      ln+=file? snprintf(log_buffer+ln, LOG_BUF_SIZE, "[%ld](%s:%ld) ", (uint32_t)time_ms(), file, line): 0; LOGCHK
      ln+=     vsnprintf(log_buffer+ln, LOG_BUF_SIZE-ln, fmt, args);                                         LOGCHK
      if(!saved_messages) saved_messages = list_new(20);
      list_add(saved_messages, strdup(log_buffer));
      return 0;
    }
    flush_saved_messages();

    r+=file? serial_printf("[%ld](%s:%ld) ", (uint32_t)time_ms(), file, line): 0;
    r+=serial_vprintf(fmt, args);
    time_delay_ms(1);
  }
  if(log_to_gfx){
    uint16_t ln=file? snprintf(log_buffer, LOG_BUF_SIZE, "%s:%ld:", file, line): 0;
    ln+=vsnprintf(log_buffer+ln, LOG_BUF_SIZE-ln, fmt, args);
    if(ln >= LOG_BUF_SIZE){ log_flash(1,0,0); return 0; }
    event_log_buffer=log_buffer;
  }
  if(log_to_rtt){
    uint16_t ln=vsnprintf(log_buffer, LOG_BUF_SIZE, fmt, args);
    if(ln >= LOG_BUF_SIZE){ log_flash(1,0,0); return 0; }
    NRF_LOG_DEBUG("%s", log_buffer);
    time_delay_ms(2);
    NRF_LOG_FLUSH();
    time_delay_ms(2);
  }
  va_end(args);
  return r;
}

static volatile bool flash_on=false;

void flash_time_cb(void*) {
#if defined(BOARD_PCA10059)
  gpio_set(LED2_R, !LEDS_ACTIVE_STATE);
  gpio_set(LED2_G, !LEDS_ACTIVE_STATE);
  gpio_set(LED2_B, !LEDS_ACTIVE_STATE);
#elif defined(BOARD_FEATHER_SENSE)
  gpio_set(LED_1,  !LEDS_ACTIVE_STATE);
#endif
  flash_on=false;
}

void log_flash_current_file_line(char* file, uint32_t line, uint8_t r, uint8_t g, uint8_t b){
  if(!strstr(file, "serial") && !strstr(file, "log")){
    log_write_current_file_line(file, line, "log_flash\n");
  }
  if(!log_to_led || flash_on) return;
#if defined(BOARD_PCA10059)
  if(r) gpio_set(LED2_R, LEDS_ACTIVE_STATE);
  if(g) gpio_set(LED2_G, LEDS_ACTIVE_STATE);
  if(b) gpio_set(LED2_B, LEDS_ACTIVE_STATE);
#elif defined(BOARD_FEATHER_SENSE)
  gpio_set(LED_1,  LEDS_ACTIVE_STATE);
#endif
  flash_on=true;
  time_start_timer(flash_id, 250);
}

void log_flush()
{
#if defined(NRF_LOG_ENABLED)
  NRF_LOG_FLUSH();
  time_delay_ms(5);
#endif
}


