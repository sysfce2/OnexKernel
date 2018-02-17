
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
  char buf[128];

  properties* op=properties_new(3);

                    properties_set(op,value_new("x"),(item*)value_new("y"));
                    properties_set(op,value_new("3"),(item*)value_new("4"));

  onex_assert(      properties_size(op)==2,                               "size should be 2");
  onex_assert_equal(item_to_text(properties_get(op,value_new("x")), buf, 128), "y",  "x returns y");
  onex_assert_equal(item_to_text(properties_get(op,value_new("3")), buf, 128), "4",  "3 returns 4");
  onex_assert(     !properties_get(op,value_new("?")),                               "? returns null");

                    properties_set(op,value_new("3"),(item*)value_new("5"));

  onex_assert(      properties_size(op)==2,                               "size should still be 2");
  onex_assert_equal(item_to_text(properties_get(op,value_new("3")), buf, 128), "5",  "3 now returns 5");

                    properties_set(op,value_new("*"),(item*)value_new("+"));

  onex_assert(      properties_size(op)==3,       "size should now be 3");
  onex_assert(     !properties_set(op,value_new("M"),(item*)value_new("N")),   "shouldn't be able to add a fourth item");
  onex_assert(      properties_size(op)==3,       "size should still be 3");

  onex_assert_equal(value_string(properties_key_n(op,1)), "x",            "1st key is 'x'");
  onex_assert_equal(item_to_text(properties_get_n(op,1), buf, 128), "y", "1st val is 'y'");
  onex_assert_equal(value_string(properties_key_n(op,2)), "3",            "2nd key is '3'");
  onex_assert_equal(item_to_text(properties_get_n(op,2), buf, 128), "5", "2nd val is '5'");
  onex_assert_equal(value_string(properties_key_n(op,3)), "*",            "3rd key is '*'");
  onex_assert_equal(item_to_text(properties_get_n(op,3), buf, 128), "+", "3rd val is '+'");
  onex_assert(                  !properties_key_n(op,4),                 "4th key is 0");
  onex_assert(                  !properties_get_n(op,4),                 "4th val is 0");
  onex_assert(                  !properties_key_n(op,0),                 "0th key is 0");
  onex_assert(                  !properties_get_n(op,0),                 "0th val is 0");

  onex_assert_equal(item_to_text(op, buf, 128), "{\n  x: y\n  3: 5\n  *: +\n}\n", "serialise to string works");

                    properties_set(op,value_new("x"),(item*)value_new("i"));

  onex_assert(      properties_get(op,value_new("x"))->type==ITEM_VALUE,             "x is a value");
  onex_assert_equal(item_to_text(properties_get(op,value_new("x")), buf, 128), "i",  "x now returns i");

                    list* li=list_new(3); list_add(li,(item*)value_new("l")); list_add(li,(item*)value_new("m"));
                    properties_set(op,value_new("3"),(item*)li);
  onex_assert(      properties_get(op,value_new("3"))->type==ITEM_LIST,              "x is a list");

  onex_assert_equal(item_to_text(op, buf, 128), "{\n  x: i\n  3: l m\n  *: +\n}\n", "serialise to string works");

                    properties_delete(op,value_new("3"));
  onex_assert(      properties_size(op)==2,                               "after delete size should now be 2");
                    properties_delete(op,value_new("x"));
  onex_assert(      properties_size(op)==1,                               "after delete size should now be 1");
                    properties_delete(op,value_new("*"));
  onex_assert(      properties_size(op)==0,                               "after delete size should now be 0");
  onex_assert_equal(item_to_text(op, buf, 128), "{\n}\n", "serialise to string works");

                    properties_log(op);
}

void run_properties_tests()
{
  log_write("------properties tests-----\n");

  test_items();
  test_properties();
}

