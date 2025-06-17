#ifndef ONN_H
#define ONN_H

#include <stdbool.h>
#include <stdint.h>

#include <items.h> // only for list

// -----------------------------------------------------------------------

#define MAX_UID_LEN 128

#if defined(NRF5)

#define MAX_LIST_SIZE  32
#define MAX_TEXT_LEN 1024
#define MAX_OBJECTS 64
#define MAX_TO_NOTIFY 64
#define MAX_OBJECT_SIZE 16

#else

#define MAX_LIST_SIZE 256
#define MAX_TEXT_LEN 2048
#define MAX_OBJECTS 4096
#define MAX_TO_NOTIFY 1024
#define MAX_OBJECT_SIZE 32

#endif

#define OBJECT_MAX_NOTIFIES 16 // REVISIT
#define OBJECT_MAX_DEVICES   8 // REVISIT

/** The actual Object. */
typedef struct object object;

/** this is the sig for your Object evaluator callback. */
typedef bool (*onex_evaluator)(struct object* n, void* data);

/** value struct for passing around OBS: message data. */
typedef struct {
  char* uid;
  char* dev;
} observe;

// --------------------------------------------------------------------

/** create a new local Object.
    uid:       if any (will generate one otherwise)
    evaluator: name of callback function, set in onex_set_evaluators(), to get and set values on this Object
    is:        string type name (e.g. "light", "button")
    max_size:  max number of properties
 */
object* object_new(char* uid, char* evaluator, char* is, uint8_t max_size);

/** create a new local Object from text. */
object* object_new_from(char* text, uint8_t max_size);

/** re-set evaluator callback. */
void object_set_evaluator(object* n, char* evaluator);

/** set/unset flag to kick the Object on restart.
  * Cache: keep-active */
void object_set_cache(object* n, char* cache);

/** read Cache: (keep-active) flag. */
char* object_get_cache(object* n);

/** set flag to determine persistence policy.
  * Persist: async
  * can be "none", "async" or "sync" (one day). */
void object_set_persist(object* n, char* persist);

/** read persist prop */
char* object_get_persist(object* n);

/** set property value. only use inside an evaluator for 'n' */
bool  object_property_set(object* n, char* path, char* val);

// REVISIT: object_property_add to ensure in the set
// object_property_append to add to end of list

/** append property value onto end of any list. only use inside an evaluator for 'n' */
bool  object_property_add(object* n, char* path, char* val);
#define object_property_append(n, path, val) object_property_add(n, path, val)

/** insert property value into any list or prepend. only use inside an evaluator for 'n' */
bool  object_property_insert(object* n, char* path, char* val);
#define object_property_prepend(n, path, val) object_property_insert(n, path, val)

/** set property values into list. only use inside an evaluator for 'n'. must finish list with 0! */
bool  object_property_set_list(object* n, char* path, ... /* char* val, ..., 0 */); // REVISIT: do array trick

/** set property values into list from printf format. only use inside an evaluator for 'n'. */
bool  object_property_set_fmt(object* n, char* path, char* fmt, ... /* <any> val, ... */);

/** set property value at path and index into list. only use inside an evaluator for 'n'. */
bool object_property_set_n(object* n, char* path, uint16_t index, char* val);

/** add property values to list, or make a list. only use inside an evaluator for 'n'. must finish list with 0! */
bool  object_property_add_list(object* n, char* path, ... /* char* val, ..., 0 */); // REVISIT: do array trick

/** ---------------------- */

/** return single property value or nothing if it's a list. */
char* object_property(object* n, char* path);

/** return single property value or nothing if it's a list; two-part path list */
char* object_pathpair(object* n, char* path1, char* path2);

/** return property value or nothing if it's a list; don't observe any sub-objects on the path. */
char* object_property_peek(object* n, char* path);

/** return whether property at path is a single value and it matches supplied string. */
bool  object_property_is(object* n, char* path, char* expected);

/** return whether property at path is a single value and it matches supplied string; two-part path list */
bool  object_pathpair_is(object* n, char* path1, char* path2, char* expected);

/** return whether property at path is a single value and it matches supplied string; don't observe any sub-objects on the path. */
bool  object_property_is_peek(object* n, char* path, char* expected);

/** return whether property at path is a single value or a list and supplied string found. */
bool  object_property_contains(object* n, char* path, char* expected);

/** return whether property at path is a single value or a list and supplied string found; two-part path list */
bool  object_pathpair_contains(object* n, char* path1, char* path2, char* expected);

/** return whether property at path is a single value or a list and supplied string found; don't observe any sub-objects on the path. */
bool  object_property_contains_peek(object* n, char* path, char* expected);

/** return 32-bit signed integer parsed from string; returns 0 for all failure cases. */
int32_t object_property_int32(object* n, char* path);

/** return 32-bit signed integer parsed from string; returns 0 for all failure cases; two-part path list */
int32_t object_pathpair_int32(object* n, char* path1, char* path2);

/** return how many items there are at a path. */
uint16_t object_property_length(object* n, char* path);

/** return how many items there are at a path; two-part path list */
uint16_t object_pathpair_length(object* n, char* path1, char* path2);

/** return property value at path and index into list. */
char* object_property_get_n(object* n, char* path, uint16_t index);

/** return property value at path and index into list; two-part path list */
char* object_pathpair_get_n(object* n, char* path1, char* path2, uint16_t index);

/** return how many properties there are at a path, or -1 if it's not a properties there. */
int16_t object_property_size(object* n, char* path);

/** return property key at path and index into properties. */
char* object_property_key(object* n, char* path, uint16_t index);

/** return property value at path and index into properties. */
char* object_property_val(object* n, char* path, uint16_t index);

/** as object_property_key but escape any colons. */
char* object_property_key_esc(object* n, char* path, uint16_t index, char* keyesc, uint8_t len);

/* for style argument of object_to_text; bigger numbers more verbose. */
#define OBJECT_TO_TEXT_NETWORK 1
#define OBJECT_TO_TEXT_PERSIST 2
#define OBJECT_TO_TEXT_LOG     3

/** Object to text; supply your own buffer, b, of length s. */
char* object_to_text(object* n, char* b, uint16_t s, int style);

/** Object uid to text; supply your own buffer, b, of length s. */
char* object_uid_to_text(char* uid, char* b, uint16_t s, int style);

/** make an OBS: string in this buffer. */
char* observe_uid_to_text(char* uid, char* b, uint16_t s);

/** OBS from text. */
observe observe_from_text(char* u);

/** log out the Object */
void object_log(object* n);

/** true if it's a UID. */
bool is_uid(char* uid);

/** true if it's a local Object (hosted/animated in this Onex). */
bool is_local(char* uid);

/** true if it's a local Object (hosted/animated in this Onex). */
bool object_is_local(object* o);

/** true if it's a remote Object. */
bool object_is_remote(object* o);

bool is_shell(char* uid);

bool object_is_shell(object* o);

bool object_is_device(object* o);

bool object_is_local_device(object* o);

bool object_is_remote_device(object* o);

/** free Object and properties */
void object_free(object* n);

// --------------------------------------------------------------------

/** the device Object for this device. */
extern object* onex_device_object;

/** set things up. example config:
               {
                 dbpath: ./my.ondb
                 channels: serial ipv6
                 ipv6_groups: ff12::1234 ff12::4321
               }
 */
void onex_init(properties* config);

/** call when you want your evaluator run
    can be used to set some state within a transaction etc with given data arg
    which is passed to each evaluator in the chain. */
void onex_run_evaluators(char* uid, void* data);

/** set the evaluator mapping from name to evaluator function chain. must finish list with 0! */
void onex_set_evaluators(char* name, ... /* onex_evaluator evaluator, ..., 0 */); // REVISIT: do array trick

/** call this to give CPU to Onex.
    returns true if the main loop cannot sleep yet */
bool onex_loop();

/** log out all the objects in the cache. */
void onex_show_cache();

/** log out all the notifies pending. */
void onex_show_notify();

/** remove Object from the cache. */
void onex_un_cache(char* uid);

// --------------------------------------------------------------------

/** return property value, or space-separated list of values, excluding uids and sub-properties. */
/** don't use this! it's nonsense and clutters the value cache */
char* object_property_values(object* n, char* path);

/** get the given Object from the cache. */
/** don't use this! can't just grab objects you like in this way. */
object* onex_get_from_cache(char* uid);

// --------------------------------------------------------------------

#endif
