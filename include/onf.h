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
    uid:       if any (will generate one otherwise)
    evaluator: name of callback function, set in onex_set_evaluator(), to get and set values on this Object
    is:        string type name (e.g. "light", "button")
    max_size:  max number of properties - for embedded use
 */
object* object_new(char* uid, char* evaluator, char* is, uint8_t max_size);

/** create a new Object from text. */
object* object_new_from(char* text, uint8_t max_size);

/** re-set evaluator callback. */
void object_set_evaluator(object* n, char* evaluator);

/** set/unset flag to kick the object on restart. */
void object_keep_active(object* n, bool keepactive);

/** read keep-active flag. */
bool object_is_keep_active(object* n);

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

/** return whether property at path is a single value or a list and supplied string found. */
bool  object_property_contains(object* n, char* path, char* expected);

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

/** free object, plus just the properties. */
void object_free(object* n);

/** object to text; supply your own buffer, b, of length s */
char* object_to_text(object* n, char* b, uint16_t s);

/** set run data. */
void object_set_run_data(object* n, int32_t data);

/** get run data. */
int32_t object_get_run_data(object* n);


/** log out the object */
void object_log(object* n);

/** true if it's a UID. */
bool is_uid(char* uid);

/** true if it's a local object (hosted/animated in this Onex). */
bool object_is_local(char* uid);

// --------------------------------------------------------------------

/** set things up. */
void onex_init(char* dbpath);

/** call when you want your evaluator run so you can set some state within a transaction;
    also runs given pre and post evaluators if supplied. */
void onex_run_evaluator(char* uid, onex_evaluator pre, onex_evaluator post);

/** set the evaluator mapping from name to function. */
void onex_set_evaluator(char* name, onex_evaluator evaluator);

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
