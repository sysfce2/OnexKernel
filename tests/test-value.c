
#include <assert.h>
#include <onex-kernel/log.h>
#include <items.h>

// ---------------------------------------------------------------------------------

void test_value()
{
  value* v1=value_new("banana");
  value* v2=value_new("banana");

  onex_assert_equal(value_get(v1), "banana",        "get it back");
  onex_assert(      value_get(v1) == value_get(v2), "all bananas are the same");

                    value_log(v1);
                    value_log(v2);
}

void run_value_tests()
{
  log_write("------value tests-----\n");

  test_value();
}


