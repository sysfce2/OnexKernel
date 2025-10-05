
#include <string.h>

#include <inttypes.h>
#define FMT_UINT32 PRIu32

#include <onex-kernel/log.h>
#include <onex-kernel/lib.h>
#include <onex-kernel/boot.h>
#include <onex-kernel/mem.h>
#include <onex-kernel/show_bytes_n_chars.h>
#include <onex-kernel/database.h>

#include <onn.h>

/*
 - each sector has erase count
 - head sector is one before the one with first lowest erase count
 - head is first sector with space
 - ---
  | e=N+1 | e=N+1 | e=N | e=N |
             ^       ^- tail - oldest sector
             +- head - writing to here currently
 - ---
 - write to head until no space
 - if no space, erase tail as new head, copy latest from new tail, write new:
 - ---
 - head space = (sector-size - head_sector_offset)
 - while(no space), head moves on to tail
    - erase tail (if not already empty)
    - increment erase count: this is new head
    - copy over latest-version objects from new tail to new head
 - write new object in head
 - ---
 - if all of a tail sector is latest-versions, copy over the whole lot, etc, until space found
 - if complete loop done without finding space, you're full up, and every object has just one latest version
 - you may go on to write a newer version of an object that was saved of course

Still to do:

 - power-fail/crash recovery:
    - CRCs (for that and for wear-out)
    - detecting head/tail anomalies
 - tenancy/GC/etc refinements
*/

bool get_next_uid_ver_cache(char* p, char* uid, char* ver, char* cch){

; if(strncmp(p, "UID: ", 5)) return false;
  p += 5;

  char* sp = strchr(p, ' ');
; if(!sp) return false;

  mem_strncpy(uid, p, sp - p + 1);
  p = sp+1;

; if(strncmp(p, "Ver: ", 5)) return false;
  p += 5;

  sp = strchr(p, ' ');
; if(!sp) return false;

  mem_strncpy(ver, p, sp - p + 1);

; if(!cch) return true;
  *cch=0;

  p = sp+1;

; if(strncmp(p, "Cache: ", 7)) return true;
  p += 7;

  sp = strchr(p, ' ');
; if(!sp) return true;

  mem_strncpy(cch, p, sp - p + 1);

  return true;
}

static list* get_uid_to_obj_info(database_storage* db){

  list* keep_actives = list_new(64);

  uint16_t empty_sectors=0;

  for(uint16_t s=0; s < db->sector_count; s++){

    uint32_t data_start = db->sector_size * s + sizeof(database_sector_info);
    uint16_t size       = db->sector_size - sizeof(database_sector_info);
    uint8_t b[size]; db->read(db, data_start, b, size, 0);

    char uid[MAX_UID_LEN];
    char ver[MAX_VER_LEN];
    char cch[MAX_CCH_LEN];

    uint16_t o=0;
    while(o < size && *(b+o)!=0xff){

      char* obj = (char*)(b+o);

    ; if(!get_next_uid_ver_cache(obj, uid, ver, cch)) break;

      obj_info* oi = properties_get(db->uid_to_obj_info, uid);

      if(!oi){
        oi = mem_alloc(sizeof(obj_info));
        properties_set(db->uid_to_obj_info, uid, oi);
        (*oi).ver = 0;
      }
      uint32_t vers = strto_int32(ver);
      uint16_t obj_size_aligned = ALIGN_RH_TO_32_BIT(strlen(obj) + 1);

      if((*oi).ver <= vers){

        if((*oi).ver==vers) log_write("two objects same version! %s %s\n", uid, ver);
        // if two same version, take later; chances are reasonable that it's newer
        // REVISIT: need to update version and save on metadata updates too!

        (*oi).ver           = vers;
        (*oi).sector        = s;
        (*oi).sector_offset = o + sizeof(database_sector_info);
        (*oi).size          = obj_size_aligned;

        if(*cch && !strcmp(cch, "keep-active")) list_vals_set_add(keep_actives, uid);
        else                                    list_vals_del(    keep_actives, uid);
      }
      o += obj_size_aligned;
    }
    if(o==0){
      log_write("no objects found in sector %d\n", s);
      empty_sectors++;
      if(empty_sectors == 2){
        log_write("two sectors empty, stopping\n");
        // one empty sector, it may have crashed after erase
        // two empty means we're into unfilled space
  ;     break;
      }
    }
  }
  log_write("number of objects in DB: %d\n", properties_size(db->uid_to_obj_info));
  return keep_actives;
}

static void find_head_sector_and_offset(database_storage* db){

  database_sector_info dbsi_0;
  db->read(db, 0, (uint8_t*)(&dbsi_0), sizeof(database_sector_info), 0);

  uint16_t s;
  for(s=1; s < db->sector_count; s++){
    database_sector_info dbsi_s;
    db->read(db, s * db->sector_size, (uint8_t*)(&dbsi_s), sizeof(database_sector_info), 0);
  ; if(dbsi_s.erase_count < dbsi_0.erase_count) break;
  }
  db->head_sector = s-1;

  uint32_t start = db->head_sector * db->sector_size + sizeof(database_sector_info);
  uint16_t size  = db->sector_size - sizeof(database_sector_info);

  uint8_t b[size]; db->read(db, start, b, size, 0);

  db->head_sector_offset = db->sector_size;
  for(uint16_t o=0; o<size; o++){
    if(b[o] == 0xff){
      db->head_sector_offset = o + sizeof(database_sector_info);
;     return;
    }
  }
  log_write("head sector is        %d\n", db->head_sector);
  log_write("head sector offset is %d\n", db->head_sector_offset);
}

#define MIN_OBJ_SIZE_ROUGHLY 32

static void new_uid_to_obj_info(database_storage* db){
  uint32_t max_objects = (db->sector_size * db->sector_count) / MIN_OBJ_SIZE_ROUGHLY;
  if(max_objects > MAX_DB_OBJECTS) max_objects = MAX_DB_OBJECTS;
  log_write("database max objects=%d\n", max_objects);
  db->uid_to_obj_info = properties_new(max_objects);
}

static void free_uid_to_obj_info(database_storage* db){
  properties* p = db->uid_to_obj_info;
  if(!p) return;
  uint16_t n = properties_size(p);
  for(uint16_t i=n; i>=1; i--){
    obj_info* oi = properties_del_n(p, i);
    mem_free(oi);
  }
  properties_free(p, false);
  db->uid_to_obj_info=0;
}

list* database_init(database_storage* db, properties* config){

  bool db_format=list_vals_has(properties_get(config, "flags"), "db-format");

  (*db).init(db);

  if(db_format){
    database_wipe(db);
;   return 0;
  }
  new_uid_to_obj_info(db);

  list* keep_actives = get_uid_to_obj_info(db);

  find_head_sector_and_offset(db);

  return keep_actives;
}

void database_wipe(database_storage* db){

  log_write("formatting DB!\n");

  free_uid_to_obj_info(db);

  db->format(db);

  new_uid_to_obj_info(db);

  find_head_sector_and_offset(db);
}

void database_free(database_storage* db){
  free_uid_to_obj_info(db);
}

void erase_tail_for_new_head(database_storage* db){

  uint16_t tail_sector = db->head_sector + 1;
  if(tail_sector==db->sector_count) tail_sector=0;

  uint32_t e_point = db->sector_size * tail_sector;

  database_sector_info dbsi_t;
  db->read(db, e_point, (uint8_t*)(&dbsi_t), sizeof(database_sector_info), 0);
  db->erase(db, e_point, db->sector_size, 0);
  dbsi_t.erase_count++;
  db->write(db, e_point, (uint8_t*)(&dbsi_t), sizeof(database_sector_info), 0);

  db->head_sector = tail_sector;
  db->head_sector_offset = sizeof(database_sector_info);
}

void write_to_db_and_record_info(database_storage* db, char* uid, uint32_t ver, uint8_t* buf, uint16_t size){

  uint32_t w_point = db->sector_size * db->head_sector + db->head_sector_offset;

  uint16_t                    size_aligned = ALIGN_RH_TO_32_BIT(size);
  ALIGN_LH_TO_32_BIT(uint8_t) buf_aligned[size_aligned];     // REVISIT: ensure size on stack is OK
  mem_strncpy((char*)buf_aligned, (char*)buf, size_aligned); // pads with zeroes up to size_aligned

  db->write(db, w_point, buf_aligned, size_aligned, 0);

  obj_info* oi = properties_get(db->uid_to_obj_info, uid);

  if(!oi){
    oi = mem_alloc(sizeof(obj_info));
    properties_set(db->uid_to_obj_info, uid, oi);
  }
  (*oi).ver           = ver;
  (*oi).sector        = db->head_sector;
  (*oi).sector_offset = db->head_sector_offset;
  (*oi).size          = size_aligned;

  db->head_sector_offset += size_aligned;
}

void save_latest_versions(database_storage* db){

  uint16_t tail_sector = db->head_sector + 1;
  if(tail_sector==db->sector_count) tail_sector=0;

  uint32_t r_point = db->sector_size * tail_sector + sizeof(database_sector_info);
  uint16_t size    = db->sector_size - sizeof(database_sector_info);

  uint8_t tail[size]; db->read(db, r_point, tail, size, 0);

  char* obj = (char*)tail;
  char  uid[MAX_UID_LEN];
  char  ver[MAX_VER_LEN];

  while(1){

  ; if(!get_next_uid_ver_cache(obj, uid, ver, 0)) break;

    obj_info* oi = properties_get(db->uid_to_obj_info, uid);

    char oi_ver[16]; snprintf(oi_ver, 16, "%"FMT_UINT32, (*oi).ver);

    if((*oi).sector==tail_sector && !strcmp(oi_ver, ver)){

      uint16_t size_aligned = ALIGN_RH_TO_32_BIT(strlen(obj)+1);
      if(size_aligned==(*oi).size){
        write_to_db_and_record_info(db, uid, (*oi).ver, (uint8_t*)obj, (*oi).size);
      }
      else{
        log_write("sizes differ! len in db=%d info size=%d\n", size_aligned, (*oi).size);
      }
    }
    obj += (*oi).size;
  }
}

// size includes term 0
bool database_put(database_storage* db, char* uid, uint32_t ver, uint8_t* buf, uint16_t size){

  uint16_t s;
  for(s=0; s < db->sector_count; s++){

    uint16_t space = db->sector_size - db->head_sector_offset;

  ; if(space >= size) break;

    erase_tail_for_new_head(db);
    save_latest_versions(db);
  }
  if(s==db->sector_count){
    log_write("********** DB full!\n");
    return false;
  }
  write_to_db_and_record_info(db, uid, ver, buf, size);

  return true;
}

// ver == 0 for latest (shells with Ver: 0 should not be persisted)
uint16_t database_get(database_storage* db, char* uid, uint32_t ver, uint8_t* buf, uint16_t size){

  obj_info* oi = properties_get(db->uid_to_obj_info, uid);

  if(!oi || (ver && ver != (*oi).ver) || (size < (*oi).size)){
    *buf=0;
;   return 0;
  }
  uint32_t r_point = db->sector_size * (*oi).sector + (*oi).sector_offset;
  db->read(db, r_point, buf, (*oi).size, 0);

  return (*oi).size;
}

void database_show(database_storage* db){

  log_write("+------------ DB ---------------------------------------------------------------\n");

  for(uint16_t s=0; s < db->sector_count; s++){

    uint32_t sector_start = db->sector_size * s;

    database_sector_info dbsi_s;
    db->read(db, sector_start, (uint8_t*)(&dbsi_s), sizeof(database_sector_info), 0);
    log_write("| Sector %d, Erase count %d\n", s, dbsi_s.erase_count);

    uint32_t data_start = sector_start + sizeof(database_sector_info);
    uint16_t size       = db->sector_size - sizeof(database_sector_info);
    uint8_t b[size]; db->read(db, data_start, b, size, 0);

    uint16_t o=0;
    while(o < size && *(b+o)!=0xff){
      char* obj = (char*)(b+o);
      uint16_t size_aligned = ALIGN_RH_TO_32_BIT(strlen(obj)+1);
      log_write("|   [%s][%d/%d]\n", obj, strlen(obj)+1, size_aligned);
      o+=size_aligned;
    }
    log_write("+-------------------------------------------------------------------------------\n");
  }
  log_write("+------------ objects ----------------------------------------------------------\n");
  properties* p = db->uid_to_obj_info;
  for(uint16_t i=1; i<=properties_size(p); i++){
    char*     uid = properties_key_n(p,i);
    obj_info* oi  = properties_get_n(p,i);
    log_write("| %s ver %d sec %d off %d size %d\n", uid, (*oi).ver,(*oi).sector,(*oi).sector_offset,(*oi).size); 
  }
  log_write("+-------------------------------------------------------------------------------\n");

}

void database_dump(database_storage* db){
  uint8_t empty_sectors=0;
  log_write("+-------- database: %d sectors --------------------------------------------\n", db->sector_count);
  uint8_t buf[db->sector_size];
  for(uint16_t s=0; s < db->sector_count; s++){
    log_write("------ sector %d -------\n", s);
    db->read(db, db->sector_size * s, buf, db->sector_size, 0);
#if defined(NRF5)
    #define NUM_CHUNKS 16
    uint16_t chunk_size=db->sector_size/NUM_CHUNKS;
    for(uint16_t c=0; c<NUM_CHUNKS; c++){
      uint16_t offset = c * chunk_size;
      show_bytes_and_chars(db->sector_size * s + offset, buf + offset, chunk_size);
      boot_feed_watchdog(); // REVISIT: why is serial print so slowww?
    }
#else
    show_bytes_and_chars(db->sector_size * s, buf, db->sector_size);
#endif
    if(*(buf+sizeof(database_sector_info))==0xff) empty_sectors++;
  ; if(empty_sectors==2) break;
  }
  log_write("+------------------------------------------------------------------------------\n");
}














