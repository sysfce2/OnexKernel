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
extern bool log_onp;

extern volatile list* gfx_log_buffer;

#define LOG_BINARY_FMT "%c%c%c%c %c%c%c%c  %c%c%c%c %c%c%c%c  %c%c%c%c %c%c%c%c  %c%c%c%c %c%c%c%c"
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

// mode 0 is current simple one
// mode 1 is current with prefixes
// mode 2 is new simple one
// mode 3 is new one with prefixes
#define log_write log_write_0
#define log_write_0(...) log_write_mode(0, __FILE__, __LINE__, __VA_ARGS__)
#define log_write_1(...) log_write_mode(1, __FILE__, __LINE__, __VA_ARGS__)
#define log_write_2(...) log_write_mode(2, __FILE__, __LINE__, __VA_ARGS__)
#define log_write_3(...) log_write_mode(3, __FILE__, __LINE__, __VA_ARGS__)
int16_t log_write_mode(uint8_t mode, char* file, uint32_t line, const char* fmt, ...);

#define log_flash(r,g,b) log_flash_current_file_line(__FILE__, __LINE__, (r),(g),(b))
void log_flash_current_file_line(char* file, uint32_t line, uint8_t r, uint8_t g, uint8_t b);

void log_flush();

bool log_debug_read(char* buf, uint16_t size);

#endif
