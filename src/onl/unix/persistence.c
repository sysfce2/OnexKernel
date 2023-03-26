
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>

#include <items.h>

#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>

#include <persistence.h>

static FILE* db=0;

properties* objects_text=0;

static properties* objects_to_save=0;

bool mkdir_p(char* filename) {

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

void persistence_init(char* filename) {

  if(!*filename) return;

  if(!mkdir_p(filename)){
    log_write("Couldn't make directory for '%s' errno=%d\n", filename, errno);
    return;
  }
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
  objects_text   =properties_new(MAX_OBJECTS);
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
}

static uint32_t lasttime=0;

#define FLUSH_RATE_MS 100

bool persistence_loop() {

  if(!objects_to_save) return false;
  uint64_t curtime = time_ms();
  if(curtime > lasttime+FLUSH_RATE_MS){
    persistence_flush();
    lasttime = curtime;
  }
  return false;
}

char* persistence_get(char* uid) {

  if(!objects_text) return 0;
  return properties_get(objects_text, uid);
}

void persistence_put(object* o) {

  if(!objects_to_save) return;

  char* uid=object_property(o, "UID");
  char* p=object_get_persist(o);
  if(p && !strcmp(p, "none")){
    mem_freestr(properties_delete(objects_text, uid));
    properties_delete(objects_to_save, uid);
    return;
  }
  properties_set(objects_to_save, uid, uid);
}

void persistence_flush() {

  if(!objects_to_save) return;

  uint16_t sz=properties_size(objects_to_save);
  if(!sz) return;
  for(int j=1; j<=sz; j++){
    char* uid=properties_get_n(objects_to_save, j);
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




