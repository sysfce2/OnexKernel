
#if defined(BOARD_MAGIC3) || defined(BOARD_FEATHER_SENSE)
#define FLASH_NOT_FAKE
#endif

#include <inttypes.h>
#define FMT_UINT32 PRIu32

#include <items.h>
#include <onex-kernel/lib.h>
#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>
#if defined(FLASH_NOT_FAKE)
#include <onex-kernel/spi-flash.h>
#include <onex-kernel/database.h>
#endif

#include <persistence.h>

#if defined(FLASH_NOT_FAKE)

static database_storage* db = 0;

#define FLASH_DB_SECTOR_SIZE  4096
#define FLASH_DB_SECTOR_COUNT  512 // REVISIT: we only do 2Mb flash!
#define FLASH_SIZE (FLASH_DB_SECTOR_SIZE * FLASH_DB_SECTOR_COUNT)

static void flash_db_init(database_storage* db){
}

static void flash_db_erase(database_storage* db, uint32_t address, uint16_t size, void (*cb)()){
  char* err = spi_flash_erase(address, SPI_FLASH_ERASE_LEN_4KB, 0);
  if(err){ log_write("flash_db_erase error: %s", err); return; }
}

static void flash_db_write(database_storage* db, uint32_t address, uint8_t* buf, uint16_t size, void (*cb)()){
  char* err = spi_flash_write(address, buf, size, 0);
  if(err){ log_write("flash_db_write error: %s", err); return; }
}

static void flash_db_read(database_storage* db, uint32_t address, uint8_t* buf, uint16_t size, void (*cb)()){
  char* err = spi_flash_read(address, buf, size, 0);
  if(decent_string((char*)buf)) log_write("flash_db_read: size=%d len=%d [%s]\n", size, strlen((char*)buf), buf);
  if(err){ log_write("flash_db_read error: %s", err); return; }
}

static void flash_db_format(database_storage* db){
  log_write("flash_db_format\n");
  for(uint16_t s=0; s < db->sector_count; s++){
    flash_db_erase(db, db->sector_size * s, db->sector_size, 0);
    database_sector_info dsi;
    dsi.erase_count = (s==0? 2: 1);
    dsi.zero_term = 0;
    flash_db_write(db, db->sector_size * s, (uint8_t*)&dsi, sizeof(database_sector_info), 0);
  }
  log_write("flash_db_format done\n");
}

static database_storage* flash_db_storage_new(){

  database_storage* db=mem_alloc(sizeof(database_storage));
  if(!db) return 0;

  (*db).sector_size  = FLASH_DB_SECTOR_SIZE;
  (*db).sector_count = FLASH_DB_SECTOR_COUNT;

  (*db).init   = flash_db_init;
  (*db).format = flash_db_format;
  (*db).erase  = flash_db_erase;
  (*db).write  = flash_db_write;
  (*db).read   = flash_db_read;

  return db;
}
#else

static properties* persistence_objects_text=0;

#endif

static list* keep_actives = 0;

list* persistence_init(properties* config){

#if defined(FLASH_NOT_FAKE)

  char allids[64];
  char* err = spi_flash_init(allids);
  if(err){ log_write("persistence_init error: %s", err); return 0; }
  else     log_write("flash: %s\n", allids);

  db = flash_db_storage_new();

  keep_actives = database_init(db,config);
  return keep_actives;
#else
  persistence_objects_text=properties_new(MAX_OBJECTS);
  keep_actives = list_new(64);
  return 0;
#endif
}

// for testing
list* persistence_reload(){
#if defined(FLASH_NOT_FAKE)
  database_free(db);
  keep_actives = database_init(db,0);
#endif
  return keep_actives;
}

void persistence_show_db(){
#if defined(FLASH_NOT_FAKE)
  if(db) database_dump(db);
  else log_write("persistence_show_db but no db\n");
#else
  log_write("persistence_show_db...\n");
#endif
}

char* persistence_get(char* uid){
#if defined(FLASH_NOT_FAKE)
  static char obj_text[2048];
  uint16_t s = database_get(db, uid, 0, (uint8_t*)obj_text, 2048);
  return obj_text; // REVISIT: hmmmmm
#else
  return properties_get(persistence_objects_text, uid);
#endif
}

void persistence_put(char* uid, uint32_t ver, char* text) {

#if defined(FLASH_NOT_FAKE)
  if(!text || !(*text)) return;
  bool ok=database_put(db, uid, ver, (uint8_t*)text, strlen(text)+1);
#else
  mem_freestr(properties_del(persistence_objects_text, uid));
  properties_set(persistence_objects_text, uid, mem_strdup(text));
  bool ka = strstr(text, "Cache: keep-active");
  if(ka) list_vals_set_add(keep_actives, uid);
#endif
}


