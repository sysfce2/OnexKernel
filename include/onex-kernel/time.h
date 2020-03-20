#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <stdbool.h>

typedef void (*time_up_cb)();

void     time_init();
uint64_t time_ms();
uint64_t time_us();
void     time_ticker(time_up_cb cb, uint32_t every);

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
