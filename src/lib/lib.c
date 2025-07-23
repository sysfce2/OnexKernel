
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include <onex-kernel/mem.h>
#include <onex-kernel/lib.h>

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

char* remove_char_in_place(char* s, char remove){
  char* pr=s;
  char* pw=s;
  while(*pr){ if(*pr==remove) pr++; *pw++ = *pr++; }
  *pw=0;
  return s;
}

char* prefix_char_in_place(char* s, char prefix, char target){
  char ss[64]; mem_strncpy(ss, s, 64);
  char* pr=ss;
  char* pw=s;
  while(*pr){ if(*pr==target) *pw++ = prefix; *pw++ = *pr++; }
  *pw=0;
  return s;
}

uint16_t num_tokens(char* s) {
  if(!s || !(*s)) return 0;
  uint16_t n=0;
  bool in_token = false;
  while(*s){
    if(isspace(*s)) in_token = false;
    else
    if(!in_token) {
      in_token = true;
      n++;
    }
    s++;
  }
  return n;
}

int32_t strto_int32(char* val){
  if(!val || !*val) return 0;
  errno=0; char* e;
  int32_t r=strtol(val, &e, 10);
  if(errno == ERANGE) return 0;
  return r;
}

bool string_is_blank(char* s){
  if(!s) return true;
  while(*s) if(!isspace(*s++)) return false;
  return true;
}

bool decent_string(char* u){
  for(uint16_t i=0; i<512; i++){
;   if(u[i]==0) return true;
;   if(u[i] < ' ' || u[i] > '~') return false;
  }
  return false;
}



