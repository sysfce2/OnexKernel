
#include <stdio.h>
#include <stdarg.h>

#include "nrf_log_default_backends.h"

#include <nRF5/m-class-support.h>
#include <boards.h>

#include <onex-kernel/boot.h>
#include <onex-kernel/mem.h>
#include <onex-kernel/serial.h>
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onex-kernel/gpio.h>

#include <onn.h>

bool log_to_gfx=false;
bool log_to_rtt=false;
bool log_to_led=false;
bool debug_on_serial=false;
bool log_onp=false;

#define LOG_BUF_SIZE 1024
static volatile char log_buffer[LOG_BUF_SIZE];
static volatile list* saved_messages = 0;
       volatile list* gfx_log_buffer = 0;
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

  if(initialised) return;

  log_to_gfx      = list_has_value(properties_get(config, "flags"), "log-to-gfx");
  log_to_rtt      = list_has_value(properties_get(config, "flags"), "log-to-rtt");
  log_to_led      = list_has_value(properties_get(config, "flags"), "log-to-led");
  debug_on_serial = list_has_value(properties_get(config, "flags"), "debug-on-serial");
  log_onp         = list_has_value(properties_get(config, "flags"), "log-onp");

#if defined(NRF_LOG_ENABLED)
  if(log_to_rtt){
    NRF_LOG_INIT(NULL);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
  }
#endif

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
    log_flash(0,1,0);
  }

  initialised=true;
}

#define LOG_EARLY_MS 800

#define FLUSH_TO_SERIAL 1
#define FLUSH_TO_RTT    2
#define FLUSH_TO_GFX    3

static char* get_reason_to_save_logs(){
  if(time_ms() < LOG_EARLY_MS)               return "ERL ";
  if(in_interrupt_context())                 return "INT ";
  if(debug_on_serial && !serial_connected()) return "SER ";
  return 0;
}

static void flush_saved_messages(uint8_t to){

  if(get_reason_to_save_logs()) return;

  for(uint8_t i=1; i<=list_size(saved_messages); i++){

    char* msg = list_get_n(saved_messages, i);

    if(to==FLUSH_TO_SERIAL){
      serial_printf("%s", msg);
      mem_free(msg);
    }
    if(to==FLUSH_TO_GFX){
      list_add(gfx_log_buffer, msg);
    }
#if defined(NRF_LOG_ENABLED)
    if(to==FLUSH_TO_RTT){
      NRF_LOG_DEBUG("%s", msg);
      mem_free(msg);
    }
#endif
  }
  list_clear(saved_messages, false);

#if defined(NRF_LOG_ENABLED)
  if(to==FLUSH_TO_RTT){
    NRF_LOG_FLUSH();
  }
#endif
}

bool log_loop() {

  if(!initialised) return true;

  if(debug_on_serial){
    flush_saved_messages(FLUSH_TO_SERIAL);
    if(char_recvd){
      log_write(">%c<----------\n", char_recvd);
      if(char_recvd=='c') onex_show_cache();
      if(char_recvd=='n') onex_show_notify();
      if(char_recvd=='v') value_dump();
      if(char_recvd=='m') mem_show_allocated(true);
      if(char_recvd=='r') boot_reset(false);
      if(char_recvd=='b') boot_reset(true);
      if(char_recvd=='h') log_write("object c.ache n.otifies v.alues m.em r.eset b.ootloader\n");
      char_recvd=0;
    }
  }
  if(log_to_gfx){
    if(!gfx_log_buffer) gfx_log_buffer = list_new(32);
    flush_saved_messages(FLUSH_TO_GFX);
  }
#if defined(NRF_LOG_ENABLED)
  if(log_to_rtt){
    flush_saved_messages(FLUSH_TO_RTT);
  }
  return NRF_LOG_PROCESS();
#else
  return false;
#endif
}

#define LOGCHK if(r >= LOG_BUF_SIZE){ log_flash(1,0,0); return 0; }

int16_t log_write_mode(uint8_t mode, char* file, uint32_t line, const char* fmt, ...){

  va_list args;
  va_start(args, fmt);

  bool fl=(mode==1 || mode==3);
  bool nw=(mode==2 || mode==3);

  // if(!nw) return 0; // narrow down logging to only modes 2/3

  int16_t r=0;

  char* save_reason=get_reason_to_save_logs();
  if(save_reason){
#ifndef LOG_MEM
    if(!saved_messages) saved_messages = list_new(32);
    r+=    snprintf(log_buffer+r, LOG_BUF_SIZE-r, save_reason);                                          LOGCHK
    r+=fl? snprintf(log_buffer+r, LOG_BUF_SIZE-r, "[%ld](%s:%ld) ", (uint32_t)time_ms(), file, line): 0; LOGCHK
    r+=   vsnprintf(log_buffer+r, LOG_BUF_SIZE-r, fmt, args);                                            LOGCHK
    char* lb=mem_strdup(log_buffer);
    if(!list_add(saved_messages, lb)) mem_freestr(lb);
#endif
    return 0;
  }
  if(!initialised) return 0;
  if(debug_on_serial){
    flush_saved_messages(FLUSH_TO_SERIAL);
    r+=fl? serial_printf(                         "[%ld](%s:%ld) ", (uint32_t)time_ms(), file, line): 0;
    r+=    serial_vprintf(fmt, args);
    time_delay_ms(1);
  }
  if(log_to_gfx){
    if(!gfx_log_buffer) gfx_log_buffer = list_new(32);
    flush_saved_messages(FLUSH_TO_GFX);
    r+=fl? snprintf(log_buffer+r, LOG_BUF_SIZE-r, "[%ld](%s:%ld) ", (uint32_t)time_ms(), file, line): 0; LOGCHK
    r+=   vsnprintf(log_buffer+r, LOG_BUF_SIZE-r, fmt, args);                                            LOGCHK
    char* lb=mem_strdup(log_buffer);
    if(!list_add(gfx_log_buffer, lb)) mem_freestr(lb);
  }
#if defined(NRF_LOG_ENABLED)
  if(log_to_rtt){
    flush_saved_messages(FLUSH_TO_RTT);
    r+=fl? snprintf(log_buffer+r, LOG_BUF_SIZE-r, "[%ld](%s:%ld) ", (uint32_t)time_ms(), file, line): 0; LOGCHK
    r+=   vsnprintf(log_buffer+r, LOG_BUF_SIZE-r, fmt, args);                                            LOGCHK
    NRF_LOG_DEBUG("%s", log_buffer);
    time_delay_ms(2);
    NRF_LOG_FLUSH();
    time_delay_ms(2);
  }
#endif
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
#ifdef ONLY_FLASH_111
  if(r+g+b != 3) return;
#endif
  if(!initialised) return;
  if(!strstr(file, "serial") && !strstr(file, "log")){
    log_write_mode(1, file, line, "log_flash\n");
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

void log_flush() {
  if(!initialised) return;
#if defined(NRF_LOG_ENABLED)
  if(log_to_rtt){
    NRF_LOG_FLUSH();
  }
  time_delay_ms(5);
#endif
}


