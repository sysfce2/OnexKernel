
#include <mem_manager.h>
#include <string.h>

#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>

#ifndef LOG_MEM
#define LOG_MEM 0
#endif

static bool initialized=false;

bool mem_fillin_up=false;

static void* bot_alloc=0;
static void* top_alloc=0;

#define xGRIND_THE_MEM
#ifdef  GRIND_THE_MEM
#define MEMGRIND_ENTRIES 2048
static void*    memgrind_pntr[MEMGRIND_ENTRIES];
static char*    memgrind_file[MEMGRIND_ENTRIES];
static uint16_t memgrind_line[MEMGRIND_ENTRIES];
static uint16_t memgrind_size[MEMGRIND_ENTRIES];
static uint16_t memgrind_top=0;
static uint32_t memgrind_tot=0;
#endif

static void memgrind_alloc(void* pntr, char* file, uint16_t line, uint16_t size){
#ifdef GRIND_THE_MEM
  if(memgrind_top==MEMGRIND_ENTRIES) return;
  uint16_t i=0;
  while(i<memgrind_top && memgrind_pntr[i]) i++;
  if(i==memgrind_top){
    memgrind_top++;
    if(memgrind_top==MEMGRIND_ENTRIES){
      // log_write("***** memgrind full!\n");
      return;
    }
  }
  memgrind_pntr[i]=pntr;
  memgrind_file[i]=file;
  memgrind_line[i]=line;
  memgrind_size[i]=size;

  memgrind_tot += memgrind_size[i];
#endif
}

static void memgrind_free(void* pntr){
#ifdef GRIND_THE_MEM
  for(uint16_t i=0; i<memgrind_top; i++){
    if(memgrind_pntr[i]==pntr){
      memgrind_tot -= memgrind_size[i];
      memgrind_pntr[i]=0;
      return;
    }
  }
  if(memgrind_top!=MEMGRIND_ENTRIES){
    // log_write("***** freeing but not allocated? %p %s\n", pntr, pntr);
  }
#endif
}

void mem_show_allocated(bool clear){
#ifdef GRIND_THE_MEM
  for(uint16_t i=0; i<memgrind_top; i++){
    if(memgrind_pntr[i]){
      bool suppress = (!strcmp(memgrind_file[i], "new_object"    )) ||
                      (!strcmp(memgrind_file[i], "new_shell"     )) ||
                      (!strcmp(memgrind_file[i], "onex_un_cache" )) ||
                      (!strcmp(memgrind_file[i], "properties_new")) ||
                      (!strcmp(memgrind_file[i], "properties_set")) ||
                      (!strcmp(memgrind_file[i], "list_new"      )) ||
                      (!strcmp(memgrind_file[i], "value_new"     )) ||
                      (!strcmp(memgrind_file[i], "chunkbuf_new"  )) ||
                      (!strcmp(memgrind_file[i], "log_write_mode"));
      if(suppress) continue;
      log_write("%p@%s:%ld=%d ", memgrind_pntr[i],
                                 memgrind_file[i],
                                 memgrind_line[i],
                                 memgrind_size[i]);
    }
    if(clear) memgrind_pntr[i]=0; // REVISIT: may warn about freeing unalloc'd
  }
  log_write("\nmemgrind_top=%d tot=%ld\n", memgrind_top, memgrind_tot);
#endif
  log_write("mem used %ld\n", mem_used());
}

// --------------------

void* Mem_alloc(char* func, int line, size_t n) {
  if(!n) return 0;
  if(!initialized){ initialized=true; nrf_mem_init(); }
  void* p=nrf_calloc(1,n);
  if(!bot_alloc) bot_alloc=p;
  if(!p){
    p=calloc(1,n);
    if(LOG_MEM) log_write("****** mem_alloc using calloc %p\n", p);
    if(p > top_alloc){
      top_alloc=p;
      if(top_alloc - bot_alloc > 70000) mem_fillin_up = true;
      if(log_to_gfx){
        if(LOG_MEM) log_write("clc %lu %s:%d %p\n", n, func, line, p);
      }else{
        if(LOG_MEM) log_write("=============== new calloc top: %p %lu %s:%d\n", p, n, func, line);
      }
    }
  }
  if(LOG_MEM) log_write("mem_alloc   %p %lu %s:%d\n", p, n, func, line);
  memgrind_alloc(p, func, line, n);
  return p;
}

void Mem_free(char* func, int line, void* p) {
  if(!p) return;
  if(!initialized){ initialized=true; nrf_mem_init(); }
  if(LOG_MEM) log_write("mem_free    %p %lu %s:%d\n", p, (size_t)0, func, line);
  if(nrf_free(p)){
    if(LOG_MEM) log_write("****** mem_free using free %p\n", p);
    free(p);
  }
  memgrind_free(p);
}

char* Mem_strdup(char* func, int line, const char* s) {
  if(!s) return 0;
  if(!initialized){ initialized=true; nrf_mem_init(); }
  size_t n=strlen(s)+1;
  void* p=nrf_malloc(n);
  if(!bot_alloc) bot_alloc=p;
  if(!p){
    p=malloc(n);
    if(LOG_MEM) log_write("****** mem_strdup using malloc %p\n", p);
    if(p > top_alloc){
      top_alloc=p;
      if(top_alloc - bot_alloc > 70000) mem_fillin_up = true;
      if(log_to_gfx){
        if(LOG_MEM) log_write("mlc %lu %s:%d %p\n", n, func, line, p);
      }else{
        if(LOG_MEM) log_write("=============== new malloc top: %p %lu %s:%d\n", p, n, func, line);
      }
    }
  }
  if(p) memcpy(p,s,n);
  if(LOG_MEM){
    char b[20]; mem_strncpy(b, s, 20);
    log_write("mem_strdup  %p %lu [%s] %s:%d\n", p, n, b, func, line);
  }
  memgrind_alloc(p, func, line, n);
  return (char*)p;
}

void Mem_freestr(char* func, int line, char* p) {
  if(!p) return;
  if(!initialized){ initialized=true; nrf_mem_init(); }
  size_t n=strlen(p)+1;
  if(LOG_MEM){
    char b[20]; mem_strncpy(b, p, 20);
    log_write("mem_freestr %p %lu [%s] %s:%d\n", p, n, b, func, line);
  }
  if(nrf_free(p)){
    if(LOG_MEM) log_write("****** mem_freestr using free %p\n", p);
    free(p);
  }
  memgrind_free(p);
}

void mem_strncpy(char* dst, const char* src, size_t count) {
  if(count < 1) return;
  strncpy(dst, src, count);
  dst[count-1] = 0;
}

uint32_t mem_used(){
  return top_alloc - bot_alloc;
}

