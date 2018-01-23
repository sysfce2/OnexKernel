
#include <stdio.h>
#include <assert.h>
#include <onex-kernel/log.h>
#include <items.h>

// ---------------------------------------------------------------------------------

void test_list()
{
  char buf[128];

  list* li=list_new(3);

                                 list_add(  li, (item*)value_new("y"));
                                 list_add(  li, (item*)value_new("3"));

  onex_assert(                   list_size( li)==2,              "size should be 2");
  onex_assert(                  !list_get_n(li,-1),              "-1th item is null");
  onex_assert(                  !list_get_n(li,0),               "0th item is null");
  onex_assert_equal(item_to_text(list_get_n(li,1),buf,128), "y", "1st item is y");
  onex_assert_equal(item_to_text(list_get_n(li,2),buf,128), "3", "2nd item is 3");
  onex_assert(                  !list_get_n(li,3),               "3rd item is null");

                                 list_set_n(li,2,(item*)value_new("5"));

  onex_assert(                   list_size( li)==2,                      "size should still be 2");
  onex_assert_equal(item_to_text(list_get_n(li,2),buf,128), "5",         "2nd item is now 5");

  onex_assert(                  !list_set_n(li,3,(item*)value_new("+")), "can't set item out of range");

  onex_assert(                   list_size( li)==2,                      "size should still be 2");

  onex_assert(                   list_add(  li,(item*)value_new("N")),   "can add a 3rd item");
  onex_assert(                   list_size( li)==3,                      "size should be 3");

  onex_assert(                   list_set_n(li,3,(item*)value_new("+")), "now can set 3rd item");

  onex_assert(                  !list_add(  li,(item*)value_new("N")),   "shouldn't be able to add a fourth item");
  onex_assert(                   list_size( li)==3,                      "size should still be 3");

  onex_assert_equal(item_to_text(li, buf, 128), "y 5 +", "serialise to string works");

  onex_assert(                   list_del_n(li,2),                       "can delete 2nd item");
  onex_assert(                   list_size( li)==2,                      "size now 2");

  onex_assert_equal(item_to_text(li, buf, 128), "y +", "serialise to string works");

  onex_assert(                   list_del_n(li,2),                       "can delete 2nd item");
  onex_assert(                   list_size( li)==1,                      "size now 1");

  onex_assert_equal(item_to_text(li, buf, 128), "y", "serialise to string works");

  onex_assert(                   list_del_n(li,1),                       "can delete 1st item");
  onex_assert(                   list_size( li)==0,                      "size now 0");

  onex_assert_equal(item_to_text(li, buf, 128), "", "serialise to string works");

  onex_assert(                   list_add(  li,(item*)value_new("*")),   "can add an item back");
  onex_assert(                   list_size( li)==1,                      "size should be 1");

                    list_log(li);
}

void run_list_tests()
{
  log_write("------list tests-----\n");

  test_list();
}

