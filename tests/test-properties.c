
#include <stdio.h>
#include <assert.h>
#include <onex-kernel/log.h>
#include <items.h>

// ---------------------------------------------------------------------------------

void test_items()
{
  properties* op=properties_new(3);
  list*       li=list_new(3);
  value*      va=value_new("banana");
  onex_assert(item_is_type(op, ITEM_PROPERTIES), "properties has the right type");
  onex_assert(item_is_type(li, ITEM_LIST),       "list has the right type");
  onex_assert(item_is_type(va, ITEM_VALUE),      "value has the right type");
}

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

  char buf[128];
  onex_assert_equal(item_to_text(op, buf, 128), "{\n  x: y\n  3: 5\n  *: +\n}\n", "serialise to string works");

                    value* v=value_new("i");
                    properties_set_item(op,"x",(item*)v);

  onex_assert(      properties_type(op,"x")==ITEM_VALUE, "x is a value");
  onex_assert_equal(properties_get(op,"x"), "i",         "x now returns i");

                    v=value_new("v");
                    properties_set_value(op,"x",v);

  onex_assert(      properties_type(op,"x")==ITEM_VALUE, "x is a value");
  onex_assert_equal(properties_get(op,"x"), "v",         "x now returns v");

/*
                    list* li=list_new(3); list_add(li,"l"); list_add(li,"m");
                    properties_set_list(op,"3",li);
  onex_assert_equal(properties_type(op,"3"), ITEM_LIST, "x is a list");

                    properties_set_properties(op,"*",);
  onex_assert_equal(properties_type(op,"*"), ITEM_PROPERTIES, "x is a properties");
*/
  onex_assert_equal(item_to_text(op, buf, 128), "{\n  x: v\n  3: 5\n  *: +\n}\n", "serialise to string works");

                    properties_log(op);
}

void run_properties_tests()
{
  log_write("------properties tests-----\n");

  test_items();
  test_properties();
}

