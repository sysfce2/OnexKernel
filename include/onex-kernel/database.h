#ifndef DATABASE_H
#define DATABASE_H

#include <stdint.h>
#include <stdbool.h>

#include <items.h>

/** round-robin database like append-only but circular. */

/* pointers to functions for flash (nRF52), mmap (Unix) or test stub (both) */

typedef struct obj_info {

  uint32_t ver;
  uint16_t sector;
  uint16_t sector_offset;
  uint16_t size;
  uint16_t pad;

} obj_info;

typedef struct database_storage database_storage;

typedef struct database_storage {

  // below properties set by database code

  uint16_t head_sector;
  uint16_t head_sector_offset;

  properties* uid_to_obj_info;

  // below properties set by backing store manager

  uint16_t sector_size;  // 4096 for mmap and spi_flash; 64 for test
  uint16_t sector_count; //  512 for 16Mbit spi_flash; 5 for test

  void (*init)  (database_storage* db);
  void (*format)(database_storage* db);
  void (*erase) (database_storage* db, uint32_t address,               uint16_t size, void (*cb)());
  void (*write) (database_storage* db, uint32_t address, uint8_t* buf, uint16_t size, void (*cb)());
  void (*read)  (database_storage* db, uint32_t address, uint8_t* buf, uint16_t size, void (*cb)());

} database_storage;

typedef struct database_sector_info {

  uint32_t erase_count;
  uint32_t zero_term; // separator from following string

} database_sector_info;

// ----------------------

list*    database_init(database_storage* db);
bool     database_put( database_storage* db, char* uid, uint32_t ver, uint8_t* buf, uint16_t size);
uint16_t database_get( database_storage* db, char* uid, uint32_t ver, uint8_t* buf, uint16_t size);
void     database_show(database_storage* db);
void     database_dump(database_storage* db);
void     database_free(database_storage* db);

// ----------------------

#endif
