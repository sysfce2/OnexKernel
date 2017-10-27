
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "onp.h"

#include <lib/properties.h>
#include <onex-kernel/log.h>
#include <onf.h>

// ---------------------------------------------------------------------------------

static char*   get_key(char** p);
static char*   get_val(char** p);
static void    add_to_cache(object* n);
static object* find_object(char* uid, object* n);
static char*   nested_property(object* n, char* property);
static bool    is_uid(char* uid);
static bool    add_observer(object* o, char* notify);
static void    set_observers(object* o, char* notify);
static void    notify_observers(object* n);
static void    show_notifies(object* o);
static void    call_all_evaluators();

// ---------------------------------

typedef struct object {
  char*           uid;
  onex_evaluator  evaluator;
  properties*     properties;
  char*           notify[OBJECT_MAX_NOTIFIES];
  struct object*  next;
} object;

object* cache=0;

object* new_object(char* uid, char* is, onex_evaluator evaluator, uint8_t max_size)
{
  object* n=(object*)calloc(1,sizeof(object));
  n->uid=uid;
  n->properties=properties_new(max_size);
  properties_set(n->properties, "is", is);
  n->evaluator=evaluator;
  return n;
}

object* object_new(char* uid, char* is, onex_evaluator evaluator, uint8_t max_size)
{
  object* n=new_object(uid, is, evaluator, max_size);
  add_to_cache(n);
  return n;
}

object* object_new_from(char* text)
{
  object* n=0;
  char* uid=0;
  char* notify=0;
  char* is=0;
  uint8_t max_size=4;
  char* p=text;
  while(true){
    char* key=get_key(&p); if(!key) break;
    char* val=get_val(&p); if(!val) break;
    if(!strcmp(key,"UID")    && strlen(val)){ uid=val;    free(key); }
    else
    if(!strcmp(key,"Notify") && strlen(val)){ notify=val; free(key); }
    else
    if(!strcmp(key,"is")     && strlen(val)){ is=val;     free(key); }
    else {
      if(!n && !uid && !is) return 0;
      if(!n){
        n=new_object(uid, is, 0, max_size);
        set_observers(n, notify);
      }
      if(!properties_set(n->properties, key, val)) break;
    }
  }
  return n;
}

void object_new_shell(char* uid, char* notify)
{
  uint8_t max_size=4;
  object* n=(object*)calloc(1,sizeof(object));
  n->uid=uid;
  add_to_cache(n);
  set_observers(n, notify);
}

bool object_is_shell(object* o)
{
  return !object_property_size(o) && !o->evaluator;
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
  if(!c){
    char* r=strdup(*p+1);
    (*p)+=strlen(*p+1)+1;
    return r;
  }
  (*c)=0;
  char* s=strrchr(*p, ' ');
  (*c)=':';
  (*s)=0;
  char* r=strdup(*p+1);
  (*s)=' ';
  (*p)=s+1;
  return r;
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

object* object_get_from_cache(char* uid)
{
  object* o=cache;
  while(o){
    if(!strcmp(o->uid, uid)) return o;
    o=o->next;
  }
  return 0;
}

void show_cache()
{
  log_write("+-----------cache dump------------\n");
  char buff[128];
  object* o=cache;
  while(o){
    log_write("| %s\n", object_to_text(o,buff,128));
    o=o->next;
  }
  log_write("+---------------------------------\n");
}

void object_set_evaluator(object* n, onex_evaluator evaluator)
{
  n->evaluator=evaluator;
}

char* object_property(object* n, char* property)
{
  if(!strcmp(property, "UID")) return n->uid;
  char* c=strchr(property, ':');
  if(!c) return properties_get(n->properties, property);
  return nested_property(n, property);
}

char* nested_property(object* n, char* property)
{
  char* p=strdup(property);
  char* c=strchr(p, ':');
  *c=0; c++;
  char* uid=properties_get(n->properties, p);
  object* o=find_object(uid,n);
  if(!o) return 0;
  char* r=object_property(o, c);
  free(p);
  return r;
}

object* find_object(char* uid, object* n)
{
  if(!is_uid(uid)) return 0;
  object* o=object_get_from_cache(uid);
  if(o && !object_is_shell(o)){
    add_observer(o,n->uid);
    return o;
  }
  if(!o) object_new_shell(uid, n->uid);
  onp_send_observe(uid, "");
  return 0;
}

bool is_uid(char* uid)
{
  return uid && !strncmp(uid,"uid-",4);
}

uint8_t object_property_size(object* n)
{
  return properties_size(n->properties);
}

char* object_property_key(object* n, uint8_t index)
{
  return properties_get_key(n->properties, index-1);
}

char* object_property_val(object* n, uint8_t index)
{
  return properties_get_val(n->properties, index-1);
}

bool object_property_is(object* n, char* property, char* expected)
{
  char* v=object_property(n, property);
  return v && !strcmp(v,expected);
}

bool object_property_set(object* n, char* property, char* value)
{
  bool ok=properties_set(n->properties, property, value);
  notify_observers(n);
  return ok;
}

bool add_observer(object* o, char* notify)
{
  int i;
  for(i=0; i< OBJECT_MAX_NOTIFIES; i++){
    if(o->notify[i] && !strcmp(o->notify[i], notify)) return true;
  }
  for(i=0; i< OBJECT_MAX_NOTIFIES; i++){
    if(!o->notify[i]){ o->notify[i]=notify; return true; }
  }
  log_write("can't add observer %s to %s\n", notify, o->uid);
  show_notifies(o);
  return false;
}

void set_observers(object* o, char* notify)
{
  if(!notify) return;
  char* s=notify;
  int i;
  for(i=0; i< OBJECT_MAX_NOTIFIES; i++){
    char* e=strchr(s, ' ');
    if(!e){ o->notify[i]=s; break; }
    (*e)=0;
    o->notify[i]=s;
    s=e+1;
  }
}

void notify_observers(object* o)
{
  int i;
  for(i=0; i< OBJECT_MAX_NOTIFIES; i++){
    if(!o->notify[i]) continue;
    char* notify = o->notify[i];
    if(is_uid(notify)){
      object* n=object_get_from_cache(notify);
      if(n && n->evaluator) n->evaluator(n);
    }
    else{
      onp_send_object(o,notify);
    }
  }
}

void show_notifies(object* o)
{
  log_write("notifies of %s\n", o->uid);
  int i;
  for(i=0; i< OBJECT_MAX_NOTIFIES; i++){
    if(o->notify[i]){ log_write("%s ", o->notify[i]); }
  }
  log_write("\n--------------\n");
}

char* object_to_text(object* n, char* b, uint8_t s)
{
  if(!n){ *b = 0; return b; }

  int ln=0;

  ln+=snprintf(b+ln, s-ln, "UID: %s", n->uid);
  if(ln>=s){ *b = 0; return b; }

  int i;
  for(i=0; i< OBJECT_MAX_NOTIFIES; i++){
    if(n->notify[i]){
      ln+=snprintf(b+ln, s-ln, ((i==0)? " Notify: %s": " %s"), n->notify[i]);
      if(ln>=s){ *b = 0; return b; }
    }
  }
  for(i=1; i<=object_property_size(n); i++){
    ln+=snprintf(b+ln, s-ln, " %s: %s", object_property_key(n,i), object_property_val(n,i));
    if(ln>=s){ *b = 0; return b; }
  }
  return b;
}

void object_log(object* o)
{
  char buff[128];
  log_write("{ %s }\n", object_to_text(o,buff,128));
}

// -----------------------------------------------------------------------

void onex_init()
{
  onp_init();
}

void onex_run_evaluators(object* n)
{
  if(n->evaluator) n->evaluator(n);
}

bool first_time=true;

void onex_loop()
{
  if(first_time){ first_time=false; call_all_evaluators(); }
  onp_loop();
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
  char* nl= strchr(uid, '\n');
  if(nl) *nl=0;
  object* o=object_get_from_cache(uid);
  if(!o) return;
  add_observer(o,from);
  onp_send_object(o,from);
}

void recv_object(char* text)
{
  object* n=object_new_from(text);
  if(!n) return;
  object* s=object_get_from_cache(n->uid);
  if(!s) return;
  s->properties = n->properties;
  notify_observers(s);
}

// -----------------------------------------------------------------------

void* onex_malloc()
{
  return 0;
}

void* onex_strdup()
{
  return 0;
}

void* onex_free()
{
  return 0;
}

void* onex_memory_usage()
{
  return 0;
}

// -----------------------------------------------------------------------

