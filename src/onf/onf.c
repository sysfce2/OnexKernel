
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
#include <onex-kernel/log.h>
#include <onex-kernel/random.h>
#include <onf.h>

// ---------------------------------------------------------------------------------


#if defined(NRF5)
#define MAX_LIST_SIZE 16
#define MAX_TEXT_LEN 256
#define MAX_OBJECTS 128
#define MAX_OBJECT_SIZE 8
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
static void        add_to_cache(object* n);
static void        add_to_cache_and_persist(object* n);
static object*     find_object(char* uid, object* n);
static item*       property_item(object* n, char* path, object* t);
static item*       nested_property_item(object* n, char* path, object* t);
static bool        nested_property_set(object* n, char* path, char* val);
static bool        nested_property_delete(object* n, char* path);
static properties* nested_properties(object* n, char* path);
static bool        set_value_or_list(object* n, char* key, char* val);
static bool        add_observer(object* o, value* notify);
static void        set_observers(object* o, char* notify);
static void        save_and_notify_observers(object* n);
static bool        has_notifies(object* o);
static void        show_notifies(object* o);
static object*     new_object(value* uid, char* evaluator, char* is, uint8_t max_size);
static object*     new_object_from(char* text, uint8_t max_size);
static object*     new_shell(value* uid, char* notify, value* remote);
static bool        is_shell(object* o);

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
  value*      remote;
  bool        running_evals;
  uint32_t    last_observe;
} object;

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

object* object_new_from(char* text, uint8_t max_size)
{
  object* n=new_object_from(text, max_size);
  if(!n) return 0;
  char* uid=value_string(n->uid);
  if(onex_get_from_cache(uid)){ log_write("Attempt to create an object with UID %s that already exists\n", uid); return 0; }
  add_to_cache_and_persist(n);
  return n;
}

object* object_new(char* uid, char* evaluator, char* is, uint8_t max_size)
{
  if(onex_get_from_cache(uid)){ log_write("Attempt to create an object with UID %s that already exists\n", uid); return 0; }
  object* n=new_object(value_new(uid), evaluator, is, max_size);
  add_to_cache_and_persist(n);
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
  value* remote=0;
  value* cache=0;
  char* notify=0;
  char* p=t;
  while(true){
    char* key=get_key(&p); if(!key) break;
    if(!*key){ free(key); key=strdup("--"); }
    char* val=get_val(&p); if(!val){ free(key); break; }
    if(!strcmp(key,"UID")) uid=value_new(val);
    else
    if(!strcmp(key,"Eval")) evaluator=value_new(val);
    else
    if(!strcmp(key,"Remote")) remote=value_new(val);
    else
    if(!strcmp(key,"Cache")) cache=value_new(val);
    else
    if(!strcmp(key,"Notify")) notify=strdup(val);
    else {
      if(!n){
        n=new_object(uid, 0, 0, max_size);
        if(evaluator) n->evaluator=evaluator;
        if(remote) n->remote=remote;
        if(cache) n->cache=cache;
        set_observers(n, notify);
        free(notify);
      }
      if(!set_value_or_list(n, key, val)) break;
    }
    free(key); free(val);
  }
  return n;
}

object* new_shell(value* uid, char* notify, value* remote)
{
  uint8_t max_size=4;
  object* n=(object*)calloc(1,sizeof(object));
  n->uid=uid;
  n->remote=remote;
  n->last_observe = 0;
  set_observers(n, notify);
  return n;
}

void object_free(object* o)
{
  item_free(o->properties);
  free(o);
}

bool is_shell(object* o)
{
  return o->remote && !o->properties;
}

bool object_is_local(char* uid)
{
  object* o=onex_get_from_cache(uid);
  return o && !o->remote;
}

char* get_key(char** p)
{
  if(!strlen(*p)) return 0;
  while(isspace(**p)) (*p)++;
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
  char* c=strstr(*p, ": ");
  while(c && *(c-1)=='\\') c=strstr(c+1, ": ");
  char* r=0;
  if(!c){
    char* s=strrchr(*p, 0);
    do s--; while(isspace(*s)); s++;
    (*s)=0;
    r=strdup(*p);
    (*p)+=strlen(*p)+1;
  }
  else{
    (*c)=0;
    char* s=strrchr(*p, ' ');
    do s--; while(isspace(*s)); s++;
    (*c)=':';
    (*s)=0;
    r=strdup(*p);
    (*s)=' ';
    (*p)=s+1;
  }
  int y=0;
  for(int x=0; x<strlen(r); x++){
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
  return n->cache && !strcmp(value_string(n->cache), "keep-active");
}

// ------------------------------------------------------

char* object_property(object* n, char* path)
{
  if(!n) return 0;
  if(!strcmp(path, "UID")) return value_string(n->uid);
  item* i=property_item(n,path,n);
  if(i && i->type==ITEM_VALUE) return value_string((value*)i);
  return 0;
}

char* object_property_values(object* n, char* path)
{
  item* i=property_item(n,path,n);
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

item* property_item(object* n, char* path, object* t)
{
  if(!strcmp(path, "UID")) return (item*)n->uid;
  if(!strcmp(path, ""))    return (item*)n->properties;
  if(!strcmp(path, ":"))   return (item*)n->properties;
  size_t m=strlen(path)+1;
  char p[m]; memcpy(p, path, m);
  char* c=strrchr(p, ':');
  bool endsincolon=(c && c+1-p==strlen(p));
  if(endsincolon){ *c=0; c=strrchr(p, ':'); }
  if(!c) return properties_get(n->properties, value_new(p));
  return nested_property_item(n, p, t);
}

item* nested_property_item(object* n, char* path, object* t)
{
  size_t m=strlen(path)+1;
  char p[m]; memcpy(p, path, m);
  char* c=strchr(p, ':');
  *c=0; c++;
  item* i=property_item(n,p,t);
  if(!i) return 0;
  if(i->type==ITEM_VALUE){
    char* uid=value_string((value*)i);
    bool looksLikeItIsIndexedOne=(*c=='1');
    if(looksLikeItIsIndexedOne){
      if(strlen(c)==1) return i;
      c+=2; // skip '1:' to next bit
    }
    if(is_uid(uid)){
      object* o=find_object(uid,t);
      return o? property_item(o,c,t): 0;
    }
    return 0;
  }
  if(i->type==ITEM_LIST){
    char* e; uint32_t index=strtol(c,&e,10);
    item* r= ((*e)==0 || (*e)==':')? list_get_n((list*)i,index): 0;
    if(r){
      if(r->type==ITEM_VALUE){
        char* uid=value_string((value*)r);
        if(is_uid(uid) && (*e)){
          object* o=find_object(uid,t);
          r= o? property_item(o, e+1, t): 0;
        }
      }
    }
    return r;
  }
  return 0;
}

object* find_object(char* uid, object* n)
{
  if(!is_uid(uid) || !n) return 0;
  object* o=onex_get_from_cache(uid);
  if(o && !is_shell(o)){
    add_observer(o,n->uid);
    return o;
  }
  if(!o){
    o=new_shell(value_new(uid), value_string(n->uid), value_new("Serial"));
    add_to_cache_and_persist(o);
  }
  uint32_t curtime = time_ms();
  if(!o->last_observe || curtime > o->last_observe + 1000){
    o->last_observe = curtime + 1;
    onp_send_observe(uid, value_string(o->remote));
  }
  return 0;
}

bool is_uid(char* uid)
{
  return uid && !strncmp(uid,"uid-",4);
}

uint16_t object_property_length(object* n, char* path)
{
  item* i=property_item(n,path,n);
  if(i){
    if(i->type==ITEM_VALUE) return 1;
    if(i->type==ITEM_PROPERTIES) return 1;
    if(i->type==ITEM_LIST)  return list_size((list*)i);
  }
  return 0;
}

char* object_property_get_n(object* n, char* path, uint8_t index)
{
  item* v=0;
  item* i=property_item(n,path,n);
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
  item* i=property_item(n,path,n);
  if(i){
    if(i->type==ITEM_PROPERTIES) return properties_size((properties*)i);
    if(i->type==ITEM_VALUE){
      char* uid=value_string((value*)i);
      if(is_uid(uid)){
        object* o=find_object(uid,n);
        if(o) return properties_size(o->properties);
      }
    }
  }
  return -1;
}

char* object_property_key(object* n, char* path, uint16_t index)
{
  value* k=0;
  item* i=property_item(n,path,n);
  if(!i) return 0;
  switch(i->type){
    case ITEM_PROPERTIES: {
      k=properties_key_n((properties*)i, index);
      break;
    }
    case ITEM_VALUE: {
      char* uid=value_string((value*)i);
      if(!is_uid(uid)) break;
      object* o=find_object(uid,n);
      if(o) k=properties_key_n(o->properties, index);
      break;
    }
    case ITEM_LIST: break;
  }
  return value_string(k);
}

char* object_property_val(object* n, char* path, uint16_t index)
{
  item* v=0;
  item* i=property_item(n,path,n);
  if(!i) return 0;
  switch(i->type){
    case ITEM_PROPERTIES: {
      v=properties_get_n((properties*)i, index);
      break;
    }
    case ITEM_VALUE: {
      char* uid=value_string((value*)i);
      if(!is_uid(uid)) break;
      object* o=find_object(uid,n);
      if(o) v=properties_get_n(o->properties, index);
      break;
    }
    case ITEM_LIST: break;
  }
  if(!(v && v->type==ITEM_VALUE)) return 0;
  return value_string((value*)v);
}

bool object_property_is(object* n, char* path, char* expected)
{
  if(!n) return false;
  if(!strcmp(path, "UID")){
    return expected && !strcmp(value_string(n->uid), expected);
  }
  item* i=property_item(n,path,n);
  if(!i) return (!expected || !*expected);
  if(i->type==ITEM_VALUE){
    return expected && !strcmp(value_string((value*)i), expected);
  }
  return false;
}

bool object_property_contains(object* n, char* path, char* expected)
{
  if(!n) return false;
  if(!strcmp(path, "UID")){
    return expected && !strcmp(value_string(n->uid), expected);
  }
  item* i=property_item(n,path,n);
  if(!i) return (!expected || !*expected);
  if(i->type==ITEM_VALUE){
    return expected && !strcmp(value_string((value*)i), expected);
  }
  if(i->type==ITEM_LIST){
    int j; int sz=list_size((list*)i);
    for(j=1; j<=sz; j++){
      item* y=list_get_n((list*)i, j);
      if(y->type!=ITEM_VALUE) continue;
      if(expected && !strcmp(value_string((value*)y), expected)) return true;
    }
    return false;
  }
  return false;
}

bool object_property_set(object* n, char* path, char* val)
{
  if(!n->running_evals && has_notifies(n)) log_write("\nNot running evaluators! uid: %s  %s: '%s'\n\n", value_string(n->uid), path, val? val: "");
  size_t m=strlen(path)+1;
  char p[m]; memcpy(p, path, m);
  char* c=strrchr(p, ':');
  bool endsincolon=(c && c+1-p==strlen(p));
  if(endsincolon){ *c=0; c=strrchr(p, ':'); }
  if(!val || !*val){
    if(c) return nested_property_delete(n, path);
    bool ok=!!properties_delete(n->properties, value_new(p));
    if(ok) save_and_notify_observers(n);
    return ok;
  }
  if(c) return nested_property_set(n, p, val);
  bool ok=set_value_or_list(n, p, val);
  if(ok) save_and_notify_observers(n);
  return ok;
}

bool set_value_or_list(object* n, char* key, char* val)
{
  if(!strchr(val, ' ') && !strchr(val, '\n')){
    return properties_set(n->properties, value_new(key), value_new(val));
  }
  list* l=list_new_from(val, MAX_LIST_SIZE);
  return properties_set(n->properties, value_new(key), l);
}

bool nested_property_set(object* n, char* path, char* val)
{
  size_t m=strlen(path)+1;
  char p[m]; memcpy(p, path, m);
  char* c=strchr(p, ':');
  *c=0; c++;
  item* i=property_item(n,p,0);
  bool ok=false;
  if(i) switch(i->type){
    case ITEM_VALUE: {
      if(!strcmp(c,"1")) ok=properties_set(n->properties, value_new(p), value_new(val)); // not singled like above fn
      break;
    }
    case ITEM_LIST: {
      char* e; uint32_t index=strtol(c,&e,10);
      ok=list_set_n((list*)i, index, value_new(val)); // not single
      break;
    }
    case ITEM_PROPERTIES: {
      break;
    }
  }
  if(ok) save_and_notify_observers(n);
  return ok;
}

bool nested_property_delete(object* n, char* path)
{
  size_t m=strlen(path)+1;
  char p[m]; memcpy(p, path, m);
  char* c=strchr(p, ':');
  *c=0; c++;
  item* i=property_item(n,p,0);
  bool ok=false;
  if(i) switch(i->type){
    case ITEM_VALUE: {
      if(!strcmp(c,"1")) ok=!!properties_delete(n->properties, value_new(p));
      break;
    }
    case ITEM_LIST: {
      char* e; uint32_t index=strtol(c,&e,10);
      list* l=(list*)i;
      ok=list_del_n(l, index);
      if(!ok) break;
      if(list_size(l)==1){
        properties_set(n->properties, value_new(p), list_get_n(l,1));
        list_free(l);
      }
      break;
    }
    case ITEM_PROPERTIES: {
      break;
    }
  }
  if(ok) save_and_notify_observers(n);
  return ok;
}

bool object_property_add(object* n, char* path, char* val)
{
  if(!n->running_evals && has_notifies(n)) log_write("\nNot running evaluators! uid: %s  %s: +'%s'\n\n", value_string(n->uid), path, val? val: "");
  if(strchr(path, ':')) return false; // no sub-properties yet
  if(!val || !*val) return 0;
  item* i=properties_get(n->properties, value_new(path));
  bool ok=true;
  if(!i){
    ok=properties_set(n->properties, value_new(path), value_new(val)); // not single
  }
  else
  switch(i->type){
    case ITEM_VALUE: {
      list* l=list_new(MAX_LIST_SIZE);
      ok=ok && list_add(l,i);
      ok=ok && list_add(l,value_new(val)); // not single
      ok=ok && properties_set(n->properties, value_new(path), l);
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
  if(ok) save_and_notify_observers(n);
  return ok;
}

// ------------------------------------------------------

bool add_observer(object* o, value* notify)
{
  int i;
  for(i=0; i< OBJECT_MAX_NOTIFIES; i++){
    if(o->notify[i] && !strcmp(value_string(o->notify[i]), value_string(notify))) return true;
  }
  for(i=0; i< OBJECT_MAX_NOTIFIES; i++){
    if(!o->notify[i]){ o->notify[i]=notify; return true; }
  }
  log_write("can't add observer %s to %s\n", value_string(notify), value_string(o->uid));
  show_notifies(o);
  return false;
}

void set_observers(object* o, char* notify)
{
  list* li=list_new_from(notify, OBJECT_MAX_NOTIFIES);
  if(!li) return;
  int i; for(i=0; i < list_size(li); i++){
    o->notify[i]=(value*)list_get_n(li, i+1);
  }
  list_free(li);
}

void save_and_notify_observers(object* o)
{
  persistence_put(o);
  int i;
  for(i=0; i< OBJECT_MAX_NOTIFIES; i++){
    if(!o->notify[i]) continue;
    char* notify = value_string(o->notify[i]);
    if(is_uid(notify)){
      onex_run_evaluators(notify, 0);
    }
    else{
      onp_send_object(o,notify);
    }
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

char* object_to_text(object* n, char* b, uint16_t s)
{
  if(!n){ *b = 0; return b; }

  int ln=0;

  ln+=snprintf(b+ln, s-ln, "UID: %s", value_string(n->uid));
  if(ln>=s){ *b = 0; return b; }

  if(n->evaluator){
    ln+=snprintf(b+ln, s-ln, " Eval: %s", value_string(n->evaluator));
    if(ln>=s){ *b = 0; return b; }
  }

  if(n->remote){
    ln+=snprintf(b+ln, s-ln, " Remote: %s", value_string(n->remote));
    if(ln>=s){ *b = 0; return b; }
  }

  if(n->cache){
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
  properties* p=n->properties;
  for(j=1; j<=properties_size(p); j++){
    ln+=snprintf(b+ln, s-ln, " ");
    if(ln>=s){ *b = 0; return b; }
    ln+=strlen(value_to_text(properties_key_n(p,j), b+ln, s-ln));
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
  log_write("{ %s }\n", object_to_text(o,buff,MAX_TEXT_LEN));
}

// -----------------------------------------------------------------------

void onex_init(char* dbpath)
{
  persistence_init(dbpath);
  onp_init();
}

bool first_time=true;

void onex_loop()
{
  persistence_loop();
  onp_loop();
}

static properties* objects_cache=0;

void add_to_cache(object* n)
{
  if(!objects_cache) objects_cache=properties_new(MAX_OBJECTS);
  properties_set(objects_cache, n->uid, n);
}

object* onex_get_from_cache(char* uid)
{
  if(!uid || !(*uid)) return 0;
  object* o=properties_get(objects_cache, value_new(uid));
  if(!o)  o=persistence_get(uid);
  return o;
}

void add_to_cache_and_persist(object* n)
{
  add_to_cache(n);
  persistence_put(n);
}

void onex_show_cache()
{
  log_write("+-----------cache dump------------\n");
  char buff[MAX_TEXT_LEN*8];
  for(int n=1; n<=properties_size(objects_cache); n++){
    object* o=properties_get_n(objects_cache,n);
    log_write("| %s\n", object_to_text(o,buff,MAX_TEXT_LEN*8));
  }
  log_write("+---------------------------------\n");
}

void onex_un_cache(char* uid)
{
  persistence_flush();
  if(!uid || !(*uid)) return;
  object* o=properties_delete(objects_cache, value_new(uid));
  object_free(o);
  scan_objects_text_for_keep_active();
}

static properties* evaluators=0;

void onex_set_evaluators(char* name, ...)
{
  if(!evaluators) evaluators = properties_new(32);
  list* evals = (list*)properties_get(evaluators, value_new(name));
  if(!evals){
    evals = list_new(6);
    properties_set(evaluators, value_new(name), evals);
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
  if(!o || !o->evaluator) return;
  if(o->running_evals){ log_write("Already in evaluators! %s\n", uid); return; }
  o->running_evals=true;
  list* evals = (list*)properties_get(evaluators, o->evaluator);
  for(int i=1; i<=list_size(evals); i++){
    onex_evaluator eval=(onex_evaluator)list_get_n(evals, i);
    if(!eval(o, data)) break;
  }
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
    if(strncmp(text, "UID: ", 5)) continue;
    char* uid=text+5;
    char* e=strchr(uid, ' ');
    if(e) *e=0;
    value* uidv=value_new(uid);
    if(e) *e=' ';
    char* prevtext=properties_delete(objects_text, uidv);
    if(prevtext) free(prevtext);
    properties_set(objects_text, uidv, strdup(text));
    text=strtok(0, "\n");
  }
  free(alldbtext);
  scan_objects_text_for_keep_active();
}

uint32_t lasttime=0;

void persistence_loop()
{
  uint32_t curtime = time_ms();
  if(curtime > lasttime+100){
    persistence_flush();
    lasttime = curtime;
  }
}

object* persistence_get(char* uid)
{
  value* uidv=value_new(uid);
  char* text=properties_get(objects_text, uidv);
  if(!text) return 0;
  object* o=new_object_from(text, MAX_OBJECT_SIZE);
  if(o) add_to_cache(o);
  return o;
}

void persistence_put(object* o)
{
  value* uidv=o->uid;
  properties_set(objects_to_save, uidv, uidv);
}

void persistence_flush()
{
  uint16_t sz=properties_size(objects_to_save);
  if(!sz) return;
  for(int j=1; j<=sz; j++){
    value* uidv=properties_get_n(objects_to_save, j);
    object* o=onex_get_from_cache(value_string(uidv));
    char buff[MAX_TEXT_LEN];
    char* text=object_to_text(o,buff,MAX_TEXT_LEN);
    free(properties_delete(objects_text, uidv));
    properties_set(objects_text, uidv, strdup(text));
    if(db) fprintf(db, "%s\n", text);
  }
  properties_clear(objects_to_save, false);
  if(db) fflush(db);
}

void scan_objects_text_for_keep_active()
{
  for(int n=1; n<=properties_size(objects_text); n++){
    value* uid=0;
    char* p=properties_get_n(objects_text, n);
    while(true){
      char* key=get_key(&p); if(!key) break; if(!*key){ free(key); break; }
      char* val=get_val(&p); if(!val){ free(key); break; }
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
      onex_run_evaluators(value_string(uid), 0);
    }
  }
}

// -----------------------------------------------------------------------

void recv_observe(char* b, char* from)
{
  char* uid=strchr(b,':')+2;
  char* u=uid; while(*u > ' ') u++; *u=0;
  object* o=onex_get_from_cache(uid);
  if(!o) return;
  add_observer(o, value_new(from));
  onp_send_object(o,from);
}

void recv_object(char* text)
{
  object* n=new_object_from(text, 4);
  if(!n) return;
  object* s=onex_get_from_cache(value_string(n->uid));
  if(!s) return;
  s->properties = n->properties;
  save_and_notify_observers(s);
}

// -----------------------------------------------------------------------

