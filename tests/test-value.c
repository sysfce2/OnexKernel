
#include <assert.h>
#include <onex-kernel/log.h>
#include <items.h>

// ---------------------------------------------------------------------------------

void test_value()
{
  value* v1=value_new("banana");
  value* v2=value_new("banana");

  onex_assert_equal(value_string(v1), "banana",           "get it back");
  onex_assert(      value_string(v1) == value_string(v2), "all bananas are the same");

                    value_log(v1);
}

void run_value_tests()
{
  log_write("------value tests-----\n");

  test_value();
}


