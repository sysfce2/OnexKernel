#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <stdbool.h>

void     time_init();
void     time_init_set(uint64_t es);

uint32_t time_s();  // seconds since startup
uint64_t time_es(); // Unix epoch seconds
uint64_t time_ms(); // ms since startup
uint64_t time_us(); // us since startup

void time_es_set(uint64_t es); // set current epoch seconds

typedef void (*time_up_cb)();

uint16_t time_ticker(time_up_cb cb, uint32_t every); // cb every ms; or 1 tick if every=0
uint16_t time_timeout(time_up_cb cb); // cb after ms; or 1 tick if timeout=0
void     time_start_timer(uint16_t id, uint32_t timeout); // for time_timeout()
void     time_stop_timer(uint16_t id);
void     time_end();

#if !defined(NRF5)

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
