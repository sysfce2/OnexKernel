#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <stdbool.h>

void     time_init();
uint32_t time_ms();
uint32_t time_us();

#ifdef TARGET_LINUX

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
