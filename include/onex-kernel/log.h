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

#if defined(LOG_TO_RTT)
#define log_write(...) do { NRF_LOG_DEBUG(__VA_ARGS__); time_delay_ms(2); NRF_LOG_FLUSH(); time_delay_ms(2); } while(0)
#else
int  log_write(const char* fmt, ...);
#endif

void log_flush();

#else

#include <android/log.h>

#define log_init()
#define log_loop()
#define log_write(...) ((void)__android_log_print(ANDROID_LOG_INFO, "OnexApp", __VA_ARGS__))
#define log_flush()

#endif

#endif
