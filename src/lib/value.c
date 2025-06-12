
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>

#if defined(NRF5)
#include <app_util_platform.h>
#else
#include <pthread.h>
#endif

#include <items.h>

#include <onex-kernel/mem.h>
#include <onex-kernel/lib.h>
#include <onex-kernel/log.h>

char* unknown_to_text(char* b){ *b='?'; *(b+1)=0; return b; }

typedef struct value {
  item_type type;
  char*  val;
  uint16_t refs;
} value;

static properties* all_values=0;

#if defined(NRF5)
#define MAX_VALUES 1024
#define MAX_TEXT_LEN 64
#else
#define MAX_VALUES 4096
#define MAX_TEXT_LEN 4096
#endif

#if defined(NRF5)

#define ENTER_LOCKING                                  \
        uint8_t __CR_NESTED = 0;                       \
        app_util_critical_region_enter(&__CR_NESTED)

#define RETURN_UNLOCKING(x)                            \
        app_util_critical_region_exit(__CR_NESTED);    \
        return x

#else

static pthread_mutex_t value_lock;
#define ENTER_LOCKING       pthread_mutex_lock(&value_lock)
#define RETURN_UNLOCKING(x) pthread_mutex_unlock(&value_lock); return x

#endif

value* value_new(char* val) {
  if(!val) return 0;
  if(!all_values) all_values=properties_new(MAX_VALUES);
  ENTER_LOCKING;
  value* ours=(value*)properties_get(all_values, val);
  if(ours){
    ours->refs++;
    // prob not being freed if getting big: 65536 refs
    if(ours->refs==0){  log_write("V00!%s\n", ours->val); ours->refs++; }
    RETURN_UNLOCKING(ours);
  }
  ours=(value*)mem_alloc(sizeof(value));
  if(!ours){
    log_write("VALS!!\n"); // this is serious
    RETURN_UNLOCKING(0);
  }
  if(strchr(val, ' ') || strchr(val, '\n')){
    log_write("VALS ! '%s'\n", val); // this is serious
 // RETURN_UNLOCKING(0); // it happens, so...
  }
  ours->type=ITEM_VALUE;
  ours->val=mem_strdup(val);
  ours->refs=1; // don't count our own 2 uses in all_values

  if(!ours->val){
    log_write("!VALS!\n"); // this is serious
    mem_free(ours);
    RETURN_UNLOCKING(0);
  }
  if(!properties_set(all_values, ours->val, ours)){
    log_write("!!VALS\n"); // this is serious
    value_dump_small();
    mem_freestr(ours->val);
    mem_free(ours);
    RETURN_UNLOCKING(0);
  }
  RETURN_UNLOCKING(ours);
}

value* value_new_fmt(char* fmt, ...){
  char valbuf[MAX_TEXT_LEN];
  va_list args;
  va_start(args, fmt);
  vsnprintf(valbuf, MAX_TEXT_LEN, fmt, args);
  va_end(args);
  return value_new(valbuf);
}

value* value_ref(value* v) {
  if(!v) return 0;
  ENTER_LOCKING;
  v->refs++;
  RETURN_UNLOCKING(v);
}

void value_free(value* v)
{
  if(!v) return;
  ENTER_LOCKING;
  v->refs--;
  if(v->refs){
    RETURN_UNLOCKING();
  }
  properties_delete(all_values, v->val);
  mem_freestr(v->val);
  mem_free(v);
  RETURN_UNLOCKING();
}

char* value_string(value* v)
{
  if(!v) return 0;
  return v->val;
}

bool value_equal(value* v1, value* v2)
{
  if(!v1) return !v2;
  if(!v2) return false;
  if(v1==v2) return true;
  return v1->val == v2->val;
}

bool value_num_greater(value* v1, value* v2){
  int32_t i1 = strto_int32(value_string(v1));
  int32_t i2 = strto_int32(value_string(v2));
  return i1 > i2;
}

bool value_is(value* v, char* s)
{
  if(!v) return !s;
  return !strcmp(v->val, s);
}

char* value_to_text(value* v, char* b, uint16_t s)
{
  if(!v){ *b = 0; return b; }
  int ln=snprintf(b,s,"%s",v->val);
  if(ln>=s){ *b = 0; return b; }
  if(b[ln-1]==':'){
    ln++;
    if(ln>=s){ *b = 0; return b; }
    b[ln-2]='\\';
    b[ln-1]=':';
    b[ln]=0;
  }
  return b;
}

void value_log(value* v)
{
  char buf[MAX_TEXT_LEN];
  log_write("%s\n", value_to_text(v,buf,MAX_TEXT_LEN));
}

void value_dump() {
  uint16_t s=properties_size(all_values);
  for(uint16_t i=1; i<=s; i++){
    char* key=properties_key_n(all_values, i);
    value* val=(value*)properties_get_n(all_values, i);
    log_write("[%d|%s|%d]\n", i, key, val->refs);
    log_flush();
  }
  log_write("#vals=%d\n", s);
}

void value_dump_small() {
  uint16_t num_nums=0;
  char b[128];
  uint8_t ln=0; b[0]=0;
  uint16_t num_vals=properties_size(all_values);
  for(uint16_t i=1; i<=num_vals; i++){
    char* key=properties_key_n(all_values, i);
    if(strto_int32(key)) num_nums++;
    ln+=snprintf(b+ln,128-ln,"%d=%.8s ", i, key);
    if(ln>=128){
      log_write("%s\n", b);
      ln=0; b[0]=0;
    }
  }
  log_write("%s\n#vals=%d #nums=%d\n", b, num_vals, num_nums);
}



