
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>

#if defined(NRF5)
#include <app_util_platform.h>
#else
#include <pthread.h>
#endif

#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>
#include <onex-kernel/random.h>
#include <onex-kernel/time.h>
#if defined(HAS_SERIAL)
#include <onex-kernel/serial.h>
#endif
#include <items.h>

#include <onp.h>
#include <onn.h>

// ---------------------------------------------------------------------------------


#define MAX_UID_LEN 128
#if defined(NRF5)
#define MAX_LIST_SIZE 256
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

// ---------------------------------------------------------------------------------

static value*  generate_uid();
static char*   get_key(char** p);
static char*   get_val(char** p);
static bool    add_to_cache(object* n);
static bool    add_to_cache_and_persist(object* n);
static object* find_object(char* uid, object* n, bool observe);
static item*   property_item(object* n, char* path, object* t, bool observe);
static item*   nested_property_item(object* n, char* path, object* t, bool observe);
static bool    nested_property_set(object* n, char* path, char* val);
static bool    nested_property_set_n(object* n, char* path, uint16_t index, char* val);
static bool    nested_property_del(object* n, char* path);
static bool    nested_property_del_n(object* n, char* path, uint16_t index);
static bool    set_value_or_list(object* n, char* key, char* val);
static bool    add_notify(object* o, char* notify);
static void    set_notifies(object* o, char* notify);
static void    save_and_notify(object* n);
static bool    has_notifies(object* o);
static void    show_notifies(object* o);
static object* new_object(value* uid, char* evaluator, char* is, uint8_t max_size);
static object* new_object_from(char* text, uint8_t max_size);
static object* new_shell(value* uid);
static bool    is_shell(object* o);
static void    run_evaluators(object* o, void* data, value* alerted, bool timedout);
static bool    run_any_evaluators();
static void    set_to_notify(value* uid, void* data, value* alerted, uint64_t timeout);

static void    device_init();

static void    persistence_init(char* filename);
static bool    persistence_loop();
static object* persistence_get(char* uid);
static void    persistence_put(object* o);
static void    persistence_flush();
static void    scan_objects_text_for_keep_active();

static void    timer_init();

// ---------------------------------

typedef struct object {
  value*      uid;
  value*      evaluator;
  properties* properties;
  value*      cache;
  value*      notify[OBJECT_MAX_NOTIFIES];
  value*      alerted;
  value*      devices;
  value*      timer;
  bool        running_evals;
  uint64_t    last_observe;
} object;

// ---------------------------------

object* onex_device_object=0;

// ---------------------------------

char* find_unescaped_colon(char* p){
  char* c;
  char* t=p;
  do {
    c=strchr(t,':');
    if(!c) return 0;
    if(c && (c==p || (*(c-1)) != '\\')) return c;
    t=c+1;
  } while(*t);
  return 0;
}

char* remove_char_in_place(char* p, char c) {
  char* pr=p;
  char* pw=p;
  while(*pr){ *pw=*pr; pw+=(*pw!=c); pr++; }
  *pw=0;
  return p;
}

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
  object* n=(object*)mem_alloc(sizeof(object));
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
    char* key=get_key(&p); if(!key) break;                   if(!*key){ mem_freestr(key); object_free(n); n=0; break; }
    char* val=get_val(&p); if(!val || !*val){ mem_freestr(key); if(val) mem_freestr(val); object_free(n); n=0; break; }
    if(!strcmp(key,"UID")) uid=value_new(val);
    else
    if(!strcmp(key,"Eval")) evaluator=value_new(val);
    else
    if(!strcmp(key,"Devices")) devices=value_new(val);
    else
    if(!strcmp(key,"Cache")) cache=value_new(val);
    else
    if(!strcmp(key,"Notify")) notify=mem_strdup(val);
    else
    if(isupper((unsigned char)(*key)));
    else {
      if(!n){
        n=new_object(uid, 0, 0, max_size);
        if(evaluator) n->evaluator=evaluator;
        if(devices) n->devices=devices;
        if(cache) n->cache=cache;
        if(notify){ set_notifies(n, notify); mem_freestr(notify); }
      }
      if(!set_value_or_list(n, key, val)) break;
    }
    mem_freestr(key); mem_freestr(val);
  }
  return n;
}

object* new_shell(value* uid)
{
  object* n=(object*)mem_alloc(sizeof(object));
  n->uid=uid;
  n->properties=properties_new(MAX_OBJECT_SIZE);
  n->devices=value_new("shell");
  n->last_observe = 0;
  return n;
}

void object_free(object* o)
{
  if(!o) return;
  value_free(o->uid);
  value_free(o->evaluator);
  properties_free(o->properties, true);
  value_free(o->cache);
  for(uint8_t i=0; i< OBJECT_MAX_NOTIFIES; i++) value_free(o->notify[i]);
  value_free(o->alerted);
  value_free(o->devices);
  value_free(o->timer);
  mem_free(o);
}

char* get_key(char** p)
{
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
  if(!strcmp(path, "Timer")) return value_string(n->timer);
  item* i=property_item(n,path,n,observe);
  if(i && i->type==ITEM_VALUE) return value_string((value*)i);
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
  if(!strcmp(path, "Timer"))   return (item*)n->timer;
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
  bool observe2=observe && !isupper((unsigned char)(*p));
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
    char* e; uint16_t in=(uint16_t)strtol(c,&e,10);
    if(!(*e==0 || *e==':')) return 0;
    item* r=list_get_n((list*)i,in);
    if(!(r && r->type==ITEM_VALUE && *e==':')) return r;
    char* uid=value_string((value*)r);
    object* o=find_object(uid,t,observe2);
    return o? property_item(o,e+1,t,observe2): 0;
  }
  return 0;
}

char* channel_of(value* device_uid)
{
  return "serial";
}

void obs_or_refresh(char* uid, object* o, uint32_t timeout)
{
  uint64_t curtime = time_ms();
  if(!o->last_observe || curtime > o->last_observe + timeout){
    o->last_observe = curtime + 1;
    onp_send_observe(uid, channel_of(o->devices));
  }
}

object* find_object(char* uid, object* n, bool observe)
{
  if(!is_uid(uid) || !n) return 0;

  object* o=onex_get_from_cache(uid);

  if(!o){
    o=new_shell(value_new(uid));
    add_to_cache_and_persist(o);
  }
  if(observe){
    add_notify(o, value_string(n->uid));
    if(is_shell(o)){
      obs_or_refresh(uid, o, 1000);
    }
    else
    if(object_is_remote(o)){
      obs_or_refresh(uid, o, 10000);
    }
  }
  return is_shell(o)? 0: o;
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

uint16_t object_pathpair_length(object* n, char* path1, char* path2){

  char pathbuf[MAX_TEXT_LEN];
  snprintf(pathbuf, MAX_TEXT_LEN, "%s:%s", path1, path2);

  return object_property_length(n, pathbuf);
}

char* object_property_get_n(object* n, char* path, uint16_t index)
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

char* object_pathpair_get_n(object* n, char* path1, char* path2, uint16_t index){

  char pathbuf[MAX_TEXT_LEN];
  snprintf(pathbuf, MAX_TEXT_LEN, "%s:%s", path1, path2);

  return object_property_get_n(n, pathbuf, index);
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

bool object_pathpair_is(object* n, char* path1, char* path2, char* expected){

  char pathbuf[MAX_TEXT_LEN];
  snprintf(pathbuf, MAX_TEXT_LEN, "%s:%s", path1, path2);

  return object_property_is_observe(n, pathbuf, expected, true);
}

static bool object_property_contains_observe(object* n, char* path, char* expected, bool observe)
{
  if(!n) return false;
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

bool object_pathpair_contains(object* n, char* path1, char* path2, char* expected){

  char pathbuf[MAX_TEXT_LEN];
  snprintf(pathbuf, MAX_TEXT_LEN, "%s:%s", path1, path2);

  return object_property_contains_observe(n, pathbuf, expected, true);
}

bool object_property_contains_peek(object* n, char* path, char* expected)
{
  return object_property_contains_observe(n, path, expected, false);
}

// ---------------------------------------------------------------------------------


static uint16_t timer_id;

void just_wake_up(){}

void timer_init()
{
  timer_id=time_timeout(just_wake_up);
}

bool set_timer(object* n, char* timer)
{
  value_free(n->timer);
  n->timer=value_new(timer);
  char* e; uint32_t tm=strtol(timer,&e,10);
  if(*e) return false;
  set_to_notify(n->uid, 0, 0, time_ms()+tm);
  return true;
}

bool zero_timer(object* n)
{
  value_free(n->timer);
  n->timer=value_new("0");
  save_and_notify(n);
  return true;
}

bool stop_timer(object* n)
{
  value_free(n->timer);
  n->timer=0;
  save_and_notify(n);
  return true;
}

bool object_property_set(object* n, char* path, char* val)
{
  if(!n->running_evals && has_notifies(n)){
#if defined(LOG_TO_GFX) || defined(LOG_TO_BLE)
    char* uid=value_string(n->uid);
    log_write("N!%s %s %s %s", uid+4+15, object_property(n, "is"), path, val);
#else
    log_write("----\n"
              "Setting property in an object but not running in an evaluator!\n"
              "uid: %s is: %s %s: '%s'\n"
              "----\n",
              value_string(n->uid), object_property(n, "is:1"), path, val? val: "");
#endif
  }
  bool del=(!val || !*val);
  if(!strcmp(path, "Timer")){
    bool zero=val && !strcmp(val, "0");
    if(zero) return zero_timer(n);
    if(del) return stop_timer(n);
    return set_timer(n, val);
  }
  size_t m=strlen(path)+1;
  char p[m]; memcpy(p, path, m);
  char* c=strrchr(p, ':');
  if(del){
    if(c) return nested_property_del(n, path);
    item* i=properties_delete(n->properties, p);
    item_free(i);
    bool ok=!!i;
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
  if(!ok) list_free(l, true);
  return ok;
}

bool nested_property_set(object* n, char* path, char* val) {
  return nested_property_set_n(n, path, 0, val);
}

bool nested_property_set_n(object* n, char* path, uint16_t index, char* val) {

  size_t m=strlen(path)+1;
  char p[m]; memcpy(p, path, m);
  char* c=0;
  if(!index){
    c=strchr(p, ':');
    *c=0; c++;
  }
  item* i=property_item(n,p,0,true);
  bool ok=false;
  if(i) switch(i->type){
    case ITEM_VALUE: {
      if((index && index==1) || (c && !strcmp(c,"1"))){
        ok=set_value_or_list(n, p, val); // not single
      }
      break;
    }
    case ITEM_LIST: {
      char* e; uint16_t in=index? index: (uint16_t)strtol(c,&e,10);
      list* l=(list*)i;
      item_free(list_get_n(l, in));
      ok=list_set_n(l, in, value_new(val)); // not single
      break;
    }
    case ITEM_PROPERTIES: {
      break;
    }
  }
  if(ok) save_and_notify(n);
  return ok;
}

bool nested_property_del(object* n, char* path)
{
  return nested_property_del_n(n, path, 0);
}

bool nested_property_del_n(object* n, char* path, uint16_t index) {

  size_t m=strlen(path)+1;
  char p[m]; memcpy(p, path, m);
  char* c=0;
  if(!index){
    c=strchr(p, ':');
    *c=0; c++;
  }
  item* i=property_item(n,p,0,true);
  bool ok=false;
  if(i) switch(i->type){
    case ITEM_VALUE: {
      if((index && index==1) || (c && !strcmp(c,"1"))){
        item* i=properties_delete(n->properties, p);
        item_free(i);
        ok=!!i;
      }
      break;
    }
    case ITEM_LIST: {
      char* e; uint16_t in=index? index: (uint16_t)strtol(c,&e,10);
      list* l=(list*)i;
      item* i=list_del_n(l, in);
      item_free(i);
      ok=!!i;
      if(!ok) break;
      if(list_size(l)==1){
        properties_set(n->properties, p, list_get_n(l,1));
        list_free(l, false);
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
#if defined(LOG_TO_GFX) || defined(LOG_TO_BLE)
    char* uid=value_string(n->uid);
    log_write("N+%.*s", 12, uid+4);
#else
    log_write("\nSetting property in an object but not running in an evaluator! uid: %s  %s: +'%s'\n\n", value_string(n->uid), path, val? val: "");
#endif
  }
  if(strchr(path, ':')) return false; // no sub-properties yet
  if(!val || !*val) return 0;
  if(!strcmp(path, "Notifying")){
    if(!is_uid(val)) return false;
    add_notify(n, val);
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

bool object_property_set_fmt(object* n, char* path, char* fmt, ... /* <any> val, ... */){

  char valbuf[MAX_TEXT_LEN];
  va_list args;
  va_start(args, fmt);
  vsnprintf(valbuf, MAX_TEXT_LEN, fmt, args);
  va_end(args);

  return object_property_set(n, path, valbuf);
}

bool object_property_set_n(object* n, char* path, uint16_t index, char* val){

  if(!n->running_evals && has_notifies(n)){
#if defined(LOG_TO_GFX) || defined(LOG_TO_BLE)
    char* uid=value_string(n->uid);
    log_write("N!%.*s", 12, uid+4);
#else
    log_write("\nSetting property in an object but not running in an evaluator! uid: %s  %s: +'%s'\n\n", value_string(n->uid), path, val? val: "");
#endif
  }
  bool del=(!val || !*val);
  if(del) return nested_property_del_n(n, path, index);
  ;       return nested_property_set_n(n, path, index, val);
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

void start_timer_for_soonest_timeout_if_in_future()
{
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

void set_to_notify(value* uid, void* data, value* alerted, uint64_t timeout)
{
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

bool run_any_evaluators()
{
//if(highest_to_notify < 0) return false;

  bool keep_awake=false;

  uint64_t curtime=time_ms();

  for(uint16_t n=0; n< MAX_TO_NOTIFY; n++){

    uint8_t type=to_notify[n].type;

    if(type==TO_NOTIFY_FREE) continue;
    if(type==TO_NOTIFY_TIMEOUT && to_notify[n].details.timeout>curtime) continue;

    keep_awake=true;

    char* uid_or_channel=value_string(to_notify[n].uid);
    object* o=onex_get_from_cache(uid_or_channel);

    switch(type){
      case(TO_NOTIFY_NONE): {
        to_notify[n].type=TO_NOTIFY_FREE;
        run_evaluators(o, 0, 0, false);
        break;
      }
      case(TO_NOTIFY_DATA): {
        to_notify[n].type=TO_NOTIFY_FREE;
        run_evaluators(o, to_notify[n].details.data, 0, false);
        break;
      }
      case(TO_NOTIFY_ALERTED): {
        to_notify[n].type=TO_NOTIFY_FREE;
        if(o) run_evaluators(o, 0, to_notify[n].details.alerted, false);
        else{
          object* a=onex_get_from_cache(value_string(to_notify[n].details.alerted));
          onp_send_object(a, uid_or_channel);
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

bool add_notify(object* o, char* notify)
{
  int i;
  for(i=0; i< OBJECT_MAX_NOTIFIES; i++){
    if(o->notify[i] && value_is(o->notify[i], notify)) return true;
  }
  for(i=0; i< OBJECT_MAX_NOTIFIES; i++){
    if(!o->notify[i]){ o->notify[i]=value_new(notify); return true; }
  }
  log_write("can't add notify %s to %s\n", notify, value_string(o->uid));
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
  list_free(li, false);
}

void save_and_notify(object* o)
{
  persistence_put(o);

  for(int i=0; i< OBJECT_MAX_NOTIFIES; i++){
    value* notifyuid=o->notify[i];
    if(!notifyuid) continue;
    object* n=onex_get_from_cache(value_string(notifyuid));
    value* uid_or_channel=(n && !object_is_remote(n))? notifyuid: value_new(channel_of(notifyuid));
    set_to_notify(uid_or_channel, 0, o->uid, 0);
  }
  if(object_is_remote_device(o)){
    set_to_notify(onex_device_object->uid, 0, o->uid, 0);
  }
  if(o==onex_device_object){
    set_to_notify(value_new("all-channels"), 0, o->uid, 0);
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

char* object_to_text(object* n, char* b, uint16_t s, int target)
{
  if(!n){ *b = 0; return b; }

  int ln=0;

  ln+=snprintf(b+ln, s-ln, "UID: %s", value_string(n->uid));
  if(ln>=s){ *b = 0; return b; }

  if(target==OBJECT_TO_TEXT_NETWORK){
    ln+=snprintf(b+ln, s-ln, " Devices: %s", value_string(onex_device_object->uid));
    if(ln>=s){ *b = 0; return b; }
  }

  if(n->evaluator && target>=OBJECT_TO_TEXT_PERSIST){
    ln+=snprintf(b+ln, s-ln, " Eval: %s", value_string(n->evaluator));
    if(ln>=s){ *b = 0; return b; }
  }

  if(n->devices && target>=OBJECT_TO_TEXT_PERSIST){
    ln+=snprintf(b+ln, s-ln, " Devices: %s", value_string(n->devices));
    if(ln>=s){ *b = 0; return b; }
  }

  if(n->cache && target>=OBJECT_TO_TEXT_PERSIST){
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

  if(n->alerted && target>=OBJECT_TO_TEXT_PERSIST){
    ln+=snprintf(b+ln, s-ln, " Alerted: %s", value_string(n->alerted));
    if(ln>=s){ *b = 0; return b; }
  }

  if(n->timer && target==OBJECT_TO_TEXT_LOG){
    ln+=snprintf(b+ln, s-ln, " Timer: %s", value_string(n->timer));
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
  timer_init();
  random_init();
  persistence_init(dbpath);
  device_init();
  onp_init();
}

bool onex_loop()
{
  bool ska=false, lka=false, pka=false, oka=false, eka=false;
#if defined(NRF5)
#if defined(HAS_SERIAL)
  ska = serial_loop();
#endif
  lka = log_loop();
#endif
  pka = persistence_loop();
  oka = onp_loop();
  eka = run_any_evaluators();
#if defined(LOG_KEEP_AWAKE)
  if(ska) log_write("keep awake by serial_loop");
  if(lka) log_write("keep awake by log_loop");
  if(pka) log_write("keep awake by persistence_loop");
  if(oka) log_write("keep awake by onp_loop");
  if(eka) log_write("keep awake by run_any_evaluators");
#endif
  return ska||lka||pka||oka||eka;
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

object* onex_get_from_cache(char* uid) // or persistence! and hide this
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
    evals = list_new(7);
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
  set_to_notify(o->uid, data, 0, 0);
}

void run_evaluators(object* o, void* data, value* alerted, bool timedout){
  if(!o || !o->evaluator) return;
  if(o->running_evals){
#if defined(LOG_TO_GFX) || defined(LOG_TO_BLE)
    char* uid=value_string(o->uid);
    log_write("E!%.*s", 12, uid+4);
#else
    log_write("Already in evaluators! %s\n", value_string(o->uid));
#endif
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

static properties* objects_text=0;
static properties* objects_to_save=0;

static FILE* db=0;

#if !defined(NRF5)
bool mkdir_p(char* filename)
{
  char* fn=mem_strdup(filename);
  char* s=fn;
  while((s=strchr(s+1, '/'))){
    *s=0;
    if(mkdir(fn, S_IRWXU) && errno != EEXIST) return false;
    *s='/';
  }
  mem_freestr(fn);
  return true;
}
#endif

void persistence_init(char* filename)
{
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
  char* alldbtext=mem_alloc(len*sizeof(char)+1);
  if(!alldbtext) {
    fclose(db); db=0;
    log_write("Can't allocate space for DB file %s\n", filename);
    return;
  }
  objects_text=properties_new(MAX_OBJECTS);
  objects_to_save=properties_new(MAX_OBJECTS);
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
        mem_freestr(properties_delete(objects_text, uid));
        properties_set(objects_text, uid, mem_strdup(text));
      }
    }
    text=strtok(0, "\n");
  }
  mem_free(alldbtext);
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

bool persistence_loop()
{
  if(!objects_to_save) return false;
  uint64_t curtime = time_ms();
  if(curtime > lasttime+100){
    persistence_flush();
    lasttime = curtime;
  }
  return false;
}

object* persistence_get(char* uid)
{
  if(!objects_text) return 0;
  char* text=properties_get(objects_text, uid);
  if(!text) return 0;
  object* o=new_object_from(text, MAX_OBJECT_SIZE);
  if(!o) return 0;
  if(!add_to_cache(o)) return 0;
  return o;
}

void persistence_put(object* o)
{
  if(!objects_to_save) return;
  value* uid=o->uid;
  properties_set(objects_to_save, value_string(uid), uid);
}

void persistence_flush()
{
  if(!objects_to_save) return;
  uint16_t sz=properties_size(objects_to_save);
  if(!sz) return;
  for(int j=1; j<=sz; j++){
    char* uid=value_string(properties_get_n(objects_to_save, j));
    object* o=onex_get_from_cache(uid);
    char buff[MAX_TEXT_LEN];
    char* text=object_to_text(o,buff,MAX_TEXT_LEN,OBJECT_TO_TEXT_PERSIST);
    mem_freestr(properties_delete(objects_text, uid));
    properties_set(objects_text, uid, mem_strdup(text));
    fprintf(db, "%s\n", text);
  }
  properties_clear(objects_to_save, false);
  fflush(db);
}

void scan_objects_text_for_keep_active()
{
  if(!objects_text) return;
  for(int n=1; n<=properties_size(objects_text); n++){
    char* uid=0;
    char* p=properties_get_n(objects_text, n);
    while(true){
      char* key=get_key(&p); if(!key) break;                   if(!*key){ mem_freestr(key); break; }
      char* val=get_val(&p); if(!val || !*val){ mem_freestr(key); if(val) mem_freestr(val); break; }
      if(!isupper((unsigned char)(*key))){
        mem_freestr(key); mem_freestr(val);
        break;
      }
      if(!strcmp(key,"Cache") && !strcmp(val,"keep-active")){
        uid=properties_key_n(objects_text, n);
        mem_freestr(key); mem_freestr(val);
        break;
      }
      mem_freestr(key); mem_freestr(val);
    }
    if(uid){
      object* o=onex_get_from_cache(uid);
      if(object_is_local_device(o)) onex_device_object = o;
      set_to_notify(o->uid, 0, 0, 0);
    }
  }
}

// -----------------------------------------------------------------------

void onn_recv_observe(char* text, char* channel)
{
  char* uid=strchr(text,':')+2;
  char* u=uid; while(*u > ' ') u++; *u=0;
  object* o=onex_get_from_cache(uid);
  if(!o) return;
  add_notify(o, "uid-of-device");
  onp_send_object(o, channel);
}

void onn_recv_object(char* text, char* channel)
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
    item_free(o->devices);
    o->properties = n->properties; n->properties=0;
    o->devices    = n->devices;    n->devices=0;
    // additional notifies: for(i=0; i< OBJECT_MAX_NOTIFIES; i++) n->notify[i];
    object_free(n);
  }
  save_and_notify(o);
}

// -----------------------------------------------------------------------

