
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "onp.h"

#include <items.h>
#include <onex-kernel/log.h>
#include <onf.h>

// ---------------------------------------------------------------------------------

static char*       get_key(char** p);
static char*       get_val(char** p);
static void        add_to_cache(object* n);
static object*     find_object(char* uid, object* n);
static item*       object_property_item(object* n, char* path);
static item*       nested_property_item(object* n, char* path);
static properties* nested_properties(object* n, char* path);
static bool        add_observer(object* o, char* notify);
static void        set_observers(object* o, char* notify);
static void        notify_observers(object* n);
static void        show_notifies(object* o);
static void        call_all_evaluators();

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
  properties_set(n->properties, "is", (item*)value_new(is));
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
      if(!properties_set(n->properties, key, (item*)value_new(val))) break;
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
  return !o->properties && !o->evaluator;
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

char* object_property(object* n, char* path)
{
  item* i=object_property_item(n,path);
  if(!i || i->type!=ITEM_VALUE) return 0;
  return value_string((value*)i);
}

item* object_property_item(object* n, char* path)
{
  if(!strcmp(path, "UID")) return (item*)value_new(n->uid); // leak!
  if(!strcmp(path, ""))    return (item*)n->properties;
  if(!strcmp(path, ":"))   return (item*)n->properties;
  char* c=strchr(path, ':');
  if(!c) return properties_get(n->properties, path);
  return nested_property_item(n, path);
}

item* nested_property_item(object* n, char* path)
{
  char* p=strdup(path);
  char* c=strchr(p, ':');
  *c=0; c++;
  char* uid=object_property(n, p);
  object* o=find_object(uid,n);
  item* r= o? object_property_item(o, c): 0;
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

uint8_t object_property_size(object* n, char* path)
{
  item* i=object_property_item(n,path);
  if(!i) return 0;
  if(i->type==ITEM_VALUE)      return 1;
  if(i->type==ITEM_LIST)       return list_size((list*)i);
  if(i->type==ITEM_PROPERTIES) return properties_size((properties*)i);
  return 0;
}

char* object_property_key(object* n, char* path, uint8_t index)
{
  item* t=object_property_item(n,path);
  if(t->type!=ITEM_PROPERTIES) return 0;
  properties* p=(properties*)t;
  return properties_key_n(p, index);
}

char* object_property_value(object* n, char* path, uint8_t index)
{
  item* i=0;
  item* t=object_property_item(n,path);
  if(t->type==ITEM_PROPERTIES){
    i=properties_get_n((properties*)t, index);
  }else
  if(t->type==ITEM_LIST){
    i=list_get_n((list*)t,index);
  }else
  if(t->type==ITEM_VALUE){
    i=(index==1)? t: 0;
  }
  if(!i || i->type!=ITEM_VALUE) return 0;
  return value_string((value*)i);
}

bool object_property_is_value(object* n, char* path)
{
  item* i=object_property_item(n,path);
  return i && i->type == ITEM_VALUE;
}

bool object_property_is_list(object* n, char* path)
{
  item* i=object_property_item(n,path);
  return i && i->type == ITEM_LIST;
}

bool object_property_is_properties(object* n, char* path)
{
  item* i=object_property_item(n,path);
  return i && i->type == ITEM_PROPERTIES;
}

bool object_property_is(object* n, char* path, char* expected)
{
  char* v=object_property(n, path);
  return v && !strcmp(v,expected);
}

bool object_property_set(object* n, char* path, char* value)
{
  if(strchr(path, ':')) return 0; /* no sub-properties yet */
  bool ok=properties_set(n->properties, path, (item*)value_new(value));
  notify_observers(n);
  return ok;
}

bool object_property_add(object* n, char* path, char* value)
{
  if(strchr(path, ':')) return false; /* no sub-properties yet */
  item* i=properties_get(n->properties, path);
  bool ok=true;
  if(!i){
    ok=properties_set(n->properties, path, (item*)value_new(value));
  }
  else
  switch(i->type){
    case ITEM_VALUE: {
      list* l=list_new(5);
      ok=ok && list_add(l,i);
      ok=ok && list_add(l,(item*)value_new(value));
      ok=ok && properties_set(n->properties, path, (item*)l);
      break;
    }
    case ITEM_LIST: {
      list* l=(list*)i;
      ok=ok && list_add(l,(item*)value_new(value));
      break;
    }
    case ITEM_PROPERTIES: {
      return false;
      break;
    }
  }
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

  int j;
  for(j=0; j< OBJECT_MAX_NOTIFIES; j++){
    if(n->notify[j]){
      ln+=snprintf(b+ln, s-ln, ((j==0)? " Notify: %s": " %s"), n->notify[j]);
      if(ln>=s){ *b = 0; return b; }
    }
  }
  properties* p=n->properties;
  for(j=1; j<=properties_size(p); j++){
    ln+=snprintf(b+ln, s-ln, " %s: ", properties_key_n(p,j));
    item* i=properties_get_n(p,j);
    ln+=strlen(item_to_text(i, b+ln, s-ln));
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
  char* u=uid; while(*u > ' ') u++; *u=0;
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

