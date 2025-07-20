
#include <string.h>
#include <stdlib.h>

#include <onex-kernel/mem.h>
#include <onex-kernel/database.h>
#include <onex-kernel/log.h>

#include <tests.h>

// ----------- database tests --------------------

#define TEST_SECTOR_SIZE  64
#define TEST_SECTOR_COUNT  4

static uint8_t test_db_storage[TEST_SECTOR_SIZE*TEST_SECTOR_COUNT];

static void test_stub_init(database_storage* db){
}

static void test_stub_format(database_storage* db){
  log_write("test_stub_format\n");
  for(uint32_t i=0; i<TEST_SECTOR_SIZE*TEST_SECTOR_COUNT; i++){
    test_db_storage[i]=0xff;
  }
  database_sector_info* dsi0 = (database_sector_info*)(test_db_storage+TEST_SECTOR_SIZE*0);
  (*dsi0).erase_count=2; (*dsi0).zero_term=0;
  database_sector_info* dsi1 = (database_sector_info*)(test_db_storage+TEST_SECTOR_SIZE*1);
  (*dsi1).erase_count=1; (*dsi1).zero_term=0;;
  database_sector_info* dsi2 = (database_sector_info*)(test_db_storage+TEST_SECTOR_SIZE*2);
  (*dsi2).erase_count=1; (*dsi2).zero_term=0;;
  database_sector_info* dsi3 = (database_sector_info*)(test_db_storage+TEST_SECTOR_SIZE*3);
  (*dsi3).erase_count=1; (*dsi3).zero_term=0;;
}

static void test_stub_erase(database_storage* db, uint32_t address, uint16_t size, void (*cb)()){
  for(uint16_t i=0; i<size; i++) test_db_storage[address+i]=0xff;
}

static void test_stub_write(database_storage* db, uint32_t address, uint8_t* buf, uint16_t size, void (*cb)()){
  for(uint16_t i=0; i<size; i++) test_db_storage[address+i]=buf[i];
}

static void test_stub_read(database_storage* db, uint32_t address, uint8_t* buf, uint16_t size, void (*cb)()){
  for(uint16_t i=0; i<size; i++) buf[i]=test_db_storage[address+i];
}

static database_storage* test_stub_storage_new(){

  database_storage* db=mem_alloc(sizeof(database_storage));
  if(!db) return 0;

  (*db).sector_size  = TEST_SECTOR_SIZE;
  (*db).sector_count = TEST_SECTOR_COUNT;

  (*db).init   = test_stub_init;
  (*db).format = test_stub_format;
  (*db).erase  = test_stub_erase;
  (*db).write  = test_stub_write;
  (*db).read   = test_stub_read;

  return db;
}

/*
      2/ww        2/ww        2/ww/head   1/tail
    | 0         | 1         | 2         | 3         |
    |+uid-abc/1+|+uid-def/1+|+uid-def/2+|           | +6 new no erase
    |+uid-123/1+|+uid-456/1+|+uid-abc/2+|           |

      2/tail      2           2           2/ww/head
    | 0         | 1         | 2         | 3         |
    | uid-abc/1 | uid-def/1 | uid-def/2 |+uid-abc/3+| +uid-abc/3
    | uid-123/1 | uid-456/1 | uid-abc/2 |*uid-123/1*|

      3/eww/head  2/tail      2           2
    | 0         | 1         | 2         | 3         |
    |+uid-def/3+| uid-def/1 | uid-def/2 | uid-abc/3 | +uid-def/3
    |*uid-456/1*| uid-456/1 | uid-abc/2 | uid-123/1 |

      3/w         3/eww/head  2/tail      2
    | 0         | 1         | 2         | 3         |
    | uid-def/3 |+uid-def/4+| uid-def/2 | uid-abc/3 | +uid-def/4
    | uid-456/1 |+uid-abc/4+| uid-abc/2 | uid-123/1 | +uid-abc/4

      3           3           3/ew/head   2/tail
    | 0         | 1         | 2         | 3         |
    | uid-def/3 | uid-def/4 |+uid-abc/5+| uid-abc/3 | +uid-abc/5
    | uid-456/1 | uid-abc/4 |*uid-123/1*| uid-123/1 |

      3/tail      3           3           3/ew/head
    | 0         | 1         | 2         | 3         |
    | uid-def/3 | uid-def/4 | uid-abc/5 |+uid-def/5+| +uid-def/5
    | uid-456/1 | uid-abc/4 | uid-123/1 |*uid-456/1*|
*/

void run_database_tests(){

  log_write("----------- database tests --------------------\n");

  char* test_object_123_1 = "UID: uid-123 Ver: 1 is: x"; // length 25 or 26 with 0
  char* test_object_456_1 = "UID: uid-456 Ver: 1 is: x"; // these don't update

  char* test_object_abc_1 = "UID: uid-abc Ver: 1 is: x";
  char* test_object_def_1 = "UID: uid-def Ver: 1 is: x";

  char* test_object_abc_2 = "UID: uid-abc Ver: 2 is: x";
  char* test_object_def_2 = "UID: uid-def Ver: 2 is: x";

  char* test_object_abc_3 = "UID: uid-abc Ver: 3 is: x";
  char* test_object_def_3 = "UID: uid-def Ver: 3 is: x";

  char* test_object_abc_4 = "UID: uid-abc Ver: 4 is: x";
  char* test_object_def_4 = "UID: uid-def Ver: 4 is: x";

  char* test_object_abc_5 = "UID: uid-abc Ver: 5 is: x";
  char* test_object_def_5 = "UID: uid-def Ver: 5 is: x";

  char* test_object_aaa_1 = "UID: uid-aaa Ver: 1 is: x";
  char* test_object_bbb_1 = "UID: uid-bbb Ver: 1 is: x";
  char* test_object_ccc_1 = "UID: uid-ccc Ver: 1 is: x";

  uint8_t test_object_len = strlen(test_object_123_1) + 1;

  // ----- Initialisation
  database_storage* db = test_stub_storage_new();
  list_free(database_init(db), false);

  char* base = (char*)test_db_storage + sizeof(database_sector_info);
  uint16_t w_point;

  // ----- Sector 0 Object 1
  database_put(db, "uid-123", 1, (uint8_t*)test_object_123_1, test_object_len);
  w_point = 0;
  onex_assert_equal(base + w_point, test_object_123_1, "uid-123/1 written to correct place");

  // ----- Sector 0 Object 2
  database_put(db, "uid-abc", 1, (uint8_t*)test_object_abc_1, test_object_len);
  w_point = test_object_len;
  onex_assert_equal(base + w_point, test_object_abc_1, "uid-abc/1 written to correct place");

  // ----- Sector 1 Object 1
  database_put(db, "uid-456", 1, (uint8_t*)test_object_456_1, test_object_len);
  w_point = TEST_SECTOR_SIZE;
  onex_assert_equal(base + w_point, test_object_456_1, "uid-456/1 written to correct place");

  // ----- Sector 1 Object 2
  database_put(db, "uid-def", 1, (uint8_t*)test_object_def_1, test_object_len);
  w_point = TEST_SECTOR_SIZE + test_object_len;
  onex_assert_equal(base + w_point, test_object_def_1, "uid-def/1 written to correct place");

  // ----- Sector 2 Object 1
  database_put(db, "uid-abc", 2, (uint8_t*)test_object_abc_2, test_object_len);
  w_point = TEST_SECTOR_SIZE * 2;
  onex_assert_equal(base + w_point, test_object_abc_2, "uid-abc/2 written to correct place");

  // ----- Sector 2 Object 2
  database_put(db, "uid-def", 2, (uint8_t*)test_object_def_2, test_object_len);
  w_point = TEST_SECTOR_SIZE * 2 + test_object_len;
  onex_assert_equal(base + w_point, test_object_def_2, "uid-def/2 written to correct place");

  // ----- Sector 3 Object 1
  database_put(db, "uid-abc", 3, (uint8_t*)test_object_abc_3, test_object_len);
  w_point = TEST_SECTOR_SIZE * 3;
  onex_assert_equal(base + w_point, test_object_123_1, "uid-123 safe from erase");
  w_point = TEST_SECTOR_SIZE * 3 + test_object_len;
  onex_assert_equal(base + w_point, test_object_abc_3, "uid-abc/3 written to correct place");

  // ----- Sector 0 Object 1
  database_put(db, "uid-def", 3, (uint8_t*)test_object_def_3, test_object_len);
  w_point = 0;
  onex_assert_equal(base + w_point, test_object_456_1, "uid-456 safe from erase");
  w_point = test_object_len;
  onex_assert_equal(base + w_point, test_object_def_3, "uid-def/3 written to correct place");

  // ----- Sector 1 Object 1
  database_put(db, "uid-abc", 4, (uint8_t*)test_object_abc_4, test_object_len);
  w_point = TEST_SECTOR_SIZE;
  onex_assert_equal(base + w_point, test_object_abc_4, "uid-abc/4 written to correct place");

  // ----- Re-initialisation
  database_show(db);
  database_free(db);
  list_free(database_init(db), false);
  database_show(db);

  // ----- Sector 1 Object 2
  database_put(db, "uid-def", 4, (uint8_t*)test_object_def_4, test_object_len);
  w_point = TEST_SECTOR_SIZE + test_object_len;
  onex_assert_equal(base + w_point, test_object_def_4, "uid-def/4 written to correct place");

  // ----- Sector 2 Object 1
  database_put(db, "uid-abc", 5, (uint8_t*)test_object_abc_5, test_object_len);
  w_point = TEST_SECTOR_SIZE * 2;
  onex_assert_equal(base + w_point, test_object_123_1, "uid-123 safe from erase");
  w_point = TEST_SECTOR_SIZE * 2 + test_object_len;
  onex_assert_equal(base + w_point, test_object_abc_5, "uid-abc/5 written to correct place");

  // ----- Sector 3 Object 1
  database_put(db, "uid-def", 5, (uint8_t*)test_object_def_5, test_object_len);
  w_point = TEST_SECTOR_SIZE * 3;
  onex_assert_equal(base + w_point, test_object_456_1, "uid-456 safe from erase");
  w_point = TEST_SECTOR_SIZE * 3 + test_object_len;
  onex_assert_equal(base + w_point, test_object_def_5, "uid-def/5 written to correct place");

  // -------------------------------------------------------

  char b[128];
  uint16_t s;

  s=database_get(db, "uid-123", 0, (uint8_t*)b, 128);
  onex_assert_equal(b, "UID: uid-123 Ver: 1 is: x", "can get uid-123");
  onex_assert_equal_num(s, 26, "length correct");

  s=database_get(db, "uid-456", 0, (uint8_t*)b, 128);
  onex_assert_equal(b, "UID: uid-456 Ver: 1 is: x", "can get uid-456");
  onex_assert_equal_num(s, 26, "length correct");

  s=database_get(db, "uid-abc", 0, (uint8_t*)b, 128);
  onex_assert_equal(b, "UID: uid-abc Ver: 5 is: x", "can get uid-abc");
  onex_assert_equal_num(s, 26, "length correct");

  s=database_get(db, "uid-def", 0, (uint8_t*)b, 128);
  onex_assert_equal(b, "UID: uid-def Ver: 5 is: x", "can get uid-def");
  onex_assert_equal_num(s, 26, "length correct");

  // ------------------------------------------

  database_show(db);

  bool ok;
  ok=database_put(db, "uid-aaa", 1, (uint8_t*)test_object_aaa_1, test_object_len);
  onex_assert(ok, "can add 1 more");
  ok=database_put(db, "uid-bbb", 1, (uint8_t*)test_object_bbb_1, test_object_len);
  onex_assert(ok, "can add 2 more");
  ok=database_put(db, "uid-ccc", 1, (uint8_t*)test_object_ccc_1, test_object_len);
  onex_assert(!ok, "can't add 3 more");

  // ------------------------------------------

  database_show(db);
  database_free(db);
  mem_free(db);
}

// ----------- end database tests --------------------


























