
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>

#if defined(NRF5)
// if any of these linker-fixen actually gets called, we should find out soon enough when
// it hangs there and I debug it...
__attribute__((weak)) void _close() { while (1); }
__attribute__((weak)) void _fstat() { while (1); }
__attribute__((weak)) void _getpid(){ while (1); }
__attribute__((weak)) void _isatty(){ while (1); }
__attribute__((weak)) void _kill()  { while (1); }
__attribute__((weak)) void _lseek() { while (1); }
__attribute__((weak)) void _read()  { while (1); }
__attribute__((weak)) void _write() { while (1); }
#endif

#if defined(NRF5)
#include <app_util_platform.h>
#else
#include <pthread.h>
#endif

#include <onex-kernel/mem.h>
#include <onex-kernel/lib.h>
#include <onex-kernel/log.h>
#include <onex-kernel/random.h>
#include <onex-kernel/time.h>
#include <onex-kernel/serial.h>
#include <items.h>

#include "persistence.h"

#include <onp.h>
#include <onn.h>

// ---------------------------------------------------------------------------------

#define LIST_EDIT_MODE_SET     1
#define LIST_EDIT_MODE_PREPEND 2
#define LIST_EDIT_MODE_APPEND  3
#define LIST_EDIT_MODE_DELETE  4

static value*  generate_uid();
static char*   get_key(char** p);
static char*   get_val(char** p);
static bool    add_to_cache(object* n);
static bool    add_to_cache_and_persist(object* n);
static object* find_object(char* uid, char* nuid, bool notify);
static item*   property_item(object* n, char* path, object* t, bool notify);
static item*   nested_property_item(object* n, char* path, object* t, bool notify);
static bool    object_property_edit(object* n, char* path, char* val, uint8_t mode);
static bool    nested_property_edit(object* n, char* path, uint16_t index, char* val, uint8_t mode);
static bool    property_edit(object* n, char* key, char* val, uint8_t mode);
static bool    add_notify(object* o, char* notifyuid);
static void    save_and_notify(object* n);
static void    show_notifies(object* o);
static object* new_object(value* uid, value* version, char* evaluator, char* is, uint8_t max_size);
static object* new_shell(value* uid);
static void    run_evaluators(object* o, void* data, value* alerted, bool timedout);
static bool    run_any_evaluators();
static void    set_to_notify(value* uid, void* data, value* alerted, uint64_t timeout);

static void    persist_init(char* dbpath);
static void    persist_put(object* o, bool saving_metadata);
static bool    persist_loop();
static void    persist_flush();
static void    persist_pull_keep_active();

static void    timer_init();
static void    device_init();

// ---------------------------------

typedef struct object {
  // --------- on-wire props ---------
  value*      uid;
  value*      version;
  list*       devices;
  list*       notifies;
  properties* properties;
  // --------- local props ---------
  value*      evaluator;
  value*      alerted;
  value*      cache;
  value*      persist;
  value*      timer;
  bool        running_evals;
} object;

// ---------------------------------

static char* test_uid_prefix=0;

object* onex_device_object=0;

// ---------------------------------

value* generate_uid() {
  char b[24];
  if(test_uid_prefix && strlen(test_uid_prefix) >= 4)
  snprintf(b, 24, "uid-%.4s-%02x%02x-%02x%02x-%02x%02x",
    test_uid_prefix,
    random_ish_byte(), random_ish_byte(),
    random_ish_byte(), random_ish_byte(),
    random_ish_byte(), random_ish_byte()
  );
  else
  snprintf(b, 24, "uid-%02x%02x-%02x%02x-%02x%02x-%02x%02x",
    random_ish_byte(), random_ish_byte(),
    random_ish_byte(), random_ish_byte(),
    random_ish_byte(), random_ish_byte(),
    random_ish_byte(), random_ish_byte()
  );
  return value_new(b);
}

// ----------------------------------------------------------

bool is_uid(char* uid){
  return uid && !strncmp(uid,"uid-",4);
}

bool object_is_local(object* o){
  return o && !list_size(o->devices);  // REVISIT: implied remote when !o
}

bool is_local(char* uid){
  object* o=onex_get_from_cache(uid);
  return object_is_local(o);
}

bool object_is_remote(object* o){
  return o && list_size(o->devices); // REVISIT: o || list_s...?
}

bool object_is_shell(object* o){
  return object_is_remote(o) && value_is(list_get_n(o->devices,1), "shell");
}

bool is_shell(char* uid){
  object* o=onex_get_from_cache(uid);
  return object_is_shell(o);
}

bool object_is_device(object* o){
  return o && object_property_contains(o, "is", "device");
}

bool object_is_local_device(object* o){
  return object_is_local(o) && object_is_device(o);
}

bool object_is_remote_device(object* o){
  return object_is_remote(o) && object_is_device(o);
}

// ----------------------------------------------------------

object* object_new_from(char* text, uint8_t max_size) {
  object* n=object_from_text(text, false, max_size);
  if(!n) return 0;
  char* uid=value_string(n->uid);
  if(onex_get_from_cache(uid)){
    log_write("Attempt to create an object with UID %s that already exists\n", uid);
    object_free(n);
    return 0;
  }
  if(!add_to_cache_and_persist(n)){ object_free(n); return 0; }
  return n;
}

object* object_new(char* uid, char* evaluator, char* is, uint8_t max_size) {
  if(onex_get_from_cache(uid)){
    log_write("Attempt to create an object with UID %s that already exists\n", uid);
    return 0;
  }
  object* n=new_object(value_new(uid), value_new("1"), evaluator, is, max_size);
  if(!n) return 0;
  n->devices  = 0;  // none for new locals: only when off net/remote
  n->notifies = list_new(OBJECT_MAX_NOTIFIES);
  if(!add_to_cache_and_persist(n)){ object_free(n); return 0; }
  return n;
}

object* new_object(value* uid, value* version, char* evaluator, char* is, uint8_t max_size) {
  object* n=(object*)mem_alloc(sizeof(object));
  n->uid    =uid?     uid: generate_uid();
  n->version=version? version: value_new("1");
  n->properties=properties_new(max_size);
  if(evaluator) n->evaluator=value_new(evaluator);
  if(is) property_edit(n, "is", is, LIST_EDIT_MODE_SET);
  return n;
}

void log_syntax_fl(char* fi, int li, char* t, char* p){
  uint8_t charsback = p-t < 4? p-t: 4;
  log_write("%s:%d %.8s..%.4s>>%.4s<<%.4s\n", fi,li, t, p-charsback, p, p+charsback);
}

#define log_syntax(t,p) log_syntax_fl(__FILE__,__LINE__, t,p)

object* object_from_text(char* text, bool need_uid_ver, uint8_t max_size){

  object* n=0;

  value* uid=0;
  value* version=0;
  char*  devices=0;
  char*  notifies=0;
  value* evaluator=0;
  value* cache=0;
  value* persist=0;

  // REVISIT: presumably to allow read-only strings, but seems expensive and risky
  size_t m=strlen(text)+1; char t[m]; memcpy(t, text, m);
  char* p=t;

  char* key=0; char* val=0;
  #define FREE_BREAK_1 { log_syntax(t,p); mem_freestr(key); mem_freestr(val); object_free(n); n=0; break; }
  while(true){

    key=get_key(&p); if(!key) break; if(!*key) FREE_BREAK_1;
    val=get_val(&p); if(!val     ||     !*val) FREE_BREAK_1;

    if(!strcmp(key,"UID")) uid=value_new(val);
    else
    if(!strcmp(key,"Ver")) version=value_new(val);
    else
    if(!strcmp(key,"Devices") && !devices) devices=mem_strdup(val);
    else
    if(!strcmp(key,"Notify") && !notifies) notifies=mem_strdup(val);
    else
    if(!strcmp(key,"Eval")) evaluator=value_new(val);
    else
    if(!strcmp(key,"Cache")) cache=value_new(val);
    else
    if(!strcmp(key,"Persist")) persist=value_new(val);
    else
    if(isupper((unsigned char)(*key)));
    else {
      if(!n){
        if(need_uid_ver && (!uid || !version)) FREE_BREAK_1;
        n=new_object(uid, version, 0, 0, max_size);
        if(devices)   n->devices  = list_vals_new_from(devices,  OBJECT_MAX_DEVICES);
        ;             n->notifies = list_vals_new_from(notifies, OBJECT_MAX_NOTIFIES);
        if(evaluator) n->evaluator=evaluator;
        if(cache)     n->cache=cache;
        if(persist)   n->persist=persist;
      }
      if(!property_edit(n, key, val, LIST_EDIT_MODE_SET)) FREE_BREAK_1;
    }
    mem_freestr(key); key=0;
    mem_freestr(val); val=0;
  }
  mem_freestr(devices);
  mem_freestr(notifies);
  return n;
}

observe observe_from_text(char* u){

  char* o=u;
  while(*u > ' ') u++;
  if(!*u) return (observe){0,0};
  *u=0;
  if(strcmp(o, "OBS:")) return (observe){0,0};
  *u=' ';
  u++;

  char* uid=u;
  while(*u > ' ') u++;
  if(!*u) return (observe){0,0};
  *u=0;
  if(!strlen(uid)) return (observe){0,0};
  uid=mem_strdup(uid);
  *u=' ';
  u++;

  char* dvp=u;
  while(*u > ' ') u++;
  if(!*u){ mem_freestr(uid); return (observe){0,0}; }
  *u=0;
  if(strcmp(dvp, "Devices:")){ mem_freestr(uid); return (observe){0,0}; }
  *u=' ';
  u++;

  char* dev=u;
  while(*u > ' ') u++;
  *u=0;
  if(!strlen(dev)){ mem_freestr(uid); return (observe){0,0}; }
  dev=mem_strdup(dev);

  if(!strcmp(object_property(onex_device_object, "UID"), dev)){
    mem_freestr(uid); mem_freestr(dev);
    return (observe){0,0};
  }

  return (observe){ uid, dev };
}

object* new_shell(value* uid){
  object* n=(object*)mem_alloc(sizeof(object));
  n->uid=uid;
  n->version=value_new("0");
  n->devices  = list_vals_new_from("shell", OBJECT_MAX_DEVICES);
  n->notifies = list_new(OBJECT_MAX_NOTIFIES);
  n->properties=properties_new(MAX_OBJECT_SIZE);
  return n;
}

void object_free(object* o) {
  if(!o) return;
  value_free(o->uid);
  value_free(o->version);
  list_free(o->devices, true);
  list_free(o->notifies, true);
  properties_free(o->properties, true);
  value_free(o->alerted);
  value_free(o->evaluator);
  value_free(o->cache);
  value_free(o->persist);
  value_free(o->timer);
  mem_free(o);
}

char* get_key(char** p) {
  while(isspace(**p)) (*p)++;
  if(!**p) return 0;
  char* s=strchr(*p, ' ');
  char* c=strstr(*p, ": ");
  if(s<c || !c) return 0;
  (*c)=0;
  char* r=mem_strdup(*p);
  (*c)=':';
  (*p)=c+1;
  return r;
}

char* get_val(char** p) {
  while(isspace(**p)) (*p)++;
  if(!**p) return 0;
  char* c=strstr(*p, ": ");
  while(c && *(c-1)=='\\') c=strstr(c+1, ": ");
  char* r=0;
  if(!c){
    char* s=strrchr(*p, 0);
    do s--; while(isspace(*s)); s++;
    (*s)=0;
    r=mem_strdup(*p);
    (*p)+=strlen(*p);
  }
  else{
    (*c)=0;
    char* s=strrchr(*p, ' ');
    if(!s){ (*c)=':'; return 0; }
    do s--; while(isspace(*s)); s++;
    (*c)=':';
    (*s)=0;
    r=mem_strdup(*p);
    (*s)=' ';
    (*p)=s+1;
  }
  uint16_t y=0;
  for(uint16_t x=0; x<strlen(r); x++){
    if(r[x]=='\\') continue;
    if(y!=x) r[y]=r[x];
    y++;
  }
  r[y]=0;
  return r;
}

void object_set_evaluator(object* n, char* evaluator) {
  n->evaluator=value_new(evaluator);
  persist_put(n, true);
}

// ------------------------------------------------------

void object_set_cache(object* n, char* cache) {
  if(!cache || !*cache){
    n->cache=0;
  }
  else{
    n->cache=value_new(cache);
  }
  persist_put(n, true);
}

char* object_get_cache(object* n) {
  return value_string(n->cache);
}

void object_set_persist(object* n, char* persist){
  if(!persist || !*persist){
    n->persist=0;
  }
  else{
    n->persist=value_new(persist);
  }
  persist_put(n, true);
}

char* object_get_persist(object* n){
  return value_string(n->persist);
}

// ------------------------------------------------------

static char* object_property_observe(object* n, char* path, bool notify) {
  if(!n) return 0;
  if(!strcmp(path, "UID"))     return value_string(n->uid);
  if(!strcmp(path, "Ver"))     return value_string(n->version);
  if(!strcmp(path, "Timer"))   return value_string(n->timer);
  if(!strcmp(path, "Devices")) return value_string(list_get_n(n->devices, 1));
                     // REVISIT Device<s> but only returns the first!!

  item* i=property_item(n,path,n,notify);
  if(i && i->type==ITEM_VALUE) return value_string((value*)i);
  if(i && i->type==ITEM_LIST){
    item* j=list_get_n((list*)i,1);
    if(j && j->type==ITEM_VALUE) return value_string((value*)j);
  }
  return 0;
}

char* object_property(object* n, char* path) {
  return object_property_observe(n, path, true);
}

char* object_pathpair(object* n, char* path1, char* path2){
  char pathbuf[MAX_TEXT_LEN];
  snprintf(pathbuf, MAX_TEXT_LEN, "%s:%s", path1, path2);
  return object_property_observe(n, pathbuf, true);
}

char* object_property_peek(object* n, char* path) {
  return object_property_observe(n, path, false);
}

char* object_property_values(object* n, char* path) {

  // object_property_values deprecated!

  item* i=property_item(n,path,n,true);
  if(!i) return 0;
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
    return strlen(b)? value_string(value_new(b)): 0; // not a single value!
  }
  return 0;
}

item* property_item(object* n, char* path, object* t, bool notify) {
  // REVISIT: why dupe these here? can't go "x:y:Alerted|Timer", or can u?
  if(!strcmp(path, "UID"))     return (item*)n->uid;
  if(!strcmp(path, "Ver"))     return (item*)n->version;
  if(!strcmp(path, "Timer"))   return (item*)n->timer;
  if(!strcmp(path, ""))        return (item*)n->properties;
  if(!strcmp(path, ":"))       return (item*)n->properties;
  if(!strcmp(path, "Alerted")) return (item*)n->alerted;
  if(find_unescaped_colon(path)){
    return nested_property_item(n, path, t, notify);
  }
  size_t m=strlen(path)+1;
  char key[m]; memcpy(key, path, m);
  remove_char_in_place(key, '\\');

  return properties_get(n->properties, key);
}

item* nested_property_item(object* n, char* path, object* t, bool notify) {
  size_t m=strlen(path)+1;
  char p[m]; memcpy(p, path, m);
  char* c=find_unescaped_colon(p);
  *c=0; c++;
  bool notify2=notify && !isupper((unsigned char)(*p));
  item* i=property_item(n,p,t,notify2);
  if(!i) return 0;
  if(i->type==ITEM_VALUE){
    char* uid=value_string((value*)i);
    bool looksLikeItIsIndexedOne=(*c=='1');
    if(looksLikeItIsIndexedOne){
      if(strlen(c)==1) return i;
      c+=2; // skip '1:' to next bit
    }
    if(is_uid(uid)){
      object* o=find_object(uid,value_string(t->uid),notify2);
      return o? property_item(o,c,t,notify2): 0;
    }
    return 0;
  }
  if(i->type==ITEM_LIST){
    char* e; uint16_t in=(uint16_t)strtol(c,&e,10);
    if(!(*e==0 || *e==':')) return 0;
    item* r=list_get_n((list*)i,in);
    if(!(r && r->type==ITEM_VALUE && *e==':')) return r;
    char* uid=value_string((value*)r);
    object* o=find_object(uid,value_string(t->uid),notify2);
    return o? property_item(o,e+1,t,notify2): 0;
  }
  return 0;
}

object* find_object(char* uid, char* nuid, bool notify) {

  if(!(is_uid(uid) && is_uid(nuid))) return 0;

  object* o=onex_get_from_cache(uid);

  if(!o){
    o=new_shell(value_new(uid));
    if(!add_to_cache_and_persist(o)){ object_free(o); return 0; }
  }
  if(notify){
    add_notify(o, nuid);
    if(object_is_remote(o)) onp_send_observe(uid, value_string(list_get_n(o->devices, 1)));
  }
  return o;
}

int32_t object_property_int32(object* n, char* path){
  char* val = object_property_observe(n, path, true);
  return strto_int32(val);
}

int32_t object_pathpair_int32(object* n, char* path1, char* path2){
  char pathbuf[MAX_TEXT_LEN];
  snprintf(pathbuf, MAX_TEXT_LEN, "%s:%s", path1, path2);
  return object_property_int32(n, pathbuf);
}

uint16_t object_property_length(object* n, char* path) {
  item* i=property_item(n,path,n,true);
  if(!i) return 0;
  switch(i->type){
    case ITEM_VALUE: return 1;
    case ITEM_PROPERTIES: return 1;
    case ITEM_LIST:  return list_size((list*)i);
  }
  return 0;
}

uint16_t object_pathpair_length(object* n, char* path1, char* path2){
  char pathbuf[MAX_TEXT_LEN];
  snprintf(pathbuf, MAX_TEXT_LEN, "%s:%s", path1, path2);
  return object_property_length(n, pathbuf);
}

char* object_property_get_n(object* n, char* path, uint16_t index) {
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

char* object_pathpair_get_n(object* n, char* path1, char* path2, uint16_t index){
  char pathbuf[MAX_TEXT_LEN];
  snprintf(pathbuf, MAX_TEXT_LEN, "%s:%s", path1, path2);
  return object_property_get_n(n, pathbuf, index);
}

int16_t object_property_size(object* n, char* path) {
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

char* object_property_key(object* n, char* path, uint16_t index) {
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

char* object_property_key_esc(object* n, char* path, uint16_t index,
                              char* keyesc, uint8_t len){

  char* key=object_property_key(n, path, index);
  if(!key){
    *keyesc=0;
    return 0;
  }
  mem_strncpy(keyesc, key, len);
  return prefix_char_in_place(keyesc, '\\', ':');
}

char* object_property_val(object* n, char* path, uint16_t index) {
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

static bool object_property_is_observe(object* n, char* path, char* expected, bool notify) {
  if(!n) return false;
  item* i=property_item(n,path,n,notify);
  if(!i) return (!expected || !*expected);
  if(i->type==ITEM_VALUE){
    return expected && value_is((value*)i, expected);
  }
  return false;
}

bool object_property_is(object* n, char* path, char* expected) {
  return object_property_is_observe(n, path, expected, true);
}

bool object_property_is_peek(object* n, char* path, char* expected) {
  return object_property_is_observe(n, path, expected, false);
}

bool object_pathpair_is(object* n, char* path1, char* path2, char* expected){
  char pathbuf[MAX_TEXT_LEN];
  snprintf(pathbuf, MAX_TEXT_LEN, "%s:%s", path1, path2);
  return object_property_is_observe(n, pathbuf, expected, true);
}

static bool object_property_contains_observe(object* n, char* path, char* expected, bool notify) {
  if(!n) return false;
  item* i=property_item(n,path,n,notify);
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

bool object_property_contains(object* n, char* path, char* expected) {
  return object_property_contains_observe(n, path, expected, true);
}

bool object_pathpair_contains(object* n, char* path1, char* path2, char* expected){
  char pathbuf[MAX_TEXT_LEN];
  snprintf(pathbuf, MAX_TEXT_LEN, "%s:%s", path1, path2);
  return object_property_contains_observe(n, pathbuf, expected, true);
}

bool object_property_contains_peek(object* n, char* path, char* expected) {
  return object_property_contains_observe(n, path, expected, false);
}

// ---------------------------------------------------------------------------------


static uint16_t timer_id;

void just_wake_up(void*){}

void timer_init() {
  timer_id=time_timeout(just_wake_up, 0);
}

bool set_timer(object* n, char* timer) {
  value_free(n->timer);
  n->timer=value_new(timer);
  char* e; uint32_t tm=strtol(timer,&e,10);
  if(*e) return false;
  set_to_notify(n->uid, 0, 0, time_ms()+tm);
  return true;
}

bool zero_timer(object* n) {
  value_free(n->timer);
  n->timer=value_new("0");
  save_and_notify(n);
  return true;
}

bool stop_timer(object* n) {
  value_free(n->timer);
  n->timer=0;
  save_and_notify(n);
  return true;
}

// ----------------------------------------------

void device_init() {
  if(onex_device_object) return;
  onex_set_evaluators("eval_device", evaluate_device_logic, 0);
  onex_device_object=object_new(0, "eval_device", "device", 8);
  object_set_cache(onex_device_object, "keep-active");
}

// ----------------------------------------------

#define NOT_IN_EVAL(o,action,act) \
  if(!log_to_gfx){ \
    log_write("--------------------------\n" \
              action " property in an object but not running in an evaluator!\n" \
              "uid: %s is: %s @ %s: '%s'\n" \
              "--------------------------\n", \
              value_string(o->uid), \
              object_property(o, "is:1")? object_property(o, "is:1"): "<none set>", \
              path, val? val: "<no val>"); \
  }
/*
  else { \
    char* uid=value_string(o->uid)+4+15; \
    log_write(act " %s|%s|%s", object_property(o, "is:1"), path, val, uid); \
  }
*/

#define IN_EVAL(o) \
  if(!log_to_gfx){ \
    log_write("--------------------------\n" \
              "Already in evaluators! %s\n" \
              "--------------------------\n", \
              value_string(o->uid)); \
  } \
  else{ \
    char* uid=value_string(o->uid)+4+15; \
    log_write("E %s", uid); \
  }

// ----------------------------------------------

bool object_property_set(object* n, char* path, char* val) {
  bool del=(!val || !*val);
  uint8_t mode=del? LIST_EDIT_MODE_DELETE: LIST_EDIT_MODE_SET;
  return object_property_edit(n, path, val, mode);
}

bool object_property_set_n(object* n, char* path, uint16_t index, char* val){

  if(!n->running_evals && list_size(n->notifies)){
    NOT_IN_EVAL(n, "Editing", "E!")
  }
  if(!strcmp(path, "Timer")) return false;
  if(!strcmp(path, "Notifying")) return false;
  bool del=(!val || !*val);
  uint8_t mode=del? LIST_EDIT_MODE_DELETE: LIST_EDIT_MODE_SET;
  return nested_property_edit(n, path, index, val, mode);
}

bool object_property_add(object* n, char* path, char* val) {
  return object_property_edit(n, path, val, LIST_EDIT_MODE_APPEND);
}

bool object_property_insert(object* n, char* path, char* val) {
  return object_property_edit(n, path, val, LIST_EDIT_MODE_PREPEND);
}

bool object_property_setwise_insert(object* n, char* path, char* val){
  if(object_property_contains(n, path, val)) return false;
  object_property_insert(n, path, val);
  return true;
}

// ------------------------------------------------------

bool object_property_edit(object* n, char* path, char* val, uint8_t mode) {

  if(!n->running_evals && list_size(n->notifies)){
    NOT_IN_EVAL(n, "Editing", "E!");
  }
  if(mode!=LIST_EDIT_MODE_DELETE && (!val || !*val)) return false;
  if(!strcmp(path, "Timer")){
    if(!(mode==LIST_EDIT_MODE_SET || mode==LIST_EDIT_MODE_DELETE)) return false;
    bool zero=val && !strcmp(val, "0");
    if(zero) return zero_timer(n);
    if(mode==LIST_EDIT_MODE_DELETE) return stop_timer(n);
    return set_timer(n, val);
  }
  if(!strcmp(path, "Notifying")){
    if(mode!=LIST_EDIT_MODE_APPEND) return false;
    if(!is_uid(val)) return false;
    add_notify(n, val);
    return true;
  }
  if(find_unescaped_colon(path)){
    return nested_property_edit(n, path, 0, val, mode);
  }
  size_t m=strlen(path)+1;
  char key[m]; memcpy(key, path, m);
  remove_char_in_place(key, '\\');

  bool ok=property_edit(n, key, val, mode);

  if(ok) save_and_notify(n);
  return ok;
}

bool property_edit(object* n, char* key, char* val, uint8_t mode){

  if(mode==LIST_EDIT_MODE_SET){
    item* i=properties_get(n->properties, key);
    if(!(strchr(val, ' ') || strchr(val, '\n'))){
      if(i && i->type==ITEM_VALUE && value_is((value*)i, val)) return false;
      item_free(i);
      return properties_set(n->properties, key, value_new(val));
    }
    list* l=list_vals_new_from(val, MAX_LIST_SIZE);
    if(i && i->type==ITEM_LIST && list_vals_equal((list*)i, l)){ list_free(l, true); return false; }
    item_free(i);
    bool ok=properties_set(n->properties, key, l);
    if(!ok) list_free(l, true);
    return ok;
  }

  if(mode==LIST_EDIT_MODE_DELETE){
    item* i=properties_delete(n->properties, key);
    item_free(i);
    return !!i;
  }

  if(strchr(val, ' ') || strchr(val, '\n')) return false; // don't do space-sept val yet

  item* i=properties_get(n->properties, key);
  if(!i){
    return properties_set(n->properties, key, value_new(val));
  }
  switch(i->type){
    case ITEM_VALUE: {
      list* l=list_new(MAX_LIST_SIZE);
      bool ok=false;
      switch(mode){
        case LIST_EDIT_MODE_PREPEND: {
          ok=      list_add(l,value_new(val));
          ok=ok && list_add(l,i);
          break;
        }
        case LIST_EDIT_MODE_APPEND: {
          ok=      list_add(l,i);
          ok=ok && list_add(l,value_new(val));
          break;
        }
      }
      ok=ok && properties_set(n->properties, key, l);
      if(!ok) list_free(l, true);
      return ok;
    }
    case ITEM_LIST: {
      list* l=(list*)i;
      switch(mode){
        case LIST_EDIT_MODE_PREPEND: {
          return list_ins_n(l,1,value_new(val));
        }
        case LIST_EDIT_MODE_APPEND: {
          return list_add(l,value_new(val));
        }
      }
      return false;
    }
    case ITEM_PROPERTIES: {
      return false;
    }
  }
  return false;
}

bool nested_property_edit(object* n, char* path, uint16_t index, char* val, uint8_t mode){

  if(mode!=LIST_EDIT_MODE_DELETE && (strchr(val, ' ') || strchr(val, '\n'))) return false;
  // don't do space-sept val yet

  size_t m=strlen(path)+1;
  char key[m]; memcpy(key, path, m);
  char* c=0;
  if(!index){
    c=find_unescaped_colon(key);
    *c=0; c++;
    char* e; index=(uint16_t)strtol(c,&e,10);
  }
  remove_char_in_place(key, '\\');
  item* i=properties_get(n->properties, key);
  bool ok=false;
  if(!i){
    if(mode==LIST_EDIT_MODE_DELETE) return true;
    if(index==1){
      ok=properties_set(n->properties, key, value_new(val));
    }
  }
  else
  switch(i->type){
    case ITEM_VALUE: {
      if(index && index==1){
        ok=property_edit(n, key, val, mode);
      }
      break;
    }
    case ITEM_LIST: {
      list* l=(list*)i;
      switch(mode){
        case LIST_EDIT_MODE_SET: {
          item_free(list_get_n(l, index));
          ok=list_set_n(l, index, value_new(val));
          break;
        }
        case LIST_EDIT_MODE_PREPEND: {
          ok=list_ins_n(l, index, value_new(val));
          break;
        }
        case LIST_EDIT_MODE_APPEND: {
          ok=list_ins_n(l, index+1, value_new(val));
          break;
        }
        case LIST_EDIT_MODE_DELETE: {
          item* i=list_del_n(l, index);
          item_free(i);
          ok=!!i;
          if(!ok) break;
          if(list_size(l)==1){
            properties_set(n->properties, key, list_get_n(l,1));
            list_free(l, false);
          }
          break;
        }
      }
    }
    case ITEM_PROPERTIES: {
      break;
    }
  }
  if(ok) save_and_notify(n);
  return ok;
}

// ------------------------------------------------------

bool object_property_set_list(object* n, char* path, ... /* char* val, ..., 0 */){
  bool ok=true;
  object_property_set(n, path, 0);
  char* val;
  va_list valist;
  va_start(valist, path);
  while(ok && (val = va_arg(valist, char*))){
    ok=ok && object_property_add(n, path, val);
  }
  va_end(valist);
  return ok;
}

bool object_property_add_list(object* n, char* path, ... /* char* val, ..., 0 */){
  bool ok=true;
  char* val;
  va_list valist;
  va_start(valist, path);
  while(ok && (val = va_arg(valist, char*))){
    ok=ok && object_property_add(n, path, val);
  }
  va_end(valist);
  return ok;
}

bool object_property_set_fmt(object* n, char* path, char* fmt, ... /* <any> val, ... */){

  char valbuf[MAX_TEXT_LEN];
  va_list args;
  va_start(args, fmt);
  vsnprintf(valbuf, MAX_TEXT_LEN, fmt, args);
  va_end(args);

  return object_property_set(n, path, valbuf);
}

// ------------------------------------------------------

#define TO_NOTIFY_FREE    0
#define TO_NOTIFY_NONE    1
#define TO_NOTIFY_DATA    2
#define TO_NOTIFY_ALERTED 3
#define TO_NOTIFY_TIMEOUT 4

typedef struct notification {
  uint8_t type;
  value*  uid;
  union {
    void*    data;
    value*   alerted;
    uint64_t timeout;
  } details;
} notification;

static volatile notification to_notify[MAX_TO_NOTIFY];
static volatile int highest_to_notify=0;

void start_timer_for_soonest_timeout_if_in_future() {
  uint64_t soonest=0;
  for(uint16_t n=0; n<MAX_TO_NOTIFY; n++){
    if(to_notify[n].type!=TO_NOTIFY_TIMEOUT) continue;
    uint64_t t=to_notify[n].details.timeout;
    if(!soonest || t<soonest) soonest=t;
  }
  if(soonest){
    int32_t t=soonest-time_ms();
    if(t>=0) time_start_timer(timer_id, t);
  }
}

void set_to_notify(value* uid, void* data, value* alerted, uint64_t timeout){

#if defined(NRF5)
  CRITICAL_REGION_ENTER();
#else
  static pthread_mutex_t to_notify_lock;
  pthread_mutex_lock(&to_notify_lock);
#endif
  uint16_t n=0;
  uint16_t h= -1;
  bool new_soonest=true;
  for(; n<MAX_TO_NOTIFY; n++){
    if(to_notify[n].type==TO_NOTIFY_FREE) continue;
    h=n;
    if(timeout && to_notify[n].type==TO_NOTIFY_TIMEOUT){
      if(to_notify[n].details.timeout<timeout) new_soonest=false;
    }
    if(!value_equal(to_notify[n].uid, uid)) continue;

    if(!data && !alerted && !timeout && to_notify[n].type==TO_NOTIFY_NONE) break;
    if( data &&                         to_notify[n].type==TO_NOTIFY_DATA &&                to_notify[n].details.data==data) break;
    if(          alerted &&             to_notify[n].type==TO_NOTIFY_ALERTED && value_equal(to_notify[n].details.alerted, alerted)) break;
    if(                      timeout && to_notify[n].type==TO_NOTIFY_TIMEOUT){              to_notify[n].details.timeout = timeout; /* reset timeout! */ break; }
  }
  if(n==MAX_TO_NOTIFY){
    highest_to_notify=h;
    for(n=0; n<MAX_TO_NOTIFY; n++){
      if(to_notify[n].type!=TO_NOTIFY_FREE) continue;
      ;            to_notify[n].uid=uid;     // Must set type last below ↓ to keep thread safety
      if(data){    to_notify[n].details.data    = data;    to_notify[n].type=TO_NOTIFY_DATA;    }
      else
      if(alerted){ to_notify[n].details.alerted = alerted; to_notify[n].type=TO_NOTIFY_ALERTED; }
      else
      if(timeout){ to_notify[n].details.timeout = timeout; to_notify[n].type=TO_NOTIFY_TIMEOUT; }
      else {
                   to_notify[n].details.timeout=0;         to_notify[n].type=TO_NOTIFY_NONE;
      }
      if(n>highest_to_notify) highest_to_notify=n;
      break;
    }
    if(n==MAX_TO_NOTIFY){ log_write("no free notification entries\n"); }
    else
    if(timeout && new_soonest){
      int32_t t=timeout-time_ms();
      if(t>=0) time_start_timer(timer_id, t);
    }
  }
  else{
    // found, but may have reset timeout above
    if(timeout) start_timer_for_soonest_timeout_if_in_future();
  }
#if defined(NRF5)
  CRITICAL_REGION_EXIT();
#else
  pthread_mutex_unlock(&to_notify_lock);
#endif
}

bool run_any_evaluators() {
//if(highest_to_notify < 0) return false;

  bool keep_awake=false;

  uint64_t curtime=time_ms();

  for(uint16_t n=0; n< MAX_TO_NOTIFY; n++){

    uint8_t type=to_notify[n].type;

    if(type==TO_NOTIFY_FREE) continue;
    if(type==TO_NOTIFY_TIMEOUT && to_notify[n].details.timeout>curtime) continue;

    keep_awake=true;

    char* uid=value_string(to_notify[n].uid);
    object* o=onex_get_from_cache(uid);

    switch(type){
      case(TO_NOTIFY_NONE): {
        to_notify[n].type=TO_NOTIFY_FREE;
        run_evaluators(o, 0, 0, false); // REVISIT: same as TO_NOTIFY_DATA
        break;
      }
      case(TO_NOTIFY_DATA): {
        void* data = to_notify[n].details.data;
        to_notify[n].type=TO_NOTIFY_FREE;
        run_evaluators(o, data, 0, false); // REVISIT: same as TO_NOTIFY_NONE
        break;
      }
      case(TO_NOTIFY_ALERTED): {
        value* alerted = to_notify[n].details.alerted;
        to_notify[n].type=TO_NOTIFY_FREE;
        if(object_is_local(o)){
          run_evaluators(o, 0, alerted, false);
        }
        else{ // REVISIT: remote if !o
          onp_send_object(value_string(alerted), o? value_string(list_get_n(o->devices, 1)): "all");
        }
        break;
      }
      case(TO_NOTIFY_TIMEOUT): {
        to_notify[n].type=TO_NOTIFY_FREE;
        start_timer_for_soonest_timeout_if_in_future();
        run_evaluators(o, 0, 0, true);
        break;
      }
    }
  }
  return keep_awake;
}

void onex_show_notify(){
  for(uint16_t n=0; n< MAX_TO_NOTIFY; n++){
    char* uid=value_string(to_notify[n].uid);
    uint8_t type=to_notify[n].type;
    switch(type){
      case(TO_NOTIFY_NONE): {
        log_write("%s none\n", uid);
        break;
      }
      case(TO_NOTIFY_DATA): {
        void* data = to_notify[n].details.data;
        log_write("%s data %x\n", uid, data);
        break;
      }
      case(TO_NOTIFY_ALERTED): {
        value* alerted = to_notify[n].details.alerted;
        log_write("%s alerted %s\n", uid, value_string(alerted));
        break;
      }
      case(TO_NOTIFY_TIMEOUT): {
        uint64_t timeout = to_notify[n].details.timeout;
        log_write("%s timeout %ld\n", uid, (uint32_t)timeout);
        break;
      }
    }
  }
}

bool add_notify(object* o, char* notifyuid){ // Persist?? // REVISIT Yess!!
  if(list_vals_set_add(o->notifies, notifyuid)) return true;
  log_write("****** can't add notify uid %s to %s\n", notifyuid, value_string(o->uid));
  show_notifies(o); // REVISIT: drop all this and bool return with expanding lists
  return false;
}

void save_and_notify(object* o){

  if(object_is_local(o)){
    int32_t v = value_int32(o->version);
    value_free(o->version);
    o->version = value_new_fmt("%ld", v+1);
  }

  persist_put(o, false);

  for(int i=1; i<=list_size(o->notifies); i++){
    set_to_notify(list_get_n(o->notifies,i), 0, o->uid, 0);
  }
}

void show_notifies(object* o) {
  log_write("notifies of %s\n", value_string(o->uid));
  for(int i=1; i<=list_size(o->notifies); i++){
    log_write("%s ", value_string(list_get_n(o->notifies,i)));
  }
  log_write("\n--------------\n");
}

// ------------------------------------------------------

#define BUFCHK if(ln>=s){ *(b+s-1) = 0; return b; }

char* object_to_text(object* n, char* b, uint16_t s, int target) {

  if(!n){ *b = 0; return b; }

  int ln=0;

  // UID, Ver, Cache must be first in that order
  ln+=snprintf(b+ln, s-ln, "UID: %s", value_string(n->uid)); BUFCHK
  ln+=snprintf(b+ln, s-ln, " Ver: %s", value_string(n->version)); BUFCHK

  if(n->cache && target!=OBJECT_TO_TEXT_NETWORK){
    ln+=snprintf(b+ln, s-ln, " Cache: %s", value_string(n->cache)); BUFCHK
  }

  if(n->persist && target!=OBJECT_TO_TEXT_NETWORK){
    ln+=snprintf(b+ln, s-ln, " Persist: %s", value_string(n->persist)); BUFCHK
  }

  if(target==OBJECT_TO_TEXT_NETWORK){
    ln+=snprintf(b+ln, s-ln, " Devices: %s", value_string(onex_device_object->uid)); BUFCHK
  }

  if(n->devices && list_size(n->devices) && target!=OBJECT_TO_TEXT_NETWORK){
    ln+=snprintf(b+ln, s-ln, " Devices: %s", value_string(list_get_n(n->devices, 1))); BUFCHK
  } // REVISIT list_to_text()

  if(n->evaluator && target!=OBJECT_TO_TEXT_NETWORK){
    ln+=snprintf(b+ln, s-ln, " Eval: %s", value_string(n->evaluator)); BUFCHK
  }

  int j;
  for(j=1; j<=list_size(n->notifies); j++){
    if(j==1) ln+=snprintf(b+ln, s-ln, " Notify:"); BUFCHK
    ln+=snprintf(b+ln, s-ln, " "); BUFCHK
    ln+=strlen(value_to_text(list_get_n(n->notifies,j), b+ln, s-ln)); BUFCHK
  } // REVISIT list_to_text()

  if(n->alerted && target==OBJECT_TO_TEXT_LOG){
    ln+=snprintf(b+ln, s-ln, " Alerted: %s", value_string(n->alerted)); BUFCHK
  }

  if(n->timer && target==OBJECT_TO_TEXT_LOG){
    ln+=snprintf(b+ln, s-ln, " Timer: %s", value_string(n->timer)); BUFCHK
  }

  properties* p=n->properties;
  for(j=1; j<=properties_size(p); j++){
    ln+=snprintf(b+ln, s-ln, " "); BUFCHK
    ln+=snprintf(b+ln, s-ln, "%s", properties_key_n(p,j)); BUFCHK
    ln+=snprintf(b+ln, s-ln, ": "); BUFCHK
    ln+=strlen(item_to_text(properties_get_n(p,j), b+ln, s-ln)); BUFCHK
  }

  return b;
}

char* object_uid_to_text(char* uid, char* b, uint16_t s, int style){
  return object_to_text(onex_get_from_cache(uid), b, s, style);
}

char* observe_uid_to_text(char* uid, char* b, uint16_t s){
  char* deviceuid = object_property(onex_device_object, "UID");
  snprintf(b, s, "OBS: %s Devices: %s", uid, deviceuid);
  return b;
}

void object_log(object* o) {
  char buff[MAX_TEXT_LEN];
  log_write("{ %s }\n", object_to_text(o,buff,MAX_TEXT_LEN,OBJECT_TO_TEXT_LOG));
}

// -----------------------------------------------------------------------

void onex_init(properties* config) { // REVISIT: onn_init()?

  log_write("Initialising Onex...\n");

  char* dbpath   =value_string(properties_get(config, "dbpath"));
  test_uid_prefix=value_string(properties_get(config, "test-uid-prefix"));

  timer_init();
  persist_init(dbpath);
  device_init();
  onp_init(config);
}

#define lOG_ONEX_LOOP
bool onex_loop() {
  bool ska=false, lka=false, pka=false, oka=false, eka=false;
#if defined(LOG_ONEX_LOOP)
  uint32_t zt=time_ms();
#endif
#if defined(NRF5)
  ska = serial_loop();
#if defined(LOG_ONEX_LOOP)
  uint32_t st=time_ms();
#endif

  lka = log_loop();
#if defined(LOG_ONEX_LOOP)
  uint32_t lt=time_ms();
#endif
#endif
  pka = persist_loop();
#if defined(LOG_ONEX_LOOP)
  uint32_t pt=time_ms();
#endif

  oka = onp_loop();
#if defined(LOG_ONEX_LOOP)
  uint32_t ot=time_ms();
#endif

  eka = run_any_evaluators();
#if defined(LOG_ONEX_LOOP)
  uint32_t et=time_ms();
#endif

#if defined(LOG_ONEX_LOOP)
  log_write("s=%ld l=%ld p=%ld o=%ld e=%ld\n", st-zt, lt-st, pt-lt, ot-pt, et-ot);
  if(ska) log_write("keep awake by serial_loop\n");
  if(lka) log_write("keep awake by log_loop\n");
  if(pka) log_write("keep awake by persist_loop\n");
  if(oka) log_write("keep awake by onp_loop\n");
  if(eka) log_write("keep awake by run_any_evaluators\n");
#endif
  return ska||lka||pka||oka||eka;
}

static properties* objects_cache=0;

bool add_to_cache(object* n) {
  if(!objects_cache) objects_cache=properties_new(MAX_OBJECTS);
  if(!properties_set(objects_cache, value_string(n->uid), n)){
    log_flash(1,0,0);
    log_write("No more room for objects!!\n");
    return false;
  }
  return true;
}

object* onex_get_from_cache(char* uid) {

  if(!uid || !(*uid)) return 0;

  object* o=properties_get(objects_cache, uid);
  if(o) return o;

  if(!persistence_objects_text) return 0;
  char* text=properties_get(persistence_objects_text, uid);
  if(!text) return 0; // <-- now pull from persistence!

  o=object_from_text(text, true, MAX_OBJECT_SIZE);

  if(!o || !add_to_cache(o)){
    object_free(o);
    return 0;
  }
  mem_freestr(properties_delete(persistence_objects_text, uid));
  return o;
}

bool add_to_cache_and_persist(object* n) {
  if(!add_to_cache(n)) return false;
  persist_put(n, true);
  return true;
}

void onex_show_cache() {
  log_write("+-----------cache dump------------\n");
  char buff[MAX_TEXT_LEN*8];
  for(int n=1; n<=properties_size(objects_cache); n++){
    object* o=properties_get_n(objects_cache,n);
    log_write("| %s\n", object_to_text(o,buff,MAX_TEXT_LEN*8,OBJECT_TO_TEXT_LOG));
  }
  log_write("+---------------------------------\n");
}

void onex_un_cache(char* uid) {

  // currently only exists to give some testability of persistence
  // but still hides a text serialisation instead of using backing store
  persist_flush();
  if(!uid || !(*uid)) return;

  object* o=properties_delete(objects_cache, uid);
  if(o){
    char buff[MAX_TEXT_LEN];
    char* text=object_to_text(o,buff,MAX_TEXT_LEN,OBJECT_TO_TEXT_PERSIST);
    mem_freestr(properties_delete(persistence_objects_text, uid));
    properties_set(persistence_objects_text, uid, mem_strdup(text));
    object_free(o);
  }

  //?? tests depend on this - uid-1 being bounced right back in again
  persist_pull_keep_active();
}

static properties* evaluators=0;

#define MAX_EVALS 7

void onex_set_evaluators(char* name, ...) {
  if(!evaluators) evaluators = properties_new(32);
  list* evals = (list*)properties_get(evaluators, name);
  if(!evals){
    evals = list_new(MAX_EVALS);
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
  if(!o) return;
  set_to_notify(o->uid, data, 0, 0);
}

void run_evaluators(object* o, void* data, value* alerted, bool timedout){
  if(!o || !o->evaluator) return;
  if(o->running_evals){
    IN_EVAL(o);
    return;
  }
  o->running_evals=true;
  o->alerted=alerted? alerted: 0;
  if(timedout) object_property_set(o, "Timer", "0");
  list* evals = (list*)properties_get(evaluators, value_string(o->evaluator));
  for(int i=1; i<=list_size(evals); i++){
    onex_evaluator eval=(onex_evaluator)list_get_n(evals, i);
    if(!eval(o, data)) break;
  }
  o->alerted=0;
  o->running_evals=false;
}

// -----------------------------------------------------------------------

static properties* objects_to_save=0;

void persist_init(char* dbpath){
  objects_to_save=properties_new(MAX_OBJECTS);
  persistence_init(dbpath);
  persist_pull_keep_active();
}

static uint32_t lasttime=0;

#if defined(NRF5)
#define FLUSH_RATE_MS 200
#else
#define FLUSH_RATE_MS 100
#endif

bool persist_loop() {

  if(!objects_to_save) return false;
  uint64_t curtime = time_ms();
  if(curtime > lasttime+FLUSH_RATE_MS){
    persist_flush();
    lasttime = curtime;
  }
  return false;
}

void persist_put(object* o, bool saving_metadata) {

  if(!objects_to_save) return;

  char* uid=value_string(o->uid);
  if(!saving_metadata){
    char* p=object_get_persist(o);
    if(p && !strcmp(p, "none")) return;
  }
  properties_set(objects_to_save, uid, uid);
}

void persist_flush() {

  if(!objects_to_save) return;

  char buff[MAX_TEXT_LEN];
  uint16_t sz=properties_size(objects_to_save);
  if(!sz) return;
  for(int j=1; j<=sz; j++){
    char* uid=properties_get_n(objects_to_save, j);
    object* o=onex_get_from_cache(uid);
    char* text=object_to_text(o,buff,MAX_TEXT_LEN,OBJECT_TO_TEXT_PERSIST);
    persistence_put(uid, text);
  }
  properties_clear(objects_to_save, false);
}

void persist_pull_keep_active() {
  if(!persistence_objects_text) return;
  for(int n=1; n<=properties_size(persistence_objects_text); n++){
    char* uid=0;
    char* p=properties_get_n(persistence_objects_text, n);
    char* key=0; char* val=0;
    #define FREE_BREAK_2 { mem_freestr(key); mem_freestr(val); break; }
    while(true){
      key=get_key(&p); if(!key) break; if(!*key) FREE_BREAK_2;
      val=get_val(&p); if(!val     ||     !*val) FREE_BREAK_2;
      if(!isupper((unsigned char)(*key)))        FREE_BREAK_2;
      if(!strcmp(key,"Cache") && !strcmp(val,"keep-active")){
        uid=properties_key_n(persistence_objects_text, n);
        FREE_BREAK_2;
      }
      mem_freestr(key); key=0;
      mem_freestr(val); val=0;
    }
    if(uid){
      object* o=onex_get_from_cache(uid);
      if(object_is_local_device(o)) onex_device_object = o;
      set_to_notify(o->uid, 0, 0, 0);
    }
  }
}

// -----------------------------------------------------------------------

void onn_recv_observe(observe obs){
  object* o=find_object(obs.uid, obs.dev, true);
  if(o && !object_is_shell(o)) onp_send_object(obs.uid, obs.dev);
  mem_freestr(obs.uid);
  mem_freestr(obs.dev);
  // REVISIT: and call the evaluator! fetching from ONP should run evaluator for
  // freshness as well as returning current state
}

void onn_recv_object(object* n) {
  object* o=onex_get_from_cache(value_string(n->uid));
  if(!o){
    if(!add_to_cache(n)){ object_free(n); return; }
    o=n;
  }
  else{
    if(value_equal(o->version, n->version) || value_num_greater(o->version, n->version)){
#ifdef LOG_VER_TEST
      log_write("Ver %s <= %s\n", value_string(n->uid), value_string(n->version));
#endif
      object_free(n);
      return;
    }
    #define ITEM_SWAP(o,n,i) item_free(o->i); o->i = n->i; n->i = 0;
    ITEM_SWAP(o,n,version);
    ITEM_SWAP(o,n,properties);
    item_free(list_vals_del(o->devices, "shell"));
    list_vals_set_add_all(o->devices,  n->devices);  list_free(n->devices,  false); n->devices=0;
    list_vals_set_add_all(o->notifies, n->notifies); list_free(n->notifies, false); n->notifies=0;
    object_free(n);
  }
  if(object_is_remote_device(o)){
    add_notify(o, value_string(onex_device_object->uid));
    add_notify(onex_device_object, value_string(o->uid));
  }
  save_and_notify(o);
}

// -----------------------------------------------------------------------

