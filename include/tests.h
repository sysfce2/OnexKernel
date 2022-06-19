#ifndef __ASSERT_H__
#define __ASSERT_H__

#ifdef ARDUINO_ARCH_AVR
#include <avr/pgmspace.h>
#endif

#include <stdbool.h>
#include <stdint.h>

/* Some stuff for tests. */

bool onex_assert(      bool condition,               const char* fail_message);
bool onex_assert_equal(char* actual, char* expected, const char* fail_message);
bool onex_assert_equal_num(int actual, int expected, const char* fail_message);

int  onex_assert_summary();


#endif
