#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#if defined(NRF5)
#include <nrf_log.h>
#include <nrf_log_ctrl.h>
#endif
#include <items.h>
#include <onex-kernel/time.h>

extern bool log_to_gfx;
extern bool log_to_rtt;
extern bool log_to_led;
extern bool debug_on_serial;

extern volatile char* event_log_buffer;

#define LOG_BINARY_FMT "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"
#define LOG_BINARY(x)         \
  (x & 0x80000000? '1': '0'), (x & 0x40000000? '1': '0'), (x & 0x20000000? '1': '0'), (x & 0x10000000? '1': '0'), \
  (x & 0x08000000? '1': '0'), (x & 0x04000000? '1': '0'), (x & 0x02000000? '1': '0'), (x & 0x01000000? '1': '0'), \
  (x & 0x00800000? '1': '0'), (x & 0x00400000? '1': '0'), (x & 0x00200000? '1': '0'), (x & 0x00100000? '1': '0'), \
  (x & 0x00080000? '1': '0'), (x & 0x00040000? '1': '0'), (x & 0x00020000? '1': '0'), (x & 0x00010000? '1': '0'), \
  (x & 0x00008000? '1': '0'), (x & 0x00004000? '1': '0'), (x & 0x00002000? '1': '0'), (x & 0x00001000? '1': '0'), \
  (x & 0x00000800? '1': '0'), (x & 0x00000400? '1': '0'), (x & 0x00000200? '1': '0'), (x & 0x00000100? '1': '0'), \
  (x & 0x00000080? '1': '0'), (x & 0x00000040? '1': '0'), (x & 0x00000020? '1': '0'), (x & 0x00000010? '1': '0'), \
  (x & 0x00000008? '1': '0'), (x & 0x00000004? '1': '0'), (x & 0x00000002? '1': '0'), (x & 0x00000001? '1': '0')

void log_init(properties* config);
bool log_loop();

#define log_write(...)     log_write_current_file_line(0,0, __VA_ARGS__)
#define log_write_src(...) log_write_current_file_line(__FILE__, __LINE__, __VA_ARGS__)
int16_t log_write_current_file_line(char* file, uint32_t line, const char* fmt, ...);

void log_flash(uint8_t r, uint8_t g, uint8_t b);
void log_flush();

bool log_debug_read(char* buf, uint16_t size);

#endif
