#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <stdbool.h>

void     time_init();

uint64_t time_es(); // Unix epoch seconds
uint64_t time_ms(); // ms since startup
uint64_t time_us(); // us since startup

void time_es_set(uint64_t es); // set current epoch seconds

typedef void (*time_up_cb)();

void time_ticker(time_up_cb cb, uint32_t every); // cb every ms; or 1 tick if every=0

#if defined(TARGET_LINUX) || defined(__ANDROID__)

 #include <unistd.h>
 #include <time.h>

 #define time_delay_s(m)  usleep(1000000 * (m))
 #define time_delay_ms(m) usleep(   1000 * (m))
 #define time_delay_us(m) usleep(          (m))

#else

void time_delay_s( uint32_t s);
void time_delay_ms(uint32_t ms);
void time_delay_us(uint32_t us);

#endif

#endif
