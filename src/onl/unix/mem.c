
#include <string.h>
#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>

#define LOG_MEM 0

void* Mem_alloc(char* func, int line, size_t n) {
  if(!n) return 0;
  void* p=calloc(1,n);
  if(LOG_MEM) log_write("mem_alloc   %p %lu %s:%d\n", p, n, func, line);
  return p;
}

void Mem_free(char* func, int line, void* p) {
  if(!p) return;
  if(LOG_MEM) log_write("mem_free    %p %lu %s:%d\n", p, (size_t)0, func, line);
  free(p);
}

char* Mem_strdup(char* func, int line, const char* s) {
  if(!s) return 0;
  size_t n=strlen(s)+1;
  char* p=malloc(n);
  if(p) memcpy(p,s,n);
  if(LOG_MEM){
    char b[20]; mem_strncpy(b, s, 18);
    log_write("mem_strdup  %p %lu [%s] %s:%d\n", p, n, b, func, line);
  }
  return p;
}

void Mem_freestr(char* func, int line, char* p) {
  if(!p) return;
  if(LOG_MEM){
    size_t n=strlen(p)+1;
    char b[20]; mem_strncpy(b, p, 18);
    log_write("mem_freestr %p %lu [%s] %s:%d\n", p, n, b, func, line);
  }
  free(p);
}

void mem_strncpy(char* dst, const char* src, size_t count) {
  if(count < 1) return;
  strncpy(dst, src, count);
  dst[count-1] = 0;
}

