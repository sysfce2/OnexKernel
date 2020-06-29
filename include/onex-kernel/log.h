#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#if defined(NRF5)
#include <nrf_log.h>
#include <nrf_log_ctrl.h>
#endif
#include <onex-kernel/time.h>

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

#if !defined(__ANDROID__)

void log_init();
bool log_loop();
int  log_write(const char* fmt, ...);
void log_flush();

#else

#include <android/log.h>

#define log_init()
#define log_loop() true
#define log_write(...) ((void)__android_log_print(ANDROID_LOG_INFO, "OnexApp", __VA_ARGS__))
#define log_flush()

#endif

#endif
