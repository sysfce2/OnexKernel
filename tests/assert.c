
#include <stdio.h>
#include <string.h>

#include <onex-kernel/serial.h>
#include <assert.h>

// -----------------------------------------------------------------------

static uint16_t success=0;
static uint16_t failure=0;

bool onex_assert_i(bool condition, const char* fail_message, char* actual, char* expected)
{
  if(condition){
    success++;
    serial_printf("%d (%s)\n",  success+failure, fail_message);
  }
  else {
    failure++;
    serial_printf("%d **** Failed to ensure %s\n",  success+failure, fail_message);
  }
  if(expected) serial_printf("    Expected: [%s]\n",  expected);
  if(actual  ) serial_printf("    Actual:   [%s]\n",  actual);
  return condition;
}

bool onex_assert(bool condition, const char* fail_message)
{
  return onex_assert_i(condition, fail_message, 0,0);
}

bool onex_assert_equal(char* actual, char* expected, const char* fail_message)
{
  return onex_assert_i(actual && expected && !strcmp(actual, expected), fail_message, actual, expected);
}

// ---------------------------------------------------------------------------------------

int onex_assert_summary()
{
  char s[128];
  if(!failure) sprintf(s, "---------------------\nTests done: %d OK, no failures\n---------------------\n", success);
  else         sprintf(s, "---------------------\nTests done: %d OK, %d FAILED\n-----------------------\n", success, failure);
  serial_printf("%s", s);
  return failure;
}

// -----------------------------------------------------------------------

