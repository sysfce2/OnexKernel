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
object* object_new_from(char* text);

/** get the given Object from the cache. */
object* object_get_from_cache(char* uid);

/** re-set evaluator callback. */
void object_set_evaluator(object* n, onex_evaluator evaluator);

/** return property value. */
char* object_property(object* n, char* path);

/** return how many properties there are at a path, -1 if not a properties. */
uint8_t object_property_size(object* n, char* path);

/** return property key at path and index. */
char* object_property_key(object* n, char* path, uint8_t index);

/** return property value at path and index. */
char* object_property_value(object* n, char* path, uint8_t index);

/** return whether property at path is a value. */
bool  object_property_is_value(object* n, char* path);

/** return whether property at path is a list. */
bool  object_property_is_list(object* n, char* path);

/** return whether property at path is a (sub)properties. */
bool  object_property_is_properties(object* n, char* path);

/** return whether property at path is a value and it matches supplied string. */
bool  object_property_is(object* n, char* path, char* expected);

/** set property value. */
bool  object_property_set(object* n, char* path, char* value);

/** add property value to list, or make a list. */
bool  object_property_add(object* n, char* path, char* value);

/** to text. Supply your own buffer, b, of length s */
char* object_to_text(object* n, char* b, uint8_t s);

/** log out the object */
void object_log(object* n);

// --------------------------------------------------------------------

/** set things up like IPv6. */
void onex_init();

/** call when you want your evaluator run so you can set some state within a transaction. */
void onex_run_evaluators(object* n);

/** call this in loop() to give CPU to Objects. */
void onex_loop();

// --------------------------------------------------------------------

#endif
