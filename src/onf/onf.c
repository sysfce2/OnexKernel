
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>

#include "onp.h"

#include <items.h>
#include <onex-kernel/time.h>
#include <onex-kernel/gpio.h>
#if defined(HAS_SERIAL)
#include <onex-kernel/serial.h>
#endif
#include <onex-kernel/log.h>
#include <onex-kernel/random.h>
#include <onf.h>

// ---------------------------------------------------------------------------------


#define MAX_UID_LEN 128
#if defined(NRF5)
#define MAX_LIST_SIZE 16
#define MAX_TEXT_LEN 1024
#define MAX_OBJECTS 64
#define MAX_OBJECT_SIZE 16
#else
#define MAX_LIST_SIZE 64
#define MAX_TEXT_LEN 2048
#define MAX_OBJECTS 4096
#define MAX_OBJECT_SIZE 32
#endif

// ---------------------------------------------------------------------------------

static value*      generate_uid();
static char*       get_key(char** p);
static char*       get_val(char** p);
static bool        add_to_cache(object* n);
static bool        add_to_cache_and_persist(object* n);
static object*     find_object(char* uid, object* n, bool observe);
static item*       property_item(object* n, char* path, object* t, bool observe);
static item*       nested_property_item(object* n, char* path, object* t, bool observe);
static bool        nested_property_set(object* n, char* path, char* val);
static bool        nested_property_delete(object* n, char* path);
static bool        set_value_or_list(object* n, char* key, char* val);
static bool        add_notify(object* o, value* notify);
static void        set_notifies(object* o, char* notify);
static void        save_and_notify(object* n);
static bool        has_notifies(object* o);
static void        show_notifies(object* o);
static object*     new_object(value* uid, char* evaluator, char* is, uint8_t max_size);
static object*     new_object_from(char* text, uint8_t max_size);
static object*     new_shell(value* uid, char* notify);
static bool        is_shell(object* o);
static void        run_evaluators(object* o, void* data, object* alerted);

static void        device_init();

static void        persistence_init(char* filename);
static void        persistence_loop();
static object*     persistence_get(char* uid);
static void        persistence_put(object* o);
static void        persistence_flush();
static void        scan_objects_text_for_keep_active();

// ---------------------------------

typedef struct object {
  value*      uid;
  value*      evaluator;
  properties* properties;
  value*      cache;
  value*      notify[OBJECT_MAX_NOTIFIES];
  value*      alerted;
  value*      devices;
  bool        running_evals;
  uint64_t    last_observe;
} object;

// ---------------------------------

object* onex_device_object=0;

// ---------------------------------

value* generate_uid()
{
  char b[24];
  snprintf(b, 24, "uid-%02x%02x-%02x%02x-%02x%02x-%02x%02x",
    random_ish_byte(), random_ish_byte(),
    random_ish_byte(), random_ish_byte(),
    random_ish_byte(), random_ish_byte(),
    random_ish_byte(), random_ish_byte()
  );
  return value_new(b);
}

bool is_uid(char* uid)
{
  return uid && !strncmp(uid,"uid-",4);
}

bool is_local(char* uid)
{
  object* o=onex_get_from_cache(uid);
  return o && !o->devices;
}

bool is_shell(object* o)
{
  return o->devices && value_is(o->devices, "shell") && !o->properties;
}

bool object_is_remote(object* o)
{
  return o && o->devices;
}

bool object_is_device(object* o)
{
  return o && object_property_contains(o, "is", "device");
}

bool object_is_remote_device(object* o)
{
  return object_is_device(o) && o->devices;
}

bool object_is_local_device(object* o)
{
  return object_is_device(o) && !o->devices;
}

object* object_new_from(char* text, uint8_t max_size)
{
  object* n=new_object_from(text, max_size);
  if(!n) return 0;
  char* uid=value_string(n->uid);
  if(onex_get_from_cache(uid)){ log_write("Attempt to create an object with UID %s that already exists\n", uid); return 0; }
  if(!add_to_cache_and_persist(n)) return 0;
  return n;
}

object* object_new(char* uid, char* evaluator, char* is, uint8_t max_size)
{
  if(onex_get_from_cache(uid)){ log_write("Attempt to create an object with UID %s that already exists\n", uid); return 0; }
  object* n=new_object(value_new(uid), evaluator, is, max_size);
  if(!add_to_cache_and_persist(n)) return 0;
  return n;
}

object* new_object(value* uid, char* evaluator, char* is, uint8_t max_size)
{
  object* n=(object*)calloc(1,sizeof(object));
  n->uid=uid? uid: generate_uid();
  n->properties=properties_new(max_size);
  if(is) set_value_or_list(n, "is", is);
  n->evaluator=value_new(evaluator);
  return n;
}

object* new_object_from(char* text, uint8_t max_size)
{
  size_t m=strlen(text)+1;
  char t[m]; memcpy(t, text, m); // TODO: presumably to allow read-only strings, but seems expensive and risky
  object* n=0;
  value* uid=0;
  value* evaluator=0;
  value* devices=0;
  value* cache=0;
  char* notify=0;
  char* p=t;
  while(true){
    char* key=get_key(&p); if(!key) break;            if(!*key){ free(key); object_free(n); n=0; break; }
    char* val=get_val(&p); if(!val || !*val){ free(key); if(val) free(val); object_free(n); n=0; break; }
    if(!strcmp(key,"UID")) uid=value_new(val);
    else
    if(!strcmp(key,"Eval")) evaluator=value_new(val);
    else
    if(!strcmp(key,"Devices")) devices=value_new(val);
    else
    if(!strcmp(key,"Cache")) cache=value_new(val);
    else
    if(!strcmp(key,"Notify")) notify=strdup(val);
    else {
      if(!n){
        n=new_object(uid, 0, 0, max_size);
        if(evaluator) n->evaluator=evaluator;
        if(devices) n->devices=devices;
        if(cache) n->cache=cache;
        if(notify){ set_notifies(n, notify); free(notify); }
      }
      if(!set_value_or_list(n, key, val)) break;
    }
    free(key); free(val);
  }
  return n;
}

object* new_shell(value* uid, char* notify)
{
  object* n=(object*)calloc(1,sizeof(object));
  n->uid=uid;
  n->properties=properties_new(MAX_OBJECT_SIZE);
  n->devices=value_new("shell");
  n->last_observe = 0;
  set_notifies(n, notify);
  return n;
}

void object_free(object* o)
{
  if(!o) return;
  item_free(o->properties);
  free(o);
}

char* get_key(char** p)
{
  while(isspace(**p)) (*p)++;
  if(!**p) return 0;
  char* s=strchr(*p, ' ');
  char* c=strstr(*p, ": ");
  if(s<c || !c) return 0;
  (*c)=0;
  char* r=strdup(*p);
  (*c)=':';
  (*p)=c+1;
  return r;
}

char* get_val(char** p)
{
  while(isspace(**p)) (*p)++;
  if(!**p) return 0;
  char* c=strstr(*p, ": ");
  while(c && *(c-1)=='\\') c=strstr(c+1, ": ");
  char* r=0;
  if(!c){
    char* s=strrchr(*p, 0);
    do s--; while(isspace(*s)); s++;
    (*s)=0;
    r=strdup(*p);
    (*p)+=strlen(*p);
  }
  else{
    (*c)=0;
    char* s=strrchr(*p, ' ');
    if(!s){ (*c)=':'; return 0; }
    do s--; while(isspace(*s)); s++;
    (*c)=':';
    (*s)=0;
    r=strdup(*p);
    (*s)=' ';
    (*p)=s+1;
  }
  unsigned int y=0;
  for(unsigned int x=0; x<strlen(r); x++){
    if(r[x]=='\\') continue;
    if(y!=x) r[y]=r[x];
    y++;
  }
  r[y]=0;
  return r;
}

void object_set_evaluator(object* n, char* evaluator)
{
  n->evaluator=value_new(evaluator);
  persistence_put(n);
}

void object_keep_active(object* n, bool keepactive)
{
  n->cache=keepactive? value_new("keep-active"): 0;
  persistence_put(n);
}

bool object_is_keep_active(object* n)
{
  return n->cache && value_is(n->cache, "keep-active");
}

// ------------------------------------------------------

static char* object_property_observe(object* n, char* path, bool observe)
{
  if(!n) return 0;
  if(!strcmp(path, "UID")) return value_string(n->uid);
  item* i=property_item(n,path,n,observe);
  if(i && i->type==ITEM_VALUE) return value_string((value*)i);
  return 0;
}

char* object_property(object* n, char* path)
{
  return object_property_observe(n, path, true);
}

char* object_property_peek(object* n, char* path)
{
  return object_property_observe(n, path, false);
}

char* object_property_values(object* n, char* path)
{
  item* i=property_item(n,path,n,true);
  if(i){
    if(i->type==ITEM_VALUE){
      char* v=value_string((value*)i);
      if(is_uid(v)) return 0;
      return v;
    }
    if(i->type==ITEM_LIST){
      char b[MAX_TEXT_LEN]; *b=0;
      int ln=0;
      int j; int sz=list_size((list*)i);
      for(j=1; j<=sz; j++){
        item* y=list_get_n((list*)i, j);
        if(y->type==ITEM_PROPERTIES) continue;
        if(y->type==ITEM_LIST) continue;
        if(y->type==ITEM_VALUE){
          char* v=value_string((value*)y);
          if(is_uid(v)) continue;
          ln+=strlen(value_to_text((value*)y, b+ln, MAX_TEXT_LEN-ln));
          if(ln>=MAX_TEXT_LEN) return 0;
          if(j!=sz) ln+=snprintf(b+ln, MAX_TEXT_LEN-ln, " ");
          if(ln>=MAX_TEXT_LEN) return 0;
        }
      }
      return strlen(b)? value_string(value_new(b)): 0; // not single value
    }
  }
  return 0;
}

item* property_item(object* n, char* path, object* t, bool observe)
{
  if(!strcmp(path, "UID"))     return (item*)n->uid;
  if(!strcmp(path, ""))        return (item*)n->properties;
  if(!strcmp(path, ":"))       return (item*)n->properties;
  if(!strcmp(path, "Alerted")) return (item*)n->alerted;
  size_t m=strlen(path)+1;
  char p[m]; memcpy(p, path, m);
  char* c=strrchr(p, ':');
  if(!c) return properties_get(n->properties, p);
  return nested_property_item(n, p, t, observe);
}

item* nested_property_item(object* n, char* path, object* t, bool observe)
{
  size_t m=strlen(path)+1;
  char p[m]; memcpy(p, path, m);
  char* c=strchr(p, ':');
  *c=0; c++;
  bool observe2=observe && !(*p>='A' && *p<='Z');
  item* i=property_item(n,p,t,observe2);
  if(!i) return 0;
  if(i->type==ITEM_VALUE){
    char* uid=value_string((value*)i);
    bool looksLikeItIsIndexedOne=(*c=='1');
    if(looksLikeItIsIndexedOne){
      if(strlen(c)==1) return i;
      c+=2; // skip '1:' to next bit
    }
    if(is_uid(uid)){
      object* o=find_object(uid,t,observe2);
      return o? property_item(o,c,t,observe2): 0;
    }
    return 0;
  }
  if(i->type==ITEM_LIST){
    char* e; uint32_t index=strtol(c,&e,10);
    if(!(*e==0 || *e==':')) return 0;
    item* r=list_get_n((list*)i,index);
    if(!(r && r->type==ITEM_VALUE && *e==':')) return r;
    char* uid=value_string((value*)r);
    object* o=find_object(uid,t,observe2);
    return o? property_item(o,e+1,t,observe2): 0;
  }
  return 0;
}

char* channel_of(char* device_uid)
{
  return "serial";
}

void ping_object(char* uid, object* o)
{
  uint64_t curtime = time_ms();
  if(!o->last_observe || curtime > o->last_observe + 10000){
    o->last_observe = curtime + 1;
    char* device_uid = value_string(o->devices);
    onp_send_observe(uid, channel_of(device_uid));
  }
}

object* find_object(char* uid, object* n, bool observe)
{
  if(!is_uid(uid) || !n){
    return 0;
  }
  object* o=onex_get_from_cache(uid);
  if(!o){
    o=new_shell(value_new(uid), value_string(n->uid));
    if(add_to_cache_and_persist(o)) ping_object(uid, o);
    return 0;
  }
  if(is_shell(o)){
    ping_object(uid, o);
    return 0;
  }
  if(observe){
    add_notify(o,n->uid);
    if(object_is_remote(o)){
      ping_object(uid, o);
    }
  }
  return o;
}

uint16_t object_property_length(object* n, char* path)
{
  item* i=property_item(n,path,n,true);
  if(!i) return 0;
  switch(i->type){
    case ITEM_VALUE: return 1;
    case ITEM_PROPERTIES: return 1;
    case ITEM_LIST:  return list_size((list*)i);
  }
  return 0;
}

char* object_property_get_n(object* n, char* path, uint8_t index)
{
  item* v=0;
  item* i=property_item(n,path,n,true);
  if(!i) return 0;
  switch(i->type){
    case ITEM_LIST: { v=list_get_n((list*)i,index); break; }
    case ITEM_VALUE: { if(index==1) v=i; break; }
    case ITEM_PROPERTIES: break;
  }
  if(!(v && v->type==ITEM_VALUE)) return 0;
  return value_string((value*)v);
}

int16_t object_property_size(object* n, char* path)
{
  item* i=property_item(n,path,n,true);
  if(!i) return -1;
  switch(i->type){
    case ITEM_PROPERTIES: {
      return properties_size((properties*)i);
    }
    case ITEM_VALUE: {
      break;
    }
    case ITEM_LIST: {
      break;
    }
  }
  return -1;
}

char* object_property_key(object* n, char* path, uint16_t index)
{
  char* key=0;
  item* i=property_item(n,path,n,true);
  if(!i) return 0;
  switch(i->type){
    case ITEM_PROPERTIES: {
      key=properties_key_n((properties*)i, index);
      break;
    }
    case ITEM_VALUE: {
      break;
    }
    case ITEM_LIST: {
      break;
    }
  }
  return key;
}

char* object_property_val(object* n, char* path, uint16_t index)
{
  item* v=0;
  item* i=property_item(n,path,n,true);
  if(!i) return 0;
  switch(i->type){
    case ITEM_PROPERTIES: {
      v=properties_get_n((properties*)i, index);
      break;
    }
    case ITEM_VALUE: {
      break;
    }
    case ITEM_LIST: {
      break;
    }
  }
  if(!(v && v->type==ITEM_VALUE)) return 0;
  return value_string((value*)v);
}

static bool object_property_is_observe(object* n, char* path, char* expected, bool observe)
{
  if(!n) return false;
  if(!strcmp(path, "UID")){
    return expected && value_is(n->uid, expected);
  }
  item* i=property_item(n,path,n,observe);
  if(!i) return (!expected || !*expected);
  if(i->type==ITEM_VALUE){
    return expected && value_is((value*)i, expected);
  }
  return false;
}

bool object_property_is(object* n, char* path, char* expected)
{
  return object_property_is_observe(n, path, expected, true);
}

bool object_property_is_peek(object* n, char* path, char* expected)
{
  return object_property_is_observe(n, path, expected, false);
}

static bool object_property_contains_observe(object* n, char* path, char* expected, bool observe)
{
  if(!n) return false;
  if(!strcmp(path, "UID")){
    return expected && value_is(n->uid, expected);
  }
  item* i=property_item(n,path,n,observe);
  if(!i) return (!expected || !*expected);
  if(i->type==ITEM_VALUE){
    return expected && value_is((value*)i, expected);
  }
  if(i->type==ITEM_LIST){
    int j; int sz=list_size((list*)i);
    for(j=1; j<=sz; j++){
      item* y=list_get_n((list*)i, j);
      if(y->type!=ITEM_VALUE) continue;
      if(expected && value_is((value*)y, expected)) return true;
    }
    return false;
  }
  return false;
}

bool object_property_contains(object* n, char* path, char* expected)
{
  return object_property_contains_observe(n, path, expected, true);
}

bool object_property_contains_peek(object* n, char* path, char* expected)
{
  return object_property_contains_observe(n, path, expected, false);
}

bool object_property_set(object* n, char* path, char* val)
{
  if(!n->running_evals && has_notifies(n)){
    log_write("\nSetting property in an object but not running in an evaluator! uid: %s  %s: '%s'\n\n", value_string(n->uid), path, val? val: "");
  }
  size_t m=strlen(path)+1;
  char p[m]; memcpy(p, path, m);
  char* c=strrchr(p, ':');
  if(!val || !*val){
    if(c) return nested_property_delete(n, path);
    bool ok=!!properties_delete(n->properties, p);
    if(ok) save_and_notify(n);
    return ok;
  }
  if(c) return nested_property_set(n, p, val);
  bool ok=set_value_or_list(n, p, val);
  if(ok) save_and_notify(n);
  return ok;
}

bool set_value_or_list(object* n, char* key, char* val)
{
  item* i=properties_get(n->properties, key);
  if(i) item_free(i);
  if(!strchr(val, ' ') && !strchr(val, '\n')){
    return properties_set(n->properties, key, value_new(val));
  }
  list* l=list_new_from(val, MAX_LIST_SIZE);
  bool ok=properties_set(n->properties, key, l);
  if(!ok) list_free(l);
  return ok;
}

bool nested_property_set(object* n, char* path, char* val)
{
  size_t m=strlen(path)+1;
  char p[m]; memcpy(p, path, m);
  char* c=strchr(p, ':');
  *c=0; c++;
  item* i=property_item(n,p,0,true);
  bool ok=false;
  if(i) switch(i->type){
    case ITEM_VALUE: {
      if(!strcmp(c,"1")) ok=set_value_or_list(n, p, val); // not single
      break;
    }
    case ITEM_LIST: {
      char* e; uint32_t index=strtol(c,&e,10);
      ok=list_set_n((list*)i, index, value_new(val)); // not single; doesn't free like set_value_or_list
      break;
    }
    case ITEM_PROPERTIES: {
      break;
    }
  }
  if(ok) save_and_notify(n);
  return ok;
}

bool nested_property_delete(object* n, char* path)
{
  size_t m=strlen(path)+1;
  char p[m]; memcpy(p, path, m);
  char* c=strchr(p, ':');
  *c=0; c++;
  item* i=property_item(n,p,0,true);
  bool ok=false;
  if(i) switch(i->type){
    case ITEM_VALUE: {
      if(!strcmp(c,"1")) ok=!!properties_delete(n->properties, p);
      break;
    }
    case ITEM_LIST: {
      char* e; uint32_t index=strtol(c,&e,10);
      list* l=(list*)i;
      ok=list_del_n(l, index);
      if(!ok) break;
      if(list_size(l)==1){
        properties_set(n->properties, p, list_get_n(l,1));
        list_free(l);
      }
      break;
    }
    case ITEM_PROPERTIES: {
      break;
    }
  }
  if(ok) save_and_notify(n);
  return ok;
}

bool object_property_add(object* n, char* path, char* val)
{
  if(!n->running_evals && has_notifies(n)){
    log_write("\nSetting property in an object but not running in an evaluator! uid: %s  %s: +'%s'\n\n", value_string(n->uid), path, val? val: "");
  }
  if(strchr(path, ':')) return false; // no sub-properties yet
  if(!val || !*val) return 0;
  if(!strcmp(path, "Notifying")){
    if(!is_uid(val)) return false;
    add_notify(n, value_new(val));
    return true;
  }
  item* i=properties_get(n->properties, path);
  bool ok=true;
  if(!i){
    ok=properties_set(n->properties, path, value_new(val)); // not single
  }
  else
  switch(i->type){
    case ITEM_VALUE: {
      list* l=list_new(MAX_LIST_SIZE);
      ok=ok && list_add(l,i);
      ok=ok && list_add(l,value_new(val)); // not single
      ok=ok && properties_set(n->properties, path, l);
      break;
    }
    case ITEM_LIST: {
      list* l=(list*)i;
      ok=ok && list_add(l,value_new(val)); // not single
      break;
    }
    case ITEM_PROPERTIES: {
      return false;
      break;
    }
  }
  if(ok) save_and_notify(n);
  return ok;
}

// ------------------------------------------------------

bool add_notify(object* o, value* notify)
{
  int i;
  for(i=0; i< OBJECT_MAX_NOTIFIES; i++){
    if(o->notify[i] && value_equal(o->notify[i], notify)) return true;
  }
  for(i=0; i< OBJECT_MAX_NOTIFIES; i++){
    if(!o->notify[i]){ o->notify[i]=notify; return true; }
  }
  log_write("can't add notify %s to %s\n", value_string(notify), value_string(o->uid));
  show_notifies(o);
  return false;
}

void set_notifies(object* o, char* notify)
{
  list* li=list_new_from(notify, OBJECT_MAX_NOTIFIES);
  if(!li) return;
  int i; for(i=0; i < list_size(li); i++){
    o->notify[i]=(value*)list_get_n(li, i+1);
  }
  list_free(li);
}

void save_and_notify(object* o)
{
  persistence_put(o);
  int i;
  for(i=0; i< OBJECT_MAX_NOTIFIES; i++){
    if(!o->notify[i]) continue;
    char* notify = value_string(o->notify[i]);
    object* n=onex_get_from_cache(notify);
    if(!n){
      // TODO !!
      if(!object_is_remote(o)) onp_send_object(o, channel_of(notify));
      continue;
    }
    if(!object_is_device(n)){
      if(!object_is_remote(n)) run_evaluators(n,0,o);
      else
      if(!object_is_remote(o)) onp_send_object(o, channel_of(notify));
    }
    else{
      if(!object_is_remote(o)) onp_send_object(o, channel_of(notify));
    }
  }
  if(object_is_remote_device(o)){
    run_evaluators(onex_device_object, 0, o);
  }
  if(o==onex_device_object){
    onp_send_object(onex_device_object, channel_of("")); // all channels
  }
}

bool has_notifies(object* o)
{
  for(int i=0; i< OBJECT_MAX_NOTIFIES; i++){
    if(o->notify[i]) return true;
  }
  return false;
}

void show_notifies(object* o)
{
  log_write("notifies of %s\n", value_string(o->uid));
  int i;
  for(i=0; i< OBJECT_MAX_NOTIFIES; i++){
    if(o->notify[i]){ log_write("%s ", value_string(o->notify[i])); }
  }
  log_write("\n--------------\n");
}

// ------------------------------------------------------

char* object_to_text(object* n, char* b, uint16_t s, int style)
{
  if(!n){ *b = 0; return b; }

  int ln=0;

  ln+=snprintf(b+ln, s-ln, "UID: %s", value_string(n->uid));
  if(ln>=s){ *b = 0; return b; }

  if(style==OBJECT_TO_TEXT_NETWORK){
    ln+=snprintf(b+ln, s-ln, " Devices: %s", value_string(onex_device_object->uid));
    if(ln>=s){ *b = 0; return b; }
  }

  if(n->evaluator && style>=OBJECT_TO_TEXT_PERSIST){
    ln+=snprintf(b+ln, s-ln, " Eval: %s", value_string(n->evaluator));
    if(ln>=s){ *b = 0; return b; }
  }

  if(n->devices && style>=OBJECT_TO_TEXT_PERSIST){
    ln+=snprintf(b+ln, s-ln, " Devices: %s", value_string(n->devices));
    if(ln>=s){ *b = 0; return b; }
  }

  if(n->cache && style>=OBJECT_TO_TEXT_PERSIST){
    ln+=snprintf(b+ln, s-ln, " Cache: %s", value_string(n->cache));
    if(ln>=s){ *b = 0; return b; }
  }

  int j;
  for(j=0; j< OBJECT_MAX_NOTIFIES; j++){
    if(n->notify[j]){
      if(j==0) ln+=snprintf(b+ln, s-ln, " Notify:");
      if(ln>=s){ *b = 0; return b; }
      ln+=snprintf(b+ln, s-ln, " ");
      if(ln>=s){ *b = 0; return b; }
      ln+=strlen(value_to_text(n->notify[j], b+ln, s-ln));
      if(ln>=s){ *b = 0; return b; }
    }
  }

  if(n->alerted && style>=OBJECT_TO_TEXT_PERSIST){
    ln+=snprintf(b+ln, s-ln, " Alerted: %s", value_string(n->alerted));
    if(ln>=s){ *b = 0; return b; }
  }

  properties* p=n->properties;
  for(j=1; j<=properties_size(p); j++){
    ln+=snprintf(b+ln, s-ln, " ");
    if(ln>=s){ *b = 0; return b; }
    ln+=snprintf(b+ln, s-ln, "%s", properties_key_n(p,j));
    if(ln>=s){ *b = 0; return b; }
    ln+=snprintf(b+ln, s-ln, ": ");
    if(ln>=s){ *b = 0; return b; }
    ln+=strlen(item_to_text(properties_get_n(p,j), b+ln, s-ln));
    if(ln>=s){ *b = 0; return b; }
  }
  return b;
}

void object_log(object* o)
{
  char buff[MAX_TEXT_LEN];
  log_write("{ %s }\n", object_to_text(o,buff,MAX_TEXT_LEN,OBJECT_TO_TEXT_LOG));
}

// -----------------------------------------------------------------------

void onex_init(char* dbpath)
{
  persistence_init(dbpath);
  device_init();
  onp_init();
}

void onex_loop()
{
#if defined(NRF5)
#if defined(HAS_SERIAL)
  serial_loop();
#endif
#endif
  persistence_loop();
  onp_loop();
}

static properties* objects_cache=0;

bool add_to_cache(object* n)
{
  if(!objects_cache) objects_cache=properties_new(MAX_OBJECTS);
  if(!properties_set(objects_cache, value_string(n->uid), n)){
    log_write("No more room for objects!!\n");
    return false;
  }
  return true;
}

object* onex_get_from_cache(char* uid)
{
  if(!uid || !(*uid)) return 0;
  object* o=properties_get(objects_cache, uid);
  if(!o)  o=persistence_get(uid);
  return o;
}

bool add_to_cache_and_persist(object* n)
{
  if(!add_to_cache(n)) return false;
  persistence_put(n);
  return true;
}

void onex_show_cache()
{
  log_write("+-----------cache dump------------\n");
  char buff[MAX_TEXT_LEN*8];
  for(int n=1; n<=properties_size(objects_cache); n++){
    object* o=properties_get_n(objects_cache,n);
    log_write("| %s\n", object_to_text(o,buff,MAX_TEXT_LEN*8,OBJECT_TO_TEXT_LOG));
  }
  log_write("+---------------------------------\n");
}

void onex_un_cache(char* uid)
{
  persistence_flush();
  if(!uid || !(*uid)) return;
  object* o=properties_delete(objects_cache, uid);
  object_free(o);
  scan_objects_text_for_keep_active();
}

static properties* evaluators=0;

void onex_set_evaluators(char* name, ...)
{
  if(!evaluators) evaluators = properties_new(32);
  list* evals = (list*)properties_get(evaluators, name);
  if(!evals){
    evals = list_new(6);
    properties_set(evaluators, name, evals);
  }
  else list_clear(evals, false);

  onex_evaluator evaluator;
  va_list valist;
  va_start(valist, name);
  while((evaluator = va_arg(valist, onex_evaluator))){
    list_add(evals, evaluator);
  }
  va_end(valist);
}

void onex_run_evaluators(char* uid, void* data){
  if(!uid) return;
  object* o=onex_get_from_cache(uid);
  run_evaluators(o, data, 0);
}

void run_evaluators(object* o, void* data, object* alerted){
  if(!o || !o->evaluator) return;
  if(o->running_evals){
#if defined(LOG_TO_GFX) || defined(LOG_TO_BLE)
    char* uid=value_string(o->uid);
    log_write("E!%.*s", 7, uid+4);
#else
    log_write("Already in evaluators! %s\n", value_string(o->uid));
#endif
    return;
  }
  o->running_evals=true;
  o->alerted=alerted? alerted->uid: 0;
  list* evals = (list*)properties_get(evaluators, value_string(o->evaluator));
  for(int i=1; i<=list_size(evals); i++){
    onex_evaluator eval=(onex_evaluator)list_get_n(evals, i);
    if(!eval(o, data)) break;
  }
  o->alerted=0;
  o->running_evals=false;
}

// -----------------------------------------------------------------------

static properties* objects_text=0;
static properties* objects_to_save=0;

static FILE* db=0;

#if !defined(NRF5)
bool mkdir_p(char* filename)
{
  char* fn=strdup(filename);
  char* s=fn;
  while((s=strchr(s+1, '/'))){
    *s=0;
    if(mkdir(fn, S_IRWXU) && errno != EEXIST) return false;
    *s='/';
  }
  free(fn);
  return true;
}
#endif

void persistence_init(char* filename)
{
  objects_text=properties_new(MAX_OBJECTS);
  objects_to_save=properties_new(MAX_OBJECTS);
  if(!*filename) return;
#if !defined(NRF5)
  if(!mkdir_p(filename)){
    log_write("Couldn't make directory for '%s' errno=%d\n", filename, errno);
    return;
  }
#endif
  db=fopen(filename, "a+");
  if(!db){
    log_write("Couldn't open DB file '%s' errno=%d\n", filename, errno);
    return;
  }
  fseek(db, 0, SEEK_END);
  long len = ftell(db);
  fseek(db, 0, SEEK_SET);
  char* alldbtext=malloc(len*sizeof(char)+1);
  if(!alldbtext) {
    fclose(db); db=0;
    log_write("Can't allocate space for DB file %s\n", filename);
    return;
  }
  long n=fread(alldbtext, sizeof(char), len, db);
  alldbtext[n] = '\0';
  char* text=strtok(alldbtext, "\n");
  while(text){
    if(!strncmp(text, "UID: ", 5)){
      char* u=text+5;
      char* e=strchr(u, ' ');
      if(e) *e=0;
      char uid[MAX_UID_LEN]; size_t m=snprintf(uid, MAX_UID_LEN, "%s", u);
      if(e) *e=' ';
      if(m<MAX_UID_LEN){
        free(properties_delete(objects_text, uid));
        properties_set(objects_text, uid, strdup(text));
      }
    }
    text=strtok(0, "\n");
  }
  free(alldbtext);
  scan_objects_text_for_keep_active();
}

void device_init()
{
  if(!onex_device_object){
    onex_device_object=object_new(0, 0, "device", 8);
    object_keep_active(onex_device_object, true);
  }
}

static uint32_t lasttime=0;

void persistence_loop()
{
  uint64_t curtime = time_ms();
  if(curtime > lasttime+100){
    persistence_flush();
    lasttime = curtime;
  }
}

object* persistence_get(char* uid)
{
  char* text=properties_get(objects_text, uid);
  if(!text) return 0;
  object* o=new_object_from(text, MAX_OBJECT_SIZE);
  if(!o) return 0;
  if(!add_to_cache(o)) return 0;
  return o;
}

void persistence_put(object* o)
{
  value* uid=o->uid;
  properties_set(objects_to_save, value_string(uid), uid);
}

void persistence_flush()
{
  uint16_t sz=properties_size(objects_to_save);
  if(!sz) return;
  for(int j=1; j<=sz; j++){
    char* uid=value_string(properties_get_n(objects_to_save, j));
    object* o=onex_get_from_cache(uid);
    char buff[MAX_TEXT_LEN];
    char* text=object_to_text(o,buff,MAX_TEXT_LEN,OBJECT_TO_TEXT_PERSIST);
    free(properties_delete(objects_text, uid));
    properties_set(objects_text, uid, strdup(text));
    if(db) fprintf(db, "%s\n", text);
  }
  properties_clear(objects_to_save, false);
  if(db) fflush(db);
}

void scan_objects_text_for_keep_active()
{
  for(int n=1; n<=properties_size(objects_text); n++){
    char* uid=0;
    char* p=properties_get_n(objects_text, n);
    while(true){
      char* key=get_key(&p); if(!key) break;            if(!*key){ free(key); break; }
      char* val=get_val(&p); if(!val || !*val){ free(key); if(val) free(val); break; }
      if(!isupper((unsigned char)(*key))){
        free(key); free(val);
        break;
      }
      if(!strcmp(key,"Cache") && !strcmp(val,"keep-active")){
        uid=properties_key_n(objects_text, n);
        free(key); free(val);
        break;
      }
      free(key); free(val);
    }
    if(uid){
      object* o=onex_get_from_cache(uid);
      if(object_is_local_device(o)) onex_device_object = o;
      run_evaluators(o,0,0);
    }
  }
}

// -----------------------------------------------------------------------

void onf_recv_observe(char* text, char* channel)
{
  char* uid=strchr(text,':')+2;
  char* u=uid; while(*u > ' ') u++; *u=0;
  object* o=onex_get_from_cache(uid);
  if(!o) return;
  add_notify(o, value_new("uid-of-device"));
  if(!object_is_remote(o)) onp_send_object(o, channel);
}

void onf_recv_object(char* text, char* channel)
{
  object* n=new_object_from(text, MAX_OBJECT_SIZE);
  if(!n) return;
  object* o=onex_get_from_cache(value_string(n->uid));
  if(!o){
    if(!add_to_cache(n)) return;
    o=n;
  }
  else{
    item_free(o->properties);
    o->properties = n->properties;
    o->devices    = n->devices;
    free(n);
  }
  save_and_notify(o);
}

// -----------------------------------------------------------------------

