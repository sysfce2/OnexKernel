#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

void log_init(uint32_t baudrate);
#if !defined(__ANDROID__)
int  log_write(const char* fmt, ...);
#else
#include <android/log.h>
#define log_write(...) ((void)__android_log_print(ANDROID_LOG_INFO, "OnexApp", __VA_ARGS__))
#endif

#endif
