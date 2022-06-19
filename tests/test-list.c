
#include <stdio.h>
#include <tests.h>
#include <onex-kernel/log.h>
#include <items.h>

// ---------------------------------------------------------------------------------

void test_list()
{
  char buf[32];

  list* li=list_new(3);
                                 list_add(  li, value_new("y"));
                                 list_add(  li, value_new("3"));

  onex_assert(                   list_size( li)==2,              "size should be 2");
  onex_assert(                  !list_get_n(li,-1),              "-1th item is null");
  onex_assert(                  !list_get_n(li,0),               "0th item is null");
  onex_assert_equal(item_to_text(list_get_n(li,1),buf,32), "y", "1st item is y");
  onex_assert_equal(item_to_text(list_get_n(li,2),buf,32), "3", "2nd item is 3");
  onex_assert(                  !list_get_n(li,3),               "3rd item is null");

                                 list_set_n(li,2,value_new("5"));

  onex_assert(                   list_size( li)==2,               "size should still be 2");
  onex_assert_equal(item_to_text(list_get_n(li,2),buf,32), "5",  "2nd item is now 5");

  onex_assert(                  !list_set_n(li,3,value_new("+")), "can't set item out of range");

  onex_assert(                   list_size( li)==2,               "size should still be 2");

  onex_assert(                   list_add(  li,value_new("N")),   "can add a 3rd item");
  onex_assert(                   list_size( li)==3,               "size should be 3");

  onex_assert(                   list_set_n(li,3,value_new("+")), "now can set 3rd item");

  onex_assert(                  !list_add(  li,value_new("N")),   "shouldn't be able to add a fourth item");
  onex_assert(                   list_size( li)==3,               "size should still be 3");

  onex_assert_equal_num(         list_find( li, (item*)value_new("y")), 1,          "y is found at location 1");
  onex_assert_equal_num(         list_find( li, (item*)value_new("5")), 2,          "5 is found at location 2");
  onex_assert_equal_num(         list_find( li, (item*)value_new("+")), 3,          "+ is found at location 3");
  onex_assert_equal_num(         list_find( li, (item*)value_new("X")), 0,          "X is not found ");

  onex_assert_equal(item_to_text(li, buf, 32), "y 5 +", "serialise to string works");

  onex_assert(                   list_del_n(li,2),                "can delete 2nd item");
  onex_assert(                   list_size( li)==2,               "size now 2");

  onex_assert_equal(item_to_text(li, buf, 32), "y +", "serialise to string works");

  list_log(li);

  onex_assert(                   list_del_n(li,2),                "can delete 2nd item");
  onex_assert(                   list_size( li)==1,               "size now 1");

  onex_assert_equal(item_to_text(li, buf, 32), "y", "serialise to string works");

  onex_assert(                   list_del_n(li,1),                "can delete 1st item");
  onex_assert(                   list_size( li)==0,               "size now 0");

  onex_assert_equal(item_to_text(li, buf, 32), "", "serialise to string works");

  onex_assert(                   list_add(  li,value_new("*")),   "can add an item back");
  onex_assert(                   list_size( li)==1,               "size should be 1");

  onex_assert(                   list_add(  li,value_new("X")),   "can add another 2nd item");
  onex_assert(                   list_size( li)==2,               "size should be 2");

                                 list_clear(li, true);
  onex_assert_equal_num(         list_size( li), 0,               "can clear the list");

  list_free(li, false);
  li=list_new_from(" one\n", 1);
  onex_assert(         list_size(li)==1,              "size should be 1");
  onex_assert_equal(item_to_text(li, buf, 32), "one", "can parse whitespace separated lists");

  list_free(li, false);
  li=list_new_from(" one\n ", 1);
  onex_assert(         list_size(li)==1,              "size should be 1");
  onex_assert_equal(item_to_text(li, buf, 32), "one", "can parse whitespace separated lists");

  list_free(li, false);
  li=list_new_from(" one\n two", 2);
  onex_assert(         list_size(li)==2,                  "size should be 2");
  onex_assert_equal(item_to_text(li, buf, 32), "one two", "can parse whitespace separated lists");

  list_free(li, false);
  li=list_new_from(" one\n two  three  \n ", 3);
  onex_assert(         list_size(li)==3,                        "size should be 3");
  onex_assert_equal(item_to_text(li, buf, 32), "one two three", "can parse whitespace separated lists");

  list_free(li, false);
}

void run_list_tests()
{
  log_write("------list tests-----\n");

  test_list();
}

