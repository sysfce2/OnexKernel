#ifndef TIME_H
#define TIME_H

#include <stdint.h>

typedef void (*time_up_cb)();

void     time_init();
uint32_t time_ms();
uint32_t time_us();
void     time_delay_s( uint32_t s);
void     time_delay_ms(uint32_t ms);
void     time_delay_us(uint32_t us);
uint16_t time_timeout(time_up_cb cb);
void     time_start_timer(uint16_t id, uint32_t timeout);
void     time_stop_timer(uint16_t id);
void     time_end();

#endif
