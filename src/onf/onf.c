
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "onp.h"

#include <items.h>
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onex-kernel/random.h>
#include <onf.h>

// ---------------------------------------------------------------------------------

#define MAX_LIST_SIZE 64

#if defined(TARGET_MCU_NRF51822)
#define MAX_TEXT_LEN 128
#else
#define MAX_TEXT_LEN 2048
#endif

// ---------------------------------------------------------------------------------

static value*      generate_uid();
static char*       get_key(char** p);
static char*       get_val(char** p);
static void        add_to_cache(object* n);
static object*     find_object(char* uid, object* n);
static item*       object_property_item(object* n, char* path, object* t);
static item*       nested_property_item(object* n, char* path, object* t);
static bool        nested_property_set(object* n, char* path, char* val);
static bool        nested_property_delete(object* n, char* path);
static properties* nested_properties(object* n, char* path);
static bool        set_value_or_list(object* n, char* key, char* val);
static bool        add_observer(object* o, value* notify);
static void        set_observers(object* o, char* notify);
static void        notify_observers(object* n);
static void        show_notifies(object* o);
static void        call_all_evaluators();
static object*     new_shell(value* uid, char* notify);
static bool        is_shell(object* o);


// ---------------------------------

typedef struct object {
  value*          uid;
  onex_evaluator  evaluator;
  properties*     properties;
  value*          notify[OBJECT_MAX_NOTIFIES];
  uint32_t        last_observe;
  struct object*  next;
} object;

object* cache=0;

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

object* new_object(value* uid, char* is, onex_evaluator evaluator, uint8_t max_size)
{
  object* n=(object*)calloc(1,sizeof(object));
  n->uid=uid? uid: generate_uid();
  n->properties=properties_new(max_size);
  if(is) set_value_or_list(n, "is", is);
  n->evaluator=evaluator;
  return n;
}

object* new_object_from(char* text, onex_evaluator evaluator, uint8_t max_size)
{
  size_t m=strlen(text)+1;
  char t[m]; memcpy(t, text, m);
  object* n=0;
  value* uid=0;
  char* notify=0;
  char* p=t;
  while(true){
    char* key=get_key(&p); if(!key) break;
    char* val=get_val(&p); if(!val) break;
    if(!strcmp(key,"UID")) uid=value_new(val);
    else
    if(!strcmp(key,"Notify")) notify=strdup(val);
    else {
      if(!n){
        n=new_object(uid, 0, 0, max_size);
        set_observers(n, notify);
        free(notify);
      }
      if(!set_value_or_list(n, key, val)) break;
    }
    free(key); free(val);
  }
  n->evaluator=evaluator;
  return n;
}

object* object_new_from(char* text, onex_evaluator evaluator, uint8_t max_size)
{
  object* n=new_object_from(text, evaluator, max_size);
  char* uid=object_property(n, "UID");
  if(onex_get_from_cache(uid)){ log_write("Attempt to create an object with UID %s that already exists\n", uid); return 0; }
  add_to_cache(n);
  return n;
}

object* object_new(char* uid, char* is, onex_evaluator evaluator, uint8_t max_size)
{
  if(onex_get_from_cache(uid)){ log_write("Attempt to create an object with UID %s that already exists\n", uid); return 0; }
  object* n=new_object(value_new(uid), is, evaluator, max_size);
  add_to_cache(n);
  return n;
}

object* new_shell(value* uid, char* notify)
{
  uint8_t max_size=4;
  object* n=(object*)calloc(1,sizeof(object));
  n->uid=uid;
  n->last_observe = 0;
  add_to_cache(n);
  set_observers(n, notify);
  return n;
}

bool is_shell(object* o)
{
  return !o->properties && !o->evaluator;
}

bool object_is_local(char* uid)
{
  object* o=onex_get_from_cache(uid);
  return o && o->evaluator;
}

char* get_key(char** p)
{
  if(!strlen(*p)) return 0;
  char* s=strchr(*p, ' ');
  char* c=strchr(*p, ':');
  if(s<c) return 0;
  (*c)=0;
  char* r=strdup(*p);
  (*c)=':';
  (*p)=c+1;
  return r;
}

char* get_val(char** p)
{
  char* c=strstr(*p, ": ");
  char* r=0;
  if(!c){
    r=strdup(*p+1);
    (*p)+=strlen(*p+1)+1;
    return r;
  }
  (*c)=0;
  char* s=strrchr(*p, ' ');
  (*c)=':';
  (*s)=0;
  r=strdup(*p+1);
  (*s)=' ';
  (*p)=s+1;
  return r;
}

void object_set_evaluator(object* n, onex_evaluator evaluator)
{
  n->evaluator=evaluator;
}

// ------------------------------------------------------

char* object_property(object* n, char* path)
{
  if(!n) return 0;
  if(!strcmp(path, "UID")) return value_string(n->uid);
  item* i=object_property_item(n,path,n);
  uint16_t s=MAX_TEXT_LEN;
  char b[MAX_TEXT_LEN]; *b=0;
  if(strlen(item_to_text(i, b, MAX_TEXT_LEN))) return value_string(value_new(b)); // not single value
  return 0;
}

char* object_property_values(object* n, char* path)
{
  item* i=object_property_item(n,path,n);
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

item* object_property_item(object* n, char* path, object* t)
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
  item* i=object_property_item(n,p,t);
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
      return o? object_property_item(o,c,t): 0;
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
          r= o? object_property_item(o, e+1, t): 0;
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
  if(!o) o=new_shell(value_new(uid), value_string(n->uid));
  uint32_t curtime = time_ms();
  if(!o->last_observe || curtime > o->last_observe + 1000){
    o->last_observe = curtime + 1;
    onp_send_observe(uid, "");
  }
  return 0;
}

bool is_uid(char* uid)
{
  return uid && !strncmp(uid,"uid-",4);
}

uint16_t object_property_length(object* n, char* path)
{
  item* i=object_property_item(n,path,n);
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
  item* i=object_property_item(n,path,n);
  if(!i) return 0;
  switch(i->type){
    case ITEM_LIST: { v=list_get_n((list*)i,index); break; }
    case ITEM_VALUE: { if(index==1) v=i; break; }
    case ITEM_PROPERTIES: break;
  }
  if(!(v && v->type==ITEM_VALUE)) return 0;
  return value_string((value*)v);
}

int8_t object_property_size(object* n, char* path)
{
  item* i=object_property_item(n,path,n);
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

char* object_property_key(object* n, char* path, uint8_t index)
{
  value* k=0;
  item* i=object_property_item(n,path,n);
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

char* object_property_val(object* n, char* path, uint8_t index)
{
  item* v=0;
  item* i=object_property_item(n,path,n);
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
  char* v=object_property(n, path);
  return v? !strcmp(v, expected): !expected || !*expected;
}

bool object_property_set(object* n, char* path, char* val)
{
  size_t m=strlen(path)+1;
  char p[m]; memcpy(p, path, m);
  char* c=strrchr(p, ':');
  bool endsincolon=(c && c+1-p==strlen(p));
  if(endsincolon){ *c=0; c=strrchr(p, ':'); }
  if(!val || !*val){
    if(c) return nested_property_delete(n, path);
    bool ok=!!properties_delete(n->properties, value_new(p));
    if(ok) notify_observers(n);
    return ok;
  }
  if(c) return nested_property_set(n, p, val);
  bool ok=set_value_or_list(n, p, val);
  if(ok) notify_observers(n);
  return ok;
}

bool set_value_or_list(object* n, char* key, char* val)
{
  if(!strchr(val, ' ')){
    return properties_set(n->properties, value_new(key), (item*)value_new(val));
  }
  else{ // give to list type to parse!
    list* l=list_new(MAX_LIST_SIZE);
    size_t m=strlen(val)+1;
    char vals[m]; memcpy(vals, val, m);
    char* t=strtok(vals, " \n");
    while(t) {
      list_add(l,(item*)value_new(t));
      t=strtok(0, " \n");
    }
    return properties_set(n->properties, value_new(key), (item*)l);
  }
}

bool nested_property_set(object* n, char* path, char* val)
{
  size_t m=strlen(path)+1;
  char p[m]; memcpy(p, path, m);
  char* c=strchr(p, ':');
  *c=0; c++;
  item* i=object_property_item(n,p,0);
  bool ok=false;
  if(i) switch(i->type){
    case ITEM_VALUE: {
      if(!strcmp(c,"1")) ok=properties_set(n->properties, value_new(p), (item*)value_new(val)); // not singled like above fn
      break;
    }
    case ITEM_LIST: {
      char* e; uint32_t index=strtol(c,&e,10);
      ok=list_set_n((list*)i, index, (item*)value_new(val)); // not single
      break;
    }
    case ITEM_PROPERTIES: {
      break;
    }
  }
  if(ok) notify_observers(n);
  return ok;
}

bool nested_property_delete(object* n, char* path)
{
  size_t m=strlen(path)+1;
  char p[m]; memcpy(p, path, m);
  char* c=strchr(p, ':');
  *c=0; c++;
  item* i=object_property_item(n,p,0);
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
      }
      break;
    }
    case ITEM_PROPERTIES: {
      break;
    }
  }
  if(ok) notify_observers(n);
  return ok;
}

bool object_property_add(object* n, char* path, char* val)
{
  if(strchr(path, ':')) return false; // no sub-properties yet
  if(!val || !*val) return 0;
  item* i=properties_get(n->properties, value_new(path));
  bool ok=true;
  if(!i){
    ok=properties_set(n->properties, value_new(path), (item*)value_new(val)); // not single
  }
  else
  switch(i->type){
    case ITEM_VALUE: {
      list* l=list_new(MAX_LIST_SIZE);
      ok=ok && list_add(l,i);
      ok=ok && list_add(l,(item*)value_new(val)); // not single
      ok=ok && properties_set(n->properties, value_new(path), (item*)l);
      break;
    }
    case ITEM_LIST: {
      list* l=(list*)i;
      ok=ok && list_add(l,(item*)value_new(val)); // not single
      break;
    }
    case ITEM_PROPERTIES: {
      return false;
      break;
    }
  }
  if(ok) notify_observers(n);
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
  if(!notify) return;
  size_t m=strlen(notify)+1;
  char s[m]; memcpy(s, notify, m);
  char* t=s;
  int i;
  for(i=0; i< OBJECT_MAX_NOTIFIES; i++){ // use strtok
    char* e=strchr(t, ' ');
    if(!e){ o->notify[i]=value_new(t); break; }
    (*e)=0;
    o->notify[i]=value_new(t);
    t=e+1;
  }
}

void notify_observers(object* o)
{
  int i;
  for(i=0; i< OBJECT_MAX_NOTIFIES; i++){
    if(!o->notify[i]) continue;
    char* notify = value_string(o->notify[i]);
    if(is_uid(notify)){
      object* n=onex_get_from_cache(notify);
      if(n && n->evaluator) n->evaluator(n);
    }
    else{
      onp_send_object(o,notify);
    }
  }
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

  int j;
  for(j=0; j< OBJECT_MAX_NOTIFIES; j++){
    if(n->notify[j]){
      ln+=snprintf(b+ln, s-ln, ((j==0)? " Notify: %s": " %s"), value_string(n->notify[j]));
      if(ln>=s){ *b = 0; return b; }
    }
  }
  properties* p=n->properties;
  for(j=1; j<=properties_size(p); j++){
    ln+=snprintf(b+ln, s-ln, " %s: ", value_string(properties_key_n(p,j)));
    if(ln>=s){ *b = 0; return b; }
    item* i=properties_get_n(p,j);
    ln+=strlen(item_to_text(i, b+ln, s-ln));
    if(ln>=s){ *b = 0; return b; }
  }

  if(is_shell(n)){
    ln+=snprintf(b+ln, s-ln, " [shell]");
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

void onex_init()
{
  onp_init();
}

bool first_time=true;

void onex_loop()
{
  if(first_time){ first_time=false; call_all_evaluators(); }
  onp_loop();
}

void add_to_cache(object* n)
{
  if(!cache) cache=n;
  else{
    object* o=cache;
    while(o->next) o=o->next;
    o->next=n;
  }
}

object* onex_get_from_cache(char* uid)
{
  if(!uid || !(*uid)) return 0;
  object* o=cache;
  while(o){
    if(!strcmp(value_string(o->uid), uid)) return o;
    o=o->next;
  }
  return 0;
}

void onex_show_cache()
{
  log_write("+-----------cache dump------------\n");
  char buff[MAX_TEXT_LEN*2];
  object* o=cache;
  while(o){
    log_write("| %s\n", object_to_text(o,buff,MAX_TEXT_LEN*2));
    o=o->next;
  }
  log_write("+---------------------------------\n");
}

void onex_un_cache(char* uid)
{
  if(!uid || !(*uid)) return;
  object* o=cache;
  object* p=0;
  while(o){
    if(!strcmp(value_string(o->uid), uid)){
      if(!p) cache=o->next;
      else p->next=o->next;
      break;
    }
    p=o;
    o=o->next;
  }
}

void onex_run_evaluators(object* n)
{
  if(n->evaluator) n->evaluator(n);
}

void call_all_evaluators()
{
  object* o=cache;
  while(o){
    if(o->evaluator) o->evaluator(o);
    o=o->next;
  }
}

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
  object* n=new_object_from(text, 0,4);
  if(!n) return;
  object* s=onex_get_from_cache(value_string(n->uid));
  if(!s) return;
  s->properties = n->properties;
  notify_observers(s);
}

// -----------------------------------------------------------------------

