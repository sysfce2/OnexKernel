
#include <mem_manager.h>
#include <string.h>
#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>

#define LOG_MEM 0

static bool initialized=false;

void* Mem_alloc(char* func, int line, size_t n)
{
  if(!initialized){ initialized=true; nrf_mem_init(); }
  void* p=nrf_calloc(1,n);
  if(!p){
    p=calloc(1,n);
    if(LOG_MEM) log_write("****** mem_alloc using calloc %p", p);
  }
  if(LOG_MEM) log_write("mem_alloc   %p %lu %s:%d\n", p, n, func, line);
  return p;
}

void  Mem_free(char* func, int line, void* p)
{
  if(!initialized){ initialized=true; nrf_mem_init(); }
  if(LOG_MEM) log_write("mem_free    %p %lu %s:%d\n", p, (size_t)0, func, line);
  if(nrf_free(p)){
    if(LOG_MEM) log_write("****** mem_free using free %p", p);
    free(p);
  }
}

char* Mem_strdup(char* func, int line, const char* s)
{
  if(!initialized){ initialized=true; nrf_mem_init(); }
  size_t n=strlen(s)+1;
  char* p=nrf_malloc(n);
  if(!p){
    p=malloc(n);
    if(LOG_MEM) log_write("****** mem_strdup using malloc %p", p);
  }
  if(p) memcpy(p,s,n);
  char b[20]; mem_strncpy(b, s, 18);
  if(LOG_MEM) log_write("mem_strdup  %p %lu [%s] %s:%d\n", p, n, b, func, line);
  return p;
}

void Mem_freestr(char* func, int line, char* p)
{
  if(!p) return;
  if(!initialized){ initialized=true; nrf_mem_init(); }
  size_t n=strlen(p)+1;
  char b[20]; mem_strncpy(b, p, 18);
  if(LOG_MEM) log_write("mem_freestr %p %lu [%s] %s:%d\n", p, n, b, func, line);
  if(nrf_free(p)){
    if(LOG_MEM) log_write("****** mem_freestr using free %p", p);
    free(p);
  }
}

void mem_strncpy(char* dst, const char* src, size_t count)
{
  if(count < 1) return;
  strncpy(dst, src, count);
  dst[count-1] = 0;
}

