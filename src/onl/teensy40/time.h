#ifndef TIME_H
#define TIME_H

#include <stdint.h>

void     time_init();
uint32_t time_ms();
uint32_t time_us();
void     time_delay_s( uint32_t s);
void     time_delay_ms(uint32_t ms);
void     time_delay_us(uint32_t us);

#endif
