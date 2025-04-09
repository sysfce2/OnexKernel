
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

properties* persistence_objects_text=0;

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

  if(!filename || !*filename) return;

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
  persistence_objects_text=properties_new(MAX_OBJECTS);

  long n=fread(alldbtext, sizeof(char), len, db);
  alldbtext[n] = '\0';
  char* text=strtok(alldbtext, "\n"); // REVISIT not re-entrant!?
  while(text){
    if(!strncmp(text, "UID: ", 5)){
      char* u=text+5;
      char* e=strchr(u, ' ');
      if(e) *e=0;
      char uid[MAX_UID_LEN]; size_t m=snprintf(uid, MAX_UID_LEN, "%s", u);
      if(e) *e=' ';
      if(m<MAX_UID_LEN){
        mem_freestr(properties_delete(persistence_objects_text, uid));
        properties_set(persistence_objects_text, uid, mem_strdup(text));
      }
    }
    text=strtok(0, "\n");
  }
  mem_free(alldbtext);
}

void persistence_put(char* uid, char* text) {

  if(!persistence_objects_text) return;

  fprintf(db, "%s\n", text);
  fflush(db);
}





