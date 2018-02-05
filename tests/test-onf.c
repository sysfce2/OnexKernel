
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <onex-kernel/log.h>
#include <assert.h>
#include <onf.h>

// ---------------------------------------------------------------------------------

bool evaluate_setup_called=false;

bool evaluate_setup(object* n1)
{
  evaluate_setup_called=true;
  return true;
}

void test_object_set_up()
{
  object* nr=object_new(0, "random uid", 0, 4);
  char* random_uid_1 = object_property(nr, "UID");
  nr=object_new(0, "random uid", 0, 4);
  char* random_uid_2 = object_property(nr, "UID");

  onex_assert(      object_property_is_list(nr, "is"),             "property 'is' is a list");
  onex_assert_equal(object_property(        nr, "is:1"), "random", "object_new parses 'is' first list item" );
  onex_assert(      object_property_is(     nr, "is:2",  "uid"),   "object_new parses 'is' second list item" );

  onex_assert(      strlen(random_uid_1)==23,                "UID generation returns long string");
  onex_assert(      strlen(random_uid_2)==23,                "UID generation returns long string");
  onex_assert(      strcmp(random_uid_1, random_uid_2),      "UID generation creates unique UIDs");

  object* n1=object_new("uid-1", "setup", evaluate_setup, 4);

  onex_assert(      n1,                                      "object_new created an object");
  onex_assert(      onex_get_from_cache("uid-1")==n1,        "onex_get_from_cache can find uid-1");
  onex_assert(     !onex_get_from_cache("uid-2"),            "onex_get_from_cache can't find uid-2");

  onex_assert(      object_property_is(n1, "UID", "uid-1"),  "object_new saves uid as a (virtual) property");
  onex_assert_equal(object_property(   n1, "is"), "setup",   "object_property returns 'is'" );
  onex_assert(      object_property_is(n1, "is",  "setup"),  "object_property_is says 'is' is 'setup'");
  onex_assert(      object_property_is_value(n1, "is"),      "property 'is' is a value");

                    object_property_set(           n1, "state", "good");
  onex_assert(      object_property_is(            n1, "state", "good"), "object_property_is says 'state' is 'good'");
  onex_assert(      object_property_is_value(      n1, "state"),         "property 'state' is a value");
  onex_assert(     !object_property_is_list(       n1, "state"),         "property 'state' is not a list");
  onex_assert(     !object_property_is_properties( n1, "state"),         "property 'state' is not a properties");

                    object_property_add(           n1, "state", "mostly");
  onex_assert(     !object_property_is(            n1, "state", "good"), "object_property_is says 'state' is not all 'good'");
  onex_assert(     !object_property_is_value(      n1, "state"),         "property 'state' is not a value");
  onex_assert(      object_property_is_list(       n1, "state"),         "property 'state' is now a list");
  onex_assert(     !object_property_is_properties( n1, "state"),         "property 'state' is not a properties");

  onex_assert(      object_property_add(     n1, "1", "a"),    "can add new property");
  onex_assert(      object_property_is_value(n1, "1"),         "property '1' is a value");
  onex_assert(      object_property_size(    n1, "1")==1,      "one item property");
  onex_assert_equal(object_property_value(   n1, "1", 1), "a", "1st value in list is 'a'");
  onex_assert_equal(object_property(         n1, "1:1"), "a",  "1st value in list is 'a'");
  onex_assert(     !object_property(         n1, "1:2"),       "2nd value in list is null");
  onex_assert(     !object_property(         n1, "1:0"),       "0th value in list is null");

  onex_assert(      object_property_add(     n1, "1", "b"),    "can add another");
  onex_assert(      object_property_is_list( n1, "1"),         "property '1' is now a list");
  onex_assert(      object_property_size(    n1, "1")==2,      "two items in the list");
  onex_assert_equal(object_property_value(   n1, "1", 2), "b", "2nd value in list is 'b' by index");
  onex_assert(     !object_property(         n1, "1:0"),       "0th value in list is null");
  onex_assert_equal(object_property(         n1, "1:1"), "a",  "1st value in list is 'a'");
  onex_assert_equal(object_property(         n1, "1:2"), "b",  "2nd value in list is 'b'");
  onex_assert(     !object_property(         n1, "1:3"),       "3rd value in list is null");
  onex_assert(     !object_property(         n1, "1:four"),    "four-th value in list is null");

  onex_assert(      object_property_add(     n1, "1", "c"),    "can add a third to existing list");
  onex_assert(      object_property_is_list( n1, "1"),         "property '1' is still a list");
  onex_assert(      object_property_size(    n1, "1")==3,      "three items in the list");
  onex_assert_equal(object_property_value(   n1, "1", 3), "c", "3rd value in list is 'c'");
  onex_assert_equal(object_property(         n1, "1:1"), "a",  "1st value in list is 'a'");
  onex_assert_equal(object_property(         n1, "1:2"), "b",  "2nd value in list is 'b'");
  onex_assert_equal(object_property(         n1, "1:3"), "c",  "3rd value in list is 'c'");
  onex_assert(     !object_property(         n1, "1:4"),       "4th value in list is null");

  onex_assert(      object_property_set(     n1, "1:2", "B"),  "can set 2nd value in list");
  onex_assert_equal(object_property(         n1, "1:2"), "B",  "2nd value in list is 'B'");
  onex_assert(     !object_property_set(     n1, "1:4", "X"),  "can't set 4th value in list");

  onex_assert(      object_property_set(     n1, "1:2", ""),   "can set 2nd value in list to empty to delete");
  onex_assert(      object_property_size(    n1, "1")==2,      "now two items in the list");
  onex_assert(      object_property_is_list( n1, "1"),         "property '1' is still a list");

  onex_assert(      object_property_set(     n1, "1:2", 0),    "can set 2nd value in list to null to delete");
  onex_assert(      object_property_size(    n1, "1")==1,      "now one item in the list");
  onex_assert(      object_property_is_value(n1, "1"),         "property '1' is now a value");
  onex_assert_equal(object_property(         n1, "1"), "a",    "property '1' is 'a'");

  onex_assert(      object_property_set(     n1, "1:1", ""),   "can set 1st value in 'list' to empty to delete");
  onex_assert(      object_property_size(    n1, "1")==0,      "now no property");
  onex_assert(     !object_property(         n1, "1"),         "now no such property");
  onex_assert(     !object_property_is_value(n1, "1"),         "property '1' is not a value");

  onex_assert(      object_property_set(     n1, "1", "a"),    "can set property back");
  onex_assert(      object_property_add(     n1, "1", "c"),    "and another");

  onex_assert(      object_property_set(n1, "2", "ok"),      "can set 2 more properties");
  onex_assert(      object_property_is_value(n1, "2"),       "property '1' is a value");

  onex_assert(     !object_property_set(n1, "3", "not ok"),  "can't set 3 more properties");
  onex_assert(     !object_property_set(n1, "4", "not ok"),  "can't set 4 more properties");

  onex_assert(     !object_property(         n1, "4"),       "empty property returns null");
  onex_assert(      object_property_is(      n1, "4", ""),   "empty property is empty");
  onex_assert(     !object_property_is_value(n1, "4"),       "empty property is not a value");

  onex_assert(      object_property_set(     n1, "2", ""),   "can set property to empty");
  onex_assert(     !object_property(         n1, "2"),       "empty property returns null");
  onex_assert(      object_property_is(      n1, "2", 0),    "empty property is empty");
  onex_assert(     !object_property_is_value(n1, "2"),       "empty property is not a value");
  onex_assert(      object_property_set(     n1, "2", "ok"), "can set empty property back");

  onex_assert(      object_property_set(     n1, "2", 0),    "can set property to null");
  onex_assert(     !object_property(         n1, "2"),       "empty property returns null");
  onex_assert(      object_property_is(      n1, "2", ""),   "empty property is empty");
  onex_assert(     !object_property_is_value(n1, "2"),       "empty property is not a value");

  onex_assert(      object_property_set(     n1, "2", "ok m8"), "can set property to two items");
  onex_assert(      object_property_is_list( n1, "2"),          "property '2' is a list");
  onex_assert(      object_property_is(      n1, "2:1", "ok"),  "list items are correct");
  onex_assert(      object_property_is(      n1, "2:2", "m8"),  "list items are correct");

  onex_assert(      object_property_is_properties(n1, ":"),  "root path is a properties");
  onex_assert(      object_property_size( n1, ":")==4,      "there are four properties");

  onex_assert_equal(object_property_key(  n1, ":", 1), "is",    "key of 1st item is 'is'");
  onex_assert_equal(object_property_value(n1, ":", 1), "setup", "val of 1st item is 'setup'");
  onex_assert_equal(object_property_key(  n1, ":", 2), "state", "key of 2nd item is 'state'");
  onex_assert(     !object_property_value(n1, ":", 2),          "val of 2nd item is not just a value");
  onex_assert_equal(object_property_key(  n1, ":", 3), "1",     "key of 3rd item is '1'");
  onex_assert(     !object_property_value(n1, ":", 3),          "val of 3rd item is not just a value");
  onex_assert_equal(object_property_key(  n1, ":", 4), "2",     "key of 4th item is '2'");
  onex_assert(     !object_property_value(n1, ":", 4),          "val of 4th item is not just a value");

  onex_assert(     !object_property_key(n1, ":", 5),            "key of 5th item is 0");
  onex_assert(     !object_property_value(n1, ":", 5),          "val of 5th item is 0");
  onex_assert(     !object_property_key(n1, ":", 0),            "key of 0th item is 0");
  onex_assert(     !object_property_value(n1, ":", 0),          "val of 0th item is 0");
}

// ---------------------------------------------------------------------------------

bool evaluate_local_state_2_called=false;

bool evaluate_local_state_2(object* n2)
{
  evaluate_local_state_2_called=true;
  return true;
}

bool evaluate_local_state_3_called=false;

#include <items.h>

bool evaluate_local_state_3(object* n3)
{
  evaluate_local_state_3_called=true;
  onex_assert(     !object_property(       n3, "is:foo"),                     "can't find is:foo");
  onex_assert(      object_property_is(    n3, "n2:UID",  "uid-2"),           "can see UID of local object immediately");
  onex_assert(      object_property_is(    n3, "n2:is",   "local-state"),     "can see 'is' of local object immediately");
  onex_assert(      object_property_is(    n3, "n2:state", "good"),           "can see state prop of local object immediately");
  onex_assert(      object_property_is_value(     n3, "self:n2"),             "property 'n2' is a value (uid)");
  onex_assert(      object_property_is_properties(n3, "self:n2:"),            "property 'n2:' is a properties");
  onex_assert(      object_property_size(  n3, "self:self:n2:")==2,           "there are two properties at n3:n2:");
  onex_assert(      object_property_is(    n3, "self:UID", "uid-3"),          "can see through link to self");
  onex_assert(     !object_property(       n3, "n2:foo"),                     "can't find n2:foo");
  onex_assert_equal(object_property(       n3, "n*:1:UID"), "uid-1",          "n*:1:UID is uid-1");
  onex_assert_equal(object_property(       n3, "n*:2:UID"), "uid-2",          "n*:2:UID is uid-2");
  onex_assert_equal(object_property(       n3, "n*:3:UID"), "uid-3",          "n*:3:UID is uid-3");
  onex_assert(     !object_property(       n3, "n*:4:UID"),                   "n*:4:UID is null");
  onex_assert_equal(object_property(       n3, "n*:1"), "uid-1",              "n*:1 is uid-1");
  onex_assert_equal(object_property(       n3, "n*:2"), "uid-2",              "n*:2 is uid-2");
  onex_assert_equal(object_property_key(   n3, ":", 2), "n2",                 "key of 2nd item is 'n2'");
  onex_assert_equal(object_property_value( n3, ":", 2), "uid-2",              "val of 1st item is 'uid-2'");
  onex_assert_equal(object_property_key(   n3, "n2:", 1), "is",               "key of 1st item in n2 is 'is'");
  onex_assert_equal(object_property_value( n3, "n2:", 1), "local-state",      "val of 1st item in n2 is 'local-state'");
  onex_assert_equal(object_property_key(   n3, "n2:", 2), "state",            "key of 2nd item in n2 is 'state'");
  onex_assert_equal(object_property_value( n3, "n2:", 2), "good",             "val of 2nd item in n2 is 'good'");
  onex_assert_equal(object_property_key(   n3, "self:self:n2:", 2), "state",  "key of 2nd item in n2 is 'state', via self");
  onex_assert_equal(object_property_value( n3, "self:n2:", 2), "good",        "val of 2nd item in n2 is 'good', via self");
  return true;
}

void test_local_state()
{
  object* n2=object_new("uid-2", "local-state", evaluate_local_state_2, 4);
  object* n3=object_new("uid-3", "local-state", evaluate_local_state_3, 4);

  onex_assert(      onex_get_from_cache("uid-2")==n2,   "onex_get_from_cache can find uid-2");
  onex_assert(      onex_get_from_cache("uid-3")==n3,   "onex_get_from_cache can find uid-3");

  onex_assert(      object_property_set(n2, "state", "good"),  "can add 'state'");
  onex_assert(      object_property_set(n3, "n2",    "uid-2"), "can add 'n2'");
  onex_assert(      object_property_set(n3, "self",  "uid-3"), "can add 'self'");

  onex_assert(      object_property_set(    n3, "n*", "uid-1"),    "can set uid-1 in n*");
  onex_assert(      object_property_add(    n3, "n*", "uid-2"),    "can add uid-2 to n*");
  onex_assert(      object_property_add(    n3, "n*", "uid-3"),    "can add uid-3 to n*");
  onex_assert(     !object_property_add(    n3, "n*", ""),         "can't add empty to n*");
  onex_assert(     !object_property_add(    n3, "n*", 0),          "can't add empty to n*");
  onex_assert(      object_property_add(    n3, "n*", "uid-4"),    "can add uid-4 to n*");
  onex_assert(      object_property_add(    n3, "n*", "uid-5"),    "can add uid-5 to n*");
//onex_assert(     !object_property_add(    n3, "n*", "uid-6"),    "can't add uid-6 to n*");
  onex_assert(      object_property_is_list(n3, "n*"),             "n* is a list");
  onex_assert(      object_property_size(   n3, "n*")==5,          "there are 5 items in n*");
  onex_assert_equal(object_property_value(  n3, "n*", 1), "uid-1", "1st item in n* list is uid-1");
  onex_assert_equal(object_property_value(  n3, "n*", 2), "uid-2", "2nd item in n* list is uid-2");
  onex_assert_equal(object_property_value(  n3, "n*", 3), "uid-3", "3rd item in n* list is uid-3");
  onex_assert_equal(object_property_value(  n3, "n*", 4), "uid-4", "4th item in n* list is uid-4");
}

// ---------------------------------------------------------------------------------

bool evaluate_local_notify_2_called=false;

bool evaluate_local_notify_2(object* n2)
{
  evaluate_local_notify_2_called=true;
  object_property_set(n2, "state", "better");
  return true;
}

bool evaluate_local_notify_3_called=false;

bool evaluate_local_notify_3(object* n3)
{
  evaluate_local_notify_3_called=true;
  onex_assert(      object_property_is(n3, "n2:state", "better"),      "n3/uid-3 can see state update");
  return true;
}

void test_local_notify()
{
  object* n2=onex_get_from_cache("uid-2");
  object* n3=onex_get_from_cache("uid-3");

  onex_assert(      object_property_is(n3, "n2",       "uid-2"),   "uid-3 links to uid-2");
  onex_assert(      object_property_is(n3, "n2:state", "good"),    "can still see through link");

  object_set_evaluator(n2, evaluate_local_notify_2);
  object_set_evaluator(n3, evaluate_local_notify_3);

                    onex_run_evaluators(n2);

  onex_assert(      object_property_is(n2, "state", "better"),  "can see state update");
}

// ---------------------------------------------------------------------------------

#define TEXTBUFFLEN 128
char textbuff[TEXTBUFFLEN];

void test_to_text()
{
  object* n1=onex_get_from_cache("uid-1");
  object* n2=onex_get_from_cache("uid-2");
  object* n3=onex_get_from_cache("uid-3");

  onex_assert_equal(object_to_text(n1,textbuff,TEXTBUFFLEN), "UID: uid-1 Notify: uid-3 is: setup state: good mostly 1: a c 2: ok m8",                            "converts uid-1 to correct text");
  onex_assert_equal(object_to_text(n2,textbuff,TEXTBUFFLEN), "UID: uid-2 Notify: uid-3 is: local-state state: better",                                           "converts uid-2 to correct text");
  onex_assert_equal(object_to_text(n3,textbuff,TEXTBUFFLEN), "UID: uid-3 Notify: uid-3 is: local-state n2: uid-2 self: uid-3 n*: uid-1 uid-2 uid-3 uid-4 uid-5", "converts uid-3 to correct text");
}

// ---------------------------------------------------------------------------------

bool evaluate_remote_notify_1_called=false;

bool evaluate_remote_notify_1(object* n1)
{
  evaluate_remote_notify_1_called=true;
  return true;
}

bool evaluate_remote_notify_2_called=false;

bool evaluate_remote_notify_2(object* n2)
{
  evaluate_remote_notify_2_called=true;
  return true;
}

void test_from_text()
{
  object* n1=onex_get_from_cache("uid-1");
  object* n2=onex_get_from_cache("uid-2");
  object_set_evaluator(n1, evaluate_remote_notify_1);
  object_set_evaluator(n2, evaluate_remote_notify_2);

  char* text="UID: uid-4 Notify: uid-1 uid-2 is: remote state n3: uid-3";
  object* n4=object_new_from(strdup(text));

  onex_assert(      !!n4,                                         "input text was parsed into an object");
  if(!n4) return;

  onex_assert_equal(object_to_text(n4,textbuff,TEXTBUFFLEN),text, "gives same text back from reconstruction");

  onex_assert(      object_property_is( n4, "UID", "uid-4"),       "object_new_from parses uid");
  onex_assert_equal(object_property(    n4, "is:1"), "remote",     "object_new_from parses 'is' first list item" );
  onex_assert_equal(object_property(    n4, "is:2"), "state",      "object_new_from parses 'is' second list item" );
  onex_assert_equal(object_property(    n4, "n3"), "uid-3",        "object_new_from parses properties" );

                    object_property_set(n4, "state", "good");
  onex_assert(      object_property_is( n4, "state", "good"),      "object_new_from creates usable object");
}

// ---------------------------------------------------------------------------------

void run_onf_tests()
{
  log_write("------ONF tests-----------\n");

  onex_init();

  test_object_set_up();
  test_local_state();

  onex_loop();

  onex_assert(      evaluate_setup_called,             "evaluate_setup was called");
  onex_assert(      evaluate_local_state_2_called,     "evaluate_local_state_2 was called");
  onex_assert(      evaluate_local_state_3_called,     "evaluate_local_state_3 was called");

  test_local_notify();
  test_to_text();
  test_from_text();

  onex_loop();

  onex_assert(      evaluate_local_notify_2_called,     "evaluate_local_notify_2 was called");
  onex_assert(      evaluate_local_notify_3_called,     "evaluate_local_notify_3 was called");
  onex_assert(      evaluate_remote_notify_1_called,    "evaluate_remote_notify_1 was called");
  onex_assert(      evaluate_remote_notify_2_called,    "evaluate_remote_notify_2 was called");

  onex_show_cache();
}

// ---------------------------------------------------------------------------------

