
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <items.h>

#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>
#include <onex-kernel/database.h>

#include <persistence.h>

#define MMAP_DB_SECTOR_SIZE  4096
#define MMAP_DB_SECTOR_COUNT 1024
#define MMAP_SIZE (MMAP_DB_SECTOR_SIZE * MMAP_DB_SECTOR_COUNT)

uint8_t* mmap_db;

static void mmap_db_init(database_storage* db){
}

static void mmap_db_format(database_storage* db){

  for(uint32_t i=0; i < MMAP_SIZE; i++) mmap_db[i]=0xff;

  for(uint16_t s=0; s < MMAP_DB_SECTOR_COUNT; s++){
    database_sector_info* dsi = (database_sector_info*)(mmap_db + MMAP_DB_SECTOR_SIZE * s);
    (*dsi).erase_count = (s==0? 2: 1);
    (*dsi).zero_term = 0;
  }
}

static void mmap_db_erase(database_storage* db, uint32_t address, uint16_t size, void (*cb)()){
  for(uint16_t i=0; i<size; i++) mmap_db[address+i]=0xff;
}

static void mmap_db_write(database_storage* db, uint32_t address, uint8_t* buf, uint16_t size, void (*cb)()){
  for(uint16_t i=0; i<size; i++) mmap_db[address+i]=buf[i];
}

static void mmap_db_read(database_storage* db, uint32_t address, uint8_t* buf, uint16_t size, void (*cb)()){
  for(uint16_t i=0; i<size; i++) buf[i]=mmap_db[address+i];
}

static database_storage* mmap_db_storage_new(){

  database_storage* db=mem_alloc(sizeof(database_storage));
  if(!db) return 0;

  (*db).sector_size  = MMAP_DB_SECTOR_SIZE;
  (*db).sector_count = MMAP_DB_SECTOR_COUNT;

  (*db).init   = mmap_db_init;
  (*db).format = mmap_db_format;
  (*db).erase  = mmap_db_erase;
  (*db).write  = mmap_db_write;
  (*db).read   = mmap_db_read;

  return db;
}

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

static database_storage* db;

list* persistence_init(char* filename) {

  if(!filename || !*filename) return 0;

  log_write("Using DB file %s\n", filename);

  if(!mkdir_p(filename)){
    log_write("Couldn't make directory for '%s' errno=%d\n", filename, errno);
    return 0;
  }
  int fd = open(filename, O_RDWR | O_CREAT, 0644);
  if(fd== -1){
    log_write("Couldn't open %s for DB: %s\n", filename, strerror(errno));
    return 0;
  }
  ftruncate(fd, MMAP_SIZE);
  mmap_db = mmap(0, MMAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

  db = mmap_db_storage_new();

  list* keep_actives = database_init(db);
  return keep_actives;
}

// for testing
list* persistence_reload(){
  database_free(db);
  list* keep_actives = database_init(db);
  return keep_actives;
}

void persistence_show_db(){
  database_dump(db);
}

char* persistence_get(char* uid){
  static char obj_text[2048];
  uint16_t s = database_get(db, uid, 0, (uint8_t*)obj_text, 2048);
  return obj_text; // REVISIT: hmmmmm
}

void persistence_put(char* uid, uint32_t ver, char* text){
  if(!text || !(*text)) return;
  bool ok=database_put(db, uid, ver, (uint8_t*)text, strlen(text)+1);
}

/*
  database_show(db);
  mem_free(db);
*/












