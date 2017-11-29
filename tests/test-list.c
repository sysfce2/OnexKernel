
#include <stdio.h>
#include <assert.h>
#include <onex-kernel/log.h>
#include <items.h>

// ---------------------------------------------------------------------------------

void test_list()
{
  list* li=list_new(3);

                    list_add(li,"y");
                    list_add(li,"3");

  onex_assert(      list_size(li)==2,     "size should be 2");
  onex_assert(     !list_get(li,-1),      "-1th item is null");
  onex_assert(     !list_get(li,0),       "0th item is null");
  onex_assert_equal(list_get(li,1), "y",  "1st item is y");
  onex_assert_equal(list_get(li,2), "3",  "2nd item is 3");
  onex_assert(     !list_get(li,3),       "3rd item is null");

                    list_set(li,2,"5");

  onex_assert(      list_size(li)==2,     "size should still be 2");
  onex_assert_equal(list_get(li,2), "5",  "2nd item is now 5");

  onex_assert(     !list_set(li,3,"+"),   "can't set item out of range");

  onex_assert(      list_size(li)==2,     "size should still be 2");

  onex_assert(      list_add(li,"N"),     "can add a 3rd item");
  onex_assert(      list_size(li)==3,     "size should be 3");

  onex_assert(      list_set(li,3,"+"),   "now can set 3rd item");

  onex_assert(     !list_add(li,"N"),     "shouldn't be able to add a fourth item");
  onex_assert(      list_size(li)==3,     "size should still be 3");

  char buf[128];
  onex_assert_equal(item_to_text(li, buf, 128), "y 5 +", "serialise to string works");

                    list_log(li);
}

void run_list_tests()
{
  log_write("------list tests-----\n");

  test_list();
}

