#ifndef ONF_H
#define ONF_H

#include <stdbool.h>
#include <stdint.h>

// -----------------------------------------------------------------------

#define OBJECT_MAX_NOTIFIES 4

/** The actual Object. */
typedef struct object object;

/** this is the sig for your object evaluator callback. */
typedef bool (*onex_evaluator)(struct object* n);

// --------------------------------------------------------------------

/** create a new Object.
    is:        string type name (e.g. "light", "button")
    uid:       if any (will generate one otherwise)
    evaluator: callback to get and set values on this Object
    max_size:  max number of properties - this is embedded!
 */
object* object_new(char* uid, char* is, onex_evaluator evaluator, uint8_t max_size);

/** create a new Object from text. */
object* object_new_from(char* text, onex_evaluator evaluator, uint8_t max_size);

/** re-set evaluator callback. */
void object_set_evaluator(object* n, onex_evaluator evaluator);

/** set property value. */
bool  object_property_set(object* n, char* path, char* val);

/** add property value to list, or make a list. */
bool  object_property_add(object* n, char* path, char* val);

/** return property value, or space-separated list of values, including uids and sub-properties. */
char* object_property(object* n, char* path);

/** return property value, or space-separated list of values, excluding uids and sub-properties. */
char* object_property_values(object* n, char* path);

/** return whether property at path is a single value and it matches supplied string. */
bool  object_property_is(object* n, char* path, char* expected);

/** return how many items there are at a path. */
uint16_t object_property_length(object* n, char* path);

/** return property value at path and index into list. */
char* object_property_get_n(object* n, char* path, uint8_t index);

/** return how many properties there are at a path, or -1 if it's not a properties there. */
int16_t object_property_size(object* n, char* path);

/** return property key at path and index into properties. */
char* object_property_key(object* n, char* path, uint16_t index);

/** return property value at path and index into properties. */
char* object_property_val(object* n, char* path, uint16_t index);

/** object to text; supply your own buffer, b, of length s */
char* object_to_text(object* n, char* b, uint16_t s);

/** log out the object */
void object_log(object* n);

/** true if it's a UID. */
bool is_uid(char* uid);

/** true if it's a local object (hosted/animated in this Onex). */
bool object_is_local(char* uid);

// --------------------------------------------------------------------

/** set things up. */
void onex_init();

/** call when you want your evaluator run so you can set some state within a transaction. */
void onex_run_evaluators(object* n);

/** call this to give CPU to Onex. */
void onex_loop();

/** get the given Object from the cache. */
object* onex_get_from_cache(char* uid);

/** log out all the objects in the cache. */
void onex_show_cache();

/** remove object from the cache. */
void onex_un_cache(char* uid);

// --------------------------------------------------------------------

#endif
