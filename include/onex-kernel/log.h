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
