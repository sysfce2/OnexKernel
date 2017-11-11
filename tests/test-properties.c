
#include <stdio.h>
#include <assert.h>
#include <onex-kernel/log.h>
#include <properties.h>

// ---------------------------------------------------------------------------------

void test_properties()
{
  properties* op=properties_new(3);

                    properties_set(op,"x","y");
                    properties_set(op,"3","4");

  onex_assert(      properties_size(op)==2,       "size should be 2");
  onex_assert_equal(properties_get(op,"x"), "y",  "x returns y");
  onex_assert_equal(properties_get(op,"3"), "4",  "3 returns 4");
  onex_assert(     !properties_get(op,"?"),       "? returns null");

                    properties_set(op,"3","5");

  onex_assert(      properties_size(op)==2,       "size should still be 2");
  onex_assert_equal(properties_get(op,"3"), "5",  "3 now returns 5");

                    properties_set(op,"*","+");

  onex_assert(      properties_size(op)==3,       "size should now be 3");
  onex_assert(     !properties_set(op,"M","N"),   "shouldn't be able to add a fourth item");
  onex_assert(      properties_size(op)==3,       "size should still be 3");

  onex_assert_equal(properties_get_key(op,1), "x", "1st key is 'x'");
  onex_assert_equal(properties_get_val(op,1), "y", "1st val is 'y'");
  onex_assert_equal(properties_get_key(op,2), "3", "2nd key is '3'");
  onex_assert_equal(properties_get_val(op,2), "5", "2nd val is '5'");
  onex_assert_equal(properties_get_key(op,3), "*", "3rd key is '*'");
  onex_assert_equal(properties_get_val(op,3), "+", "3rd val is '+'");
  onex_assert(     !properties_get_key(op,4),      "4th key is 0");
  onex_assert(     !properties_get_val(op,4),      "4th val is 0");
  onex_assert(     !properties_get_key(op,0),      "0th key is 0");
  onex_assert(     !properties_get_val(op,0),      "0th val is 0");

                    properties_log(op);
}

void run_properties_tests()
{
  log_write("------properties tests-----\n");

  test_properties();
}

