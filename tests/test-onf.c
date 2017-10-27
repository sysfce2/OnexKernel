
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
  object* n1=object_new("uid-1", "setup", evaluate_setup, 4);

  onex_assert(      n1,                                      "object_new created an object");
  onex_assert(      object_get_from_cache("uid-1")==n1,      "object_get_from_cache can find uid-1");
  onex_assert(     !object_get_from_cache("uid-2"),          "object_get_from_cache can't find uid-2");

  onex_assert(      object_property_is(n1, "UID", "uid-1"),  "object_new saves uid as a (virtual) property");
  onex_assert_equal(object_property(   n1, "is"), "setup",   "object_property returns 'is'" );
  onex_assert(      object_property_is(n1, "is",  "setup"),  "object_property_is says 'is' is 'setup'");

                    object_property_set(n1, "state", "good");

  onex_assert(      object_property_is(n1, "state", "good"), "object_property_is says 'state' is 'good'");

  onex_assert(      object_property_set(n1, "1", "ok"),      "can set 1 more property");
  onex_assert(      object_property_set(n1, "2", "ok"),      "can set 2 more properties");
  onex_assert(     !object_property_set(n1, "3", "not ok"),  "can't set 3 more properties");
  onex_assert(     !object_property_set(n1, "4", "not ok"),  "can't set 4 more properties");

  onex_assert_equal(object_property_key(n1, 1), "is",        "key of 1st item is 'is'");
  onex_assert_equal(object_property_val(n1, 1), "setup",     "val of 1st item is 'setup'");
  onex_assert_equal(object_property_key(n1, 2), "state",     "key of 2nd item is 'state'");
  onex_assert_equal(object_property_val(n1, 2), "good",      "val of 2nd item is 'good'");
  onex_assert_equal(object_property_key(n1, 3), "1",         "key of 3rd item is '1'");
  onex_assert_equal(object_property_val(n1, 3), "ok",        "val of 3rd item is 'ok'");
  onex_assert_equal(object_property_key(n1, 4), "2",         "key of 4th item is '2'");
  onex_assert_equal(object_property_val(n1, 4), "ok",        "val of 4th item is 'ok'");
  onex_assert(     !object_property_key(n1, 5),              "key of 5th item is 0");
  onex_assert(     !object_property_val(n1, 5),              "val of 5th item is 0");
  onex_assert(     !object_property_key(n1, 0),              "key of 0th item is 0");
  onex_assert(     !object_property_val(n1, 0),              "val of 0th item is 0");

  onex_assert(      object_property_size(n1)==4,             "there are four properties");
}

// ---------------------------------------------------------------------------------

bool evaluate_local_state_2_called=false;

bool evaluate_local_state_2(object* n2)
{
  evaluate_local_state_2_called=true;
  return true;
}

bool evaluate_local_state_3_called=false;

bool evaluate_local_state_3(object* n3)
{
  evaluate_local_state_3_called=true;
  onex_assert(     !object_property(   n3, "is:foo"),                  "can't find is:foo");
  onex_assert(      object_property_is(n3, "n2:UID",  "uid-2"),        "can see UID of local object immediately");
  onex_assert(      object_property_is(n3, "n2:is",   "local state"),  "can see 'is' of local object immediately");
  onex_assert(      object_property_is(n3, "n2:state", "good"),        "can see state prop of local object immediately");
  onex_assert(      object_property_is(n3, "self:UID", "uid-3"),       "can see through link to self");
  onex_assert(     !object_property(   n3, "n2:foo"),                  "can't find n2:foo");
  onex_assert(     !object_property(   n3, "n4:UID"),                  "can't find n4:UID");
  return true;
}

void test_local_state()
{
  object* n2=object_new("uid-2", "local state", evaluate_local_state_2, 4);
  object* n3=object_new("uid-3", "local state", evaluate_local_state_3, 4);

  onex_assert(      object_get_from_cache("uid-2")==n2,   "object_get_from_cache can find uid-2");
  onex_assert(      object_get_from_cache("uid-3")==n3,   "object_get_from_cache can find uid-3");

                    object_property_set(n2, "state", "good");
                    object_property_set(n3, "n2",    "uid-2");
                    object_property_set(n3, "n4",    "uid-4");
                    object_property_set(n3, "self",  "uid-3");
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
  object* n2=object_get_from_cache("uid-2");
  object* n3=object_get_from_cache("uid-3");

  onex_assert(      object_property_is(n3, "n2",       "uid-2"),   "uid-3 links to uid-2");
  onex_assert(      object_property_is(n3, "n2:state", "good"),    "can still see through link");

  object_set_evaluator(n2, evaluate_local_notify_2);
  object_set_evaluator(n3, evaluate_local_notify_3);

                    onex_run_evaluators(n2);

  onex_assert(      object_property_is(n2, "state", "better"),  "can see state update");
}

// ---------------------------------------------------------------------------------

#define TEXTBUFFLEN 80
char textbuff[TEXTBUFFLEN];

void test_to_text()
{
  object* n1=object_get_from_cache("uid-1");
  object* n2=object_get_from_cache("uid-2");
  object* n3=object_get_from_cache("uid-3");

  onex_assert_equal(object_to_text(n1,textbuff,TEXTBUFFLEN), "UID: uid-1 is: setup state: good 1: ok 2: ok",                             "converts uid-1 to correct text");
  onex_assert_equal(object_to_text(n2,textbuff,TEXTBUFFLEN), "UID: uid-2 Notify: uid-3 is: local state state: better",                   "converts uid-2 to correct text");
  onex_assert_equal(object_to_text(n3,textbuff,TEXTBUFFLEN), "UID: uid-3 Notify: uid-3 is: local state n2: uid-2 n4: uid-4 self: uid-3", "converts uid-3 to correct text");
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
  object* n1=object_get_from_cache("uid-1");
  object* n2=object_get_from_cache("uid-2");
  object_set_evaluator(n1, evaluate_remote_notify_1);
  object_set_evaluator(n2, evaluate_remote_notify_2);

  char* text="UID: uid-4 Notify: uid-1 uid-2 is: remote state n3: uid-3";
  object* n4=object_new_from(strdup(text));

  onex_assert(      !!n4,                                         "input text was parsed into an object");
  if(!n4) return;

  onex_assert_equal(object_to_text(n4,textbuff,TEXTBUFFLEN),text, "gives same text back from reconstruction");

  onex_assert(      object_property_is( n4, "UID", "uid-4"),       "object_new_from parses uid");
  onex_assert_equal(object_property(    n4, "is"), "remote state", "object_new_from parses 'is'" );
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
}

// ---------------------------------------------------------------------------------

