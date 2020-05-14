#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <stdio.h>

#if !defined(__ANDROID__)
void log_init();
void log_loop();
int  log_write(const char* fmt, ...);
#else
#include <android/log.h>
#define log_init()
#define log_write(...) ((void)__android_log_print(ANDROID_LOG_INFO, "OnexApp", __VA_ARGS__))
#endif

#endif
