
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <onex-kernel/log.h>
#include <onex-kernel/time.h>
#include <tests.h>

// -----------------------------------------------------------------------

static uint16_t success=0;
static uint16_t failure=0;

bool exit_on_fail=false;

bool onex_assert_i(bool condition, const char* fail_message, char* actual, char* expected)
{
  if(condition){
    success++;
    log_write("%d (%s)\n",  success+failure, fail_message);
  }
  else {
    failure++;
    log_write("%d **** Failed to ensure %s ********\n",  success+failure, fail_message);
  }
  if(expected) log_write("    Expected: [%s]\n",  expected);
  if(actual  ) log_write("    Actual:   [%s]\n",  actual);
  log_write("-------------\n");
  if(exit_on_fail && !condition){ onex_assert_summary(); log_write("exit on fail!\n-------------\n"); exit(1); }
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

bool onex_assert_equal_num(int actual, int expected, const char* fail_message)
{
  char a[32]; char e[32];
  snprintf(a, 32, "%d", actual);
  snprintf(e, 32, "%d", expected);
  return onex_assert_i(actual==expected, fail_message, a, e);
}

// ---------------------------------------------------------------------------------------

int onex_assert_summary()
{
  char s[128];
  if(!failure) sprintf(s, "---------------------\nTests done: %d OK, 0 failed\n---------------------\n", success);
  else         sprintf(s, "---------------------\nTests done: %d OK, %d FAILED\n-----------------------\n", success, failure);
  log_write("%s", s);
  return failure;
}

// -----------------------------------------------------------------------

