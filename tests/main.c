
// --------------------------------------------------------------------

#if defined(NRF5)

#include <boards.h>

#include <onex-kernel/mem.h>
#include <onex-kernel/boot.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/seesaw.h>
#include <onex-kernel/radio.h>

#if defined(BOARD_FEATHER_SENSE)
#include <onex-kernel/compass.h>
#include <onex-kernel/led-strip.h>
#include <onex-kernel/led-matrix.h>
#endif

#if (defined(BOARD_MAGIC3) || defined(BOARD_FEATHER_SENSE)) && defined(NRF_DO_FLASH)
#include <onex-kernel/spi-flash.h>
#endif

#if defined(DO_MOTION)
#include <onex-kernel/motion.h>
#endif

#include <onex-kernel/serial.h>
#include <onex-kernel/gfx.h>
#include <onex-kernel/touch.h>

#endif // NRF5

#include <string.h>

#include <onex-kernel/chunkbuf.h>
#include <onex-kernel/colours.h>
#include <onex-kernel/random.h>
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

#include "persistence.h"
#include <onn.h>

#include <tests.h>

static volatile int16_t run_tests= -1;
static volatile char    char_recvd= 0;

extern void run_value_tests();
extern void run_list_tests();
extern void run_properties_tests();

extern void run_database_tests();

extern void run_onn_tests(properties* config);

void run_chunkbuf_tests();

#if defined(NRF5)
static volatile bool display_state_prev= LEDS_ACTIVE_STATE;
static volatile bool display_state     =!LEDS_ACTIVE_STATE;

void button_changed(uint8_t pin, uint8_t type)
{
  bool pressed=(gpio_get(pin)==BUTTONS_ACTIVE_STATE);
  if(pressed) display_state = !display_state;
}

#if defined(BOARD_MAGIC3)
static volatile bool is_charging=false;

static void charging_changed(uint8_t pin, uint8_t type){
  is_charging=!gpio_get(CHARGE_SENSE);
}
#endif

#if defined(BOARD_PCA10059) || defined(BOARD_ADAFRUIT_DONGLE) || defined(BOARD_ITSYBITSY) || defined(BOARD_FEATHER_SENSE)
const uint8_t leds_list[LEDS_NUMBER] = LEDS_LIST;
#endif

#if defined(BOARD_PCA10059)
#define DISPLAY_STATE_LED 0  // the single green LED
#define FAILURE_LED       1  // red of RGB
#define SUCCESS_LED       2  // green of RGB
#define READY_LED         3  // blue of RGB
#elif defined(BOARD_ADAFRUIT_DONGLE) || defined(BOARD_ITSYBITSY) || defined(BOARD_FEATHER_SENSE)
#define DISPLAY_STATE_LED 0
#endif

#define ROTARY_ENC_ADDRESS 0x36
#define ROTARY_ENC_BUTTON  24

#define BATT_ADC_CHANNEL 0
#define POT1_ADC_CHANNEL 1
#define POT2_ADC_CHANNEL 2
static void set_up_gpio(void) {
#if defined(BOARD_PCA10059) || defined(BOARD_ADAFRUIT_DONGLE) || defined(BOARD_ITSYBITSY) || defined(BOARD_FEATHER_SENSE)
  gpio_mode_cb(BUTTON_1, INPUT_PULLUP, RISING_AND_FALLING, button_changed);
  for(uint8_t l=0; l< LEDS_NUMBER; l++){ gpio_mode(leds_list[l], OUTPUT); gpio_set(leds_list[l], !LEDS_ACTIVE_STATE); }
  gpio_set(leds_list[DISPLAY_STATE_LED], display_state);
#if defined(BOARD_PCA10059)
  gpio_set(leds_list[READY_LED], LEDS_ACTIVE_STATE);
#endif
#if defined(BOARD_FEATHER_SENSE)
  seesaw_gpio_input_pullup(ROTARY_ENC_ADDRESS, ROTARY_ENC_BUTTON);
  gpio_adc_init(BATTERY_V, BATT_ADC_CHANNEL);
  gpio_adc_init(GPIO_A0,   POT1_ADC_CHANNEL);
  gpio_adc_init(GPIO_A1,   POT2_ADC_CHANNEL);
#endif
#elif defined(BOARD_MAGIC3)
  gpio_mode_cb(BUTTON_1, INPUT_PULLDOWN, RISING_AND_FALLING, button_changed);
  gpio_mode(I2C_ENABLE, OUTPUT);
  gpio_set( I2C_ENABLE, 1);
  gpio_mode(LCD_BACKLIGHT, OUTPUT);
  gpio_set(LCD_BACKLIGHT, LEDS_ACTIVE_STATE);
  gpio_mode_cb(CHARGE_SENSE, INPUT, RISING_AND_FALLING, charging_changed);
  gpio_adc_init(BATTERY_V, BATT_ADC_CHANNEL);
#endif
}
#endif

#if defined(NRF5)

// this is for Feather Sense
#define ADC_TOP_V        3600
#define ADC_BITS_RANGE   1024
#if defined(BOARD_FEATHER_SENSE)
#define ADC_RESISTOR_DIV    2
#elif defined(BOARD_MAGIC3)
#define ADC_RESISTOR_DIV    7 / 4
#else
#define ADC_RESISTOR_DIV    1
#endif
#define BATT_LOWEST      3400
#define BATT_HIGHEST     3750

void sprintf_battery(char* buf, uint16_t size) {

  int16_t bv = gpio_read(BATT_ADC_CHANNEL);
  int16_t mv = bv * ADC_TOP_V / ADC_BITS_RANGE * ADC_RESISTOR_DIV;
  int16_t pc = (mv-BATT_LOWEST)*100/(BATT_HIGHEST - BATT_LOWEST);

  snprintf(buf, size, "%d/%dmv/%d%%", bv, mv, pc);
}
#endif

#if defined(BOARD_MAGIC3)
static void show_touch();
static bool new_touch_info=false;
static touch_info_t ti={ 120, 140 };

#if defined(DO_MOTION)
static void show_motion();
static bool new_motion_info=false;
static motion_info_t mi;
#endif

static int irqs=0;

void touched(touch_info_t touchinfo)
{
  ti=touchinfo;
  new_touch_info=true;
  irqs++;
}

#if defined(DO_MOTION)
void moved(motion_info_t motioninfo)
{
  mi=motioninfo;
  new_motion_info=true;
}
#endif

char buf[64];

void show_random()
{
  uint8_t r1=random_ish_byte();
  uint8_t r2=random_byte();
  snprintf(buf, 64, "#%02x#%02x#", r1, r2);
  gfx_pos(10, 135);
  gfx_text(buf);
}

void show_touch() {
  snprintf(buf, 64, "-%03d-%03d-", ti.x, ti.y);
  gfx_pos(10, 90);
  gfx_text(buf);
  snprintf(buf, 64, "-%s-%s-%d-", touch_actions[ti.action], touch_gestures[ti.gesture], irqs);
  gfx_pos(10, 110);
  gfx_text(buf);
}

#if defined(DO_MOTION)
void show_motion()
{
  snprintf(buf, 64, "(%05d)(%05d)(%05d)", mi.x, mi.y, mi.z);
  gfx_pos(10, 30);
  gfx_text(buf);
}
#endif

static void show_battery(){

  sprintf_battery(buf, 64);

  gfx_pos(10, 135);
  gfx_text(buf);

  gfx_pos(10, 155);
  gfx_text(is_charging? "charging": "battery");
}

#endif // magic 3

void serial_cb(bool connect, char* tty){
  if(connect) return;
#if defined(NRF5)
  char chars[1024];
  serial_read(chars, 1024);
  if(chars[0]=='#') char_recvd=chars[1];
#else
  char_recvd='t';
#endif
}

#if (defined(BOARD_MAGIC3) || defined(BOARD_FEATHER_SENSE)) && defined(NRF_DO_FLASH)

#define FLASH_TEST_DATA_START 0x31000
#define FLASH_TEST_DATA_SIZE  4096
#define FLASH_TEST_ERASE_LEN  SPI_FLASH_ERASE_LEN_4KB

static uint8_t wrbuf[FLASH_TEST_DATA_SIZE];
static uint8_t rdbuf[FLASH_TEST_DATA_SIZE];

char* run_flash_tests(char* allids) {

  char* err;

  err=spi_flash_init(allids);
  if(err) return err;

  err = spi_flash_erase(FLASH_TEST_DATA_START, FLASH_TEST_ERASE_LEN, 0);
  if(err) return err;

  for(uint32_t i=0; i < FLASH_TEST_DATA_SIZE; i++){
    wrbuf[i] = random_ish_byte();
  }
  uint32_t start_write_ts=(uint32_t)time_ms();
  static volatile uint32_t end_write_ts=0;
  void record_end_ts(){ end_write_ts=(uint32_t)time_ms(); }
  err = spi_flash_write(FLASH_TEST_DATA_START, wrbuf, FLASH_TEST_DATA_SIZE, record_end_ts);
  if(err) return err;
  while(!end_write_ts);

  err = spi_flash_read(FLASH_TEST_DATA_START, rdbuf, FLASH_TEST_DATA_SIZE, 0);
  if(err) return err;

  bool ok=!memcmp(wrbuf, rdbuf, FLASH_TEST_DATA_SIZE);
  static char res[64];
  snprintf(res, 64, ok? "%ldms %x:%x:%x:%x==%x:%x:%x:%x":
                        "%ldms %x:%x:%x:%x!=%x:%x:%x:%x",
                    end_write_ts-start_write_ts,
                    wrbuf[0], wrbuf[1], wrbuf[2], wrbuf[3],
                    rdbuf[0], rdbuf[1], rdbuf[2], rdbuf[3]);
  return res;
}
#endif

void run_tests_maybe(properties* config) {

  if(run_tests) return;
  run_tests++;

  log_write("-----------------OnexKernel tests------------------------\n");

#if defined(BOARD_MAGIC3) && defined(NRF_DO_FLASH)
  char allids[64];
  char* flash_result=run_flash_tests(allids);

  gfx_pos(15, 235);
  gfx_text(flash_result);

  gfx_pos(15, 255);
  gfx_text(allids);
#endif

#if defined(BOARD_PCA10059) || defined(BOARD_ADAFRUIT_DONGLE) || defined(BOARD_ITSYBITSY) || defined(BOARD_FEATHER_SENSE)
  gpio_set(leds_list[DISPLAY_STATE_LED], !LEDS_ACTIVE_STATE);
#if defined(BOARD_PCA10059)
  gpio_set(leds_list[READY_LED],         !LEDS_ACTIVE_STATE);
#endif
#endif

  run_value_tests();
  run_list_tests();
  run_properties_tests();

  run_database_tests();
  run_chunkbuf_tests();

  run_onn_tests(config);

#if defined(BOARD_FEATHER_SENSE) && defined(NRF_DO_FLASH)
  char allids[64];
  char* flash_result=run_flash_tests(allids);
  log_write("flash tests: %s %s\n", flash_result, allids);
  // flash tests: 30ms 1f:1d:b2:67==1f:1d:b2:67 (ef,40,15)(ef,14)
#endif

#if defined(NRF5)
  int failures=onex_assert_summary();
#if defined(BOARD_PCA10059)
  if(failures) gpio_set(leds_list[FAILURE_LED], LEDS_ACTIVE_STATE);
  else         gpio_set(leds_list[SUCCESS_LED], LEDS_ACTIVE_STATE);
#elif defined(BOARD_ADAFRUIT_DONGLE) || defined(BOARD_ITSYBITSY) || defined(BOARD_FEATHER_SENSE)
  if(!failures) gpio_set(leds_list[DISPLAY_STATE_LED], LEDS_ACTIVE_STATE);
#else
  gfx_pos(10, 55);
  gfx_text(failures? "FAIL": "SUCCESS");
#endif
#else
  onex_assert_summary();
#endif
}

#if defined(NRF5)

static int8_t radio_rssi;
void radio_cb(bool connect, char* channel){
  if(connect) return;
  radio_rssi=radio_last_rssi();
}

static void send_big_radio_data(bool first_send){
  char buf[1024];
  if(first_send){     // 152 * 3 = 456 - 252 = 204 = 2 pkts; 3 lines  ! 3rd line "ffff" triggers a reply

    log_write("send_big_radio_data first\n");

    snprintf(buf, 1024, "UID: uid-1111-da59-40a5-560b Devices: uid-9bd4-da59-40a5-560b is: device io: uid-b7e0-376f-59b8-212cc uid-6dd9-c392-4bd7-aa79 uid-b7e0-376f-59b8-212cc\n"
                        "UID: uid-2222-da59-40a5-560b Devices: uid-9bd4-da59-40a5-560b is: device io: uid-b7e0-376f-59b8-212cc uid-6dd9-c392-4bd7-aa79 uid-b7e0-376f-59b8-212cc\n"
                        "UID: uid-ffff-da59-40a5-560b Devices: uid-9bd4-da59-40a5-560b is: device io: uid-b7e0-376f-59b8-212cc uid-6dd9-c392-4bd7-aa79 uid-b7e0-376f-59b8-212cc\n"
    );
    radio_write("",buf,strlen(buf));

  } else {     // 269 chars = 2 pkts; 1 line

    log_write("send_big_radio_data response\n");

    snprintf(buf, 1024, "UID: uid-e7d3-f5fb-18bd-881e Devices: uid-pcr-device Notify: uid-c392-a132-1deb-29c6 uid-pcr-device is: device name: OnexApp user: uid-c392-a132-1deb-29c6 "
                        "io: uid-d90b-7d12-2ca9-3cbc uid-ac9c-8998-d9f6-f6a7 uid-fce5-31ad-2a29-eba9 peers: uid-pcr-device uid-iot-device\n"
    );
    radio_write("",buf,strlen(buf));

    snprintf(buf, 1024, "OBS: uid-4ea0-9edd-f54b-ef44 Devices: uid-pcr-device\n");
    radio_write("",buf,strlen(buf));
  }
}

static void check_big_radio_data(){
  do{
    static char buf[1024];
    uint16_t rm=radio_available();
    int16_t  rn=radio_read(buf, 1024);
    if(rn==0) return;
    log_write("radio available/read: %d %d (%s)\n", rm, rn, buf);
    if(rn<0){ log_write("** radio data corrupt\n"); return; }
    if(strstr(buf, "UID: uid-ffff")){
      send_big_radio_data(false);
    }
    log_write("-----------------(%d)--\n", radio_rssi);
  } while(true);
}

static void loop_serial(void*){ serial_loop(); }

#endif

void run_chunkbuf_tests(){

  log_write("-------- chunkbuf tests ---------\n");

  chunkbuf* wside = chunkbuf_new(130+20*4+22, true);
  chunkbuf* rside = chunkbuf_new(130+20*4+22, true);

  // this exact string has a checksum that's \n
  #define CHKSUM_IS_NL "UID: uid-ab3c-ae72-bf87-7c00 Ver: 2 Devices: uid-c124-447c-ad00-e529 Notify: uid-5cef-4db6-79f0-0197 is: editable light light: off"
  onex_assert(chunkbuf_writable(wside, strlen(CHKSUM_IS_NL), '\n'), "wside writable");
  chunkbuf_write(wside, CHKSUM_IS_NL, strlen(CHKSUM_IS_NL), '\n');

  onex_assert(chunkbuf_writable(wside, strlen("1234567890123456789"), '\n'), "wside writable");
  chunkbuf_write(wside, "1234567890123456789", strlen("1234567890123456789"), '\n');

  onex_assert(chunkbuf_writable(wside, strlen("5678901234567891234"), '\n'), "wside writable");
  chunkbuf_write(wside, "5678901234567891234", strlen("5678901234567891234"), '\n');

  onex_assert(chunkbuf_writable(wside, strlen("1234567890123456789"), '\n'), "wside writable");
  chunkbuf_write(wside, "1234567890123456789", strlen("1234567890123456789"), '\n');

  onex_assert(chunkbuf_writable(wside, strlen("5678901234567891234"), '\n'), "wside writable");
  chunkbuf_write(wside, "5678901234567891234", strlen("5678901234567891234"), '\n');

  onex_assert( chunkbuf_writable(wside, 15,   -1), "wside writable for 15 more no delim");
  onex_assert(!chunkbuf_writable(wside, 15, '\n'), "wside not writable for 15 more w. delim");

  log_write("-------- chunkbuf written -----------\n");

  for(int i=0; ; i++){
    char pkt[7];
    int n=216-i*7;
    if(n>0) onex_assert_equal_num(chunkbuf_readable(wside, -1), n, "wside readable");
    uint16_t rn = chunkbuf_read(wside, pkt, 7, -1);
  ; if(!rn) break;
    if(i==23){ log_write("ohh nooo! packet loossss!! %^#!&*~ (%.7s)\n", pkt); continue; }
    if(i==27){ log_write("ohh nooo! corrupptiooion!! %^#!&*~\n"); pkt[1]='#'; }
    onex_assert(chunkbuf_writable(rside, rn, -1), "---- rside writable");
    chunkbuf_write(rside, pkt, rn, -1);
  }
  onex_assert( chunkbuf_writable(rside, 15+7, -1), "---- rside writable 15+7 more");
  onex_assert(!chunkbuf_writable(rside, 15+8, -1), "---- rside not writable 15+8 more");

  log_write("-------- chunkbuf transferred ---\n");

  for(int l=1; ; l++){
    char line[132];
    uint16_t rd = chunkbuf_readable(rside, '\n');
  ; if(!rd) break;
    onex_assert_equal_num(rd, l==1? 132: l==3? 14: 21, "---- rside readable (wrong when pkt lost)");
    uint16_t rn = chunkbuf_read(rside, line, 132, '\n');
    log_write("rn=%d line: \"%s\" len: %d\n", rn, line, strlen(line));
    if(l==1) onex_assert_equal_num(rn, 130, "first line is 130 long");
    else     onex_assert_equal_num(rn, l==3 || l==4? 0: 19,  "next lines are 19 long unless corrupt");
  }

  log_write("-------- chunkbuf done ----------\n");

  chunkbuf_free(rside);
  chunkbuf_free(wside);
}

void run_colour_tests(){

  log_write("-------- colour tests ---------\n");

  colours_bcs bcs;
  colours_hsv hsv;
  colours_rgb rgb;

  bcs = (colours_bcs){ 255,  85,  64 }; rgb = colours_bcs_to_rgb(bcs);
  log_write("bcs=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", bcs.b, bcs.c, bcs.s, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){  85, 191, 255 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);

  hsv = (colours_hsv){   0,   0,   0 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){   0,   0, 127 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){   0,   0, 255 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);

  hsv = (colours_hsv){  85,   0,   0 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){  85,   0, 127 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){  85,   0, 255 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);

  hsv = (colours_hsv){ 171,   0,   0 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){ 171,   0, 127 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){ 171,   0, 255 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);

  hsv = (colours_hsv){   0, 127,   0 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){   0, 127, 127 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){   0, 127, 255 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);

  hsv = (colours_hsv){  85, 127,   0 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){  85, 127, 127 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){  85, 127, 255 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);

  hsv = (colours_hsv){ 171, 127,   0 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){ 171, 127, 127 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){ 171, 127, 255 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);

  hsv = (colours_hsv){   0, 255,   0 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){   0, 255, 127 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){   0, 255, 255 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);

  hsv = (colours_hsv){  85, 255,   0 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){  85, 255, 127 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){  85, 255, 255 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);

  hsv = (colours_hsv){ 171, 255,   0 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){ 171, 255, 127 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
  hsv = (colours_hsv){ 171, 255, 255 }; rgb = colours_hsv_to_rgb(hsv);
  log_write("hsv=(%3d,%3d,%3d): rgb=(%3d,%3d,%3d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);

  char* colour_chars;
  colours_rgb from_chars;

  colour_chars = "%ffff00"; from_chars = colours_parse_string(colour_chars);
  log_write("%s = rgb: { %02x %02x %02x }\n", colour_chars, from_chars.r, from_chars.g, from_chars.b);
  colour_chars = "%ff5500"; from_chars = colours_parse_string(colour_chars);
  log_write("%s = rgb: { %02x %02x %02x }\n", colour_chars, from_chars.r, from_chars.g, from_chars.b);
  colour_chars = "%ffab00"; from_chars = colours_parse_string(colour_chars);
  log_write("%s = rgb: { %02x %02x %02x }\n", colour_chars, from_chars.r, from_chars.g, from_chars.b);
  colour_chars = "%7f557f"; from_chars = colours_parse_string(colour_chars);
  log_write("%s = rgb: { %02x %02x %02x }\n", colour_chars, from_chars.r, from_chars.g, from_chars.b);

  colour_chars = "#ffffff"; from_chars = colours_parse_string(colour_chars);
  log_write("%s = rgb: { %02x %02x %02x }\n", colour_chars, from_chars.r, from_chars.g, from_chars.b);
  colour_chars = "#55ffff"; from_chars = colours_parse_string(colour_chars);
  log_write("%s = rgb: { %02x %02x %02x }\n", colour_chars, from_chars.r, from_chars.g, from_chars.b);
  colour_chars = "#abffff"; from_chars = colours_parse_string(colour_chars);
  log_write("%s = rgb: { %02x %02x %02x }\n", colour_chars, from_chars.r, from_chars.g, from_chars.b);
  colour_chars = "#557f7f"; from_chars = colours_parse_string(colour_chars);
  log_write("%s = rgb: { %02x %02x %02x }\n", colour_chars, from_chars.r, from_chars.g, from_chars.b);

#if defined(BOARD_FEATHER_SENSE)
  led_strip_fill_col( "#ff0");
  led_matrix_fill_col("#ff0");  led_strip_show(); led_matrix_show(); time_delay_ms(350);
  led_strip_fill_col( "#f0f");
  led_matrix_fill_col("#f0f");  led_strip_show(); led_matrix_show(); time_delay_ms(350);
  led_strip_fill_col( "#0ff");
  led_matrix_fill_col("#0ff");  led_strip_show(); led_matrix_show(); time_delay_ms(350);

  led_strip_fill_col( "red");
  led_matrix_fill_col("red");   led_strip_show(); led_matrix_show(); time_delay_ms(350);
  led_strip_fill_col( "green");
  led_matrix_fill_col("green"); led_strip_show(); led_matrix_show(); time_delay_ms(350);
  led_strip_fill_col( "blue");
  led_matrix_fill_col("blue");  led_strip_show(); led_matrix_show(); time_delay_ms(350);

  led_strip_fill_rgb( (colours_rgb){ 255,255,  0 });
  led_matrix_fill_rgb((colours_rgb){ 255,255,  0 }); led_strip_show(); led_matrix_show(); time_delay_ms(350);
  led_strip_fill_rgb( (colours_rgb){ 255,  0,255 });
  led_matrix_fill_rgb((colours_rgb){ 255,  0,255 }); led_strip_show(); led_matrix_show(); time_delay_ms(350);
  led_strip_fill_rgb( (colours_rgb){   0,255,255 });
  led_matrix_fill_rgb((colours_rgb){   0,255,255 }); led_strip_show(); led_matrix_show(); time_delay_ms(350);

  led_strip_fill_hsv( (colours_hsv){   0,255,127 });
  led_matrix_fill_hsv((colours_hsv){   0,255,127 }); led_strip_show(); led_matrix_show(); time_delay_ms(350);
  led_strip_fill_hsv( (colours_hsv){  85,255,127 });
  led_matrix_fill_hsv((colours_hsv){  85,255,127 }); led_strip_show(); led_matrix_show(); time_delay_ms(350);
  led_strip_fill_hsv( (colours_hsv){ 171,255,127 });
  led_matrix_fill_hsv((colours_hsv){ 171,255,127 }); led_strip_show(); led_matrix_show(); time_delay_ms(350);
#endif
}

int main() {

  properties* config = properties_new(32);
#if !defined(NRF5)
  properties_set(config, "dbpath", value_new("tests.ondb"));
  properties_set(config, "channels", list_vals_new_from_fixed("serial ipv6"));
#else
#if !defined(BOARD_MAGIC3)
  properties_set(config, "flags", list_vals_new_from_fixed("debug-on-serial log-to-led"));
#else
  properties_set(config, "flags", list_vals_new_from_fixed("log-to-gfx"));
#endif
#endif
  properties_set(config, "test-uid-prefix", value_new("tests"));

  time_init();

  log_init(config);
  log_write("-------- test of early message 1 -------\n");

#if defined(NRF5) && !defined(BOARD_MAGIC3)
  serial_init(0,0,serial_cb); // overrides one in log for commands
  serial_ready_state(); // blocks until stable at start
  time_ticker(loop_serial, 0, 1);
#endif

  log_write("-------- test of early message 2-------\n");

  random_init();
#if defined(NRF5)
  gpio_init();
  set_up_gpio();
#if !defined(BOARD_MAGIC3)

#if defined(BOARD_FEATHER_SENSE)
  uint8_t usb_status = serial_ready_state();

  led_matrix_init();
  if(usb_status == SERIAL_POWERED_NOT_READY){
    led_strip_fill_col( "#700");
    led_matrix_fill_col("#700"); led_strip_show(); led_matrix_show();
    log_flash(1,0,0);
    time_delay_ms(500);
    boot_reset(false); // REVISIT
  }
  else
  if(usb_status == SERIAL_NOT_POWERED_OR_READY){
    led_strip_fill_col( "#110");
    led_matrix_fill_col("#110"); led_strip_show(); led_matrix_show();
  }
  else
  if(usb_status == SERIAL_READY){
    led_strip_fill_col( "#010");
    led_matrix_fill_col("#010"); led_strip_show(); led_matrix_show();
  }
#endif

  radio_init(0, radio_cb);

#if defined(BOARD_FEATHER_SENSE)
  compass_init();
#endif

  log_write("----------nRF52 tests----------------\n");

  while(1){

    log_loop();

    if(char_recvd){
      log_write(">%c<----------\n", char_recvd);
      if(char_recvd=='t') run_tests++;
      if(char_recvd=='l') run_colour_tests();
      if(char_recvd=='s') send_big_radio_data(true);
#if defined(BOARD_FEATHER_SENSE)
      if(char_recvd=='i'){
        compass_info_t ci = compass_direction();
        uint8_t      temp = compass_temperature();
        uint16_t     vers = seesaw_status_version_hi(ROTARY_ENC_ADDRESS);
        int32_t      rotn = seesaw_encoder_position(ROTARY_ENC_ADDRESS);
        bool         butt = seesaw_gpio_read(ROTARY_ENC_ADDRESS, ROTARY_ENC_BUTTON);
        int16_t      pot1 = gpio_read(POT1_ADC_CHANNEL);
        int16_t      pot2 = gpio_read(POT2_ADC_CHANNEL);
        char batt[64]; sprintf_battery(batt, 64);
        log_write("compass data: %5d/%5d/%5d=%d° %d°C\n", ci.x, ci.y, ci.z, ci.o, temp);
        log_write("rotary data: vers=%d rotn=%d butt=%d pot1=%d pot2=%d\n", vers, rotn, butt, pot1, pot2);
        log_write("battery: %s\n", batt);
      }
#endif
      if(char_recvd=='c') onex_show_cache();
      if(char_recvd=='n') onex_show_notify();
      if(char_recvd=='v') value_dump_small();
      if(char_recvd=='V') value_dump();
      if(char_recvd=='f') persistence_show_db();
      if(char_recvd=='m') mem_show_allocated(true);
      if(char_recvd=='p') gpio_show_power_status();
      if(char_recvd=='r') boot_reset(false);
      if(char_recvd=='b') boot_reset(true);
      if(char_recvd=='h') log_write("t.ests co.l.our s.end-radio i.nputs | object c.ache n.otifies Vv.alues m.em p.ower r.eset b.ootloader\n");

      if(char_recvd=='i') time_delay_ms(100);
      else char_recvd=0;
    }

    run_tests_maybe(config);

    check_big_radio_data();

    if (display_state_prev != display_state){
      display_state_prev = display_state;
      gpio_set(leds_list[DISPLAY_STATE_LED], display_state);
      log_write("#%d %d %d\n", display_state, random_ish_byte(), random_byte());
    }
  }
#else
  gfx_init();
  gfx_screen_colour(GFX_YELLOW);
  gfx_screen_fill();
  gfx_rect_line(  0,  0, ST7789_WIDTH, ST7789_HEIGHT, GFX_RED, 3);
  gfx_rect_fill( 15,180,  20, 20, GFX_RGB256(255,255,255));
  gfx_rect_fill( 15,210,  20, 20, GFX_WHITE);
  gfx_rect_fill( 35,180,  20, 20, GFX_RGB256(63,63,63));
  gfx_rect_fill( 35,210,  20, 20, GFX_GREY_7);
  gfx_rect_fill( 55,180,  20, 20, GFX_RGB256(31,31,31));
  gfx_rect_fill( 55,210,  20, 20, GFX_GREY_3);
  gfx_rect_fill( 75,180,  20, 20, GFX_RGB256(15,15,15));
  gfx_rect_fill( 75,210,  20, 20, GFX_GREY_1);
  gfx_rect_fill( 95,180,  20, 20, GFX_RGB256(255,0,0));
  gfx_rect_fill( 95,210,  20, 20, GFX_RED);
  gfx_rect_fill(115,180,  20, 20, GFX_RGB256(0,255,0));
  gfx_rect_fill(115,210,  20, 20, GFX_GREEN);
  gfx_rect_fill(135,180,  20, 20, GFX_RGB256(0,0,255));
  gfx_rect_fill(135,210,  20, 20, GFX_BLUE);
  gfx_rect_fill(155,180,  20, 20, GFX_RGB256(255,255,0));
  gfx_rect_fill(155,210,  20, 20, GFX_YELLOW);
  gfx_rect_fill(175,180,  20, 20, GFX_RGB256(255,0,255));
  gfx_rect_fill(175,210,  20, 20, GFX_MAGENTA);
  gfx_rect_fill(195,180,  20, 20, GFX_RGB256(0,255,255));
  gfx_rect_fill(195,210,  20, 20, GFX_CYAN);
  gfx_text_colour(GFX_BLUE);

  radio_init(0, radio_cb);
  send_big_radio_data(true);

  touch_init(touched);
#if defined(DO_MOTION)
  motion_init(moved);
#endif

  while(1){

    log_loop();

    run_tests_maybe(config);

    check_big_radio_data();

    if(new_touch_info){
      new_touch_info=false;
      show_touch();
    //show_random();
      show_battery();

      run_tests++;
    }
#if defined(DO_MOTION)
    if(new_motion_info){
      new_motion_info=false;
      static int ticks=0; // every 20ms
      ticks++;
      if(!(ticks%20)) show_motion();
    }
#endif
    if (display_state_prev != display_state){
      display_state_prev = display_state;
      gpio_set(LCD_BACKLIGHT, display_state);
    }
    if(log_to_gfx){
      if(gfx_log_buffer){
        gfx_pos(10, 10);
        gfx_text_colour(GFX_RED);
        for(uint8_t i=1; i<=list_size(gfx_log_buffer); i++){
          char* msg = list_get_n(gfx_log_buffer, i);
          if(strlen(msg) > 40) msg[40]=0;
          gfx_text(msg);
          free(msg);
        }
        gfx_text_colour(GFX_BLUE);
        list_clear(gfx_log_buffer, false);
      }
    }

    static uint8_t  loop_count = 0;
    static uint64_t tm_last = 0;
    static uint8_t  lps = 111;

    loop_count++;
    uint64_t tm=time_ms();
    if(tm > tm_last + 1000) {
      tm_last = tm;
      lps = loop_count;
      loop_count = 0;
    }
    snprintf(buf, 64, "lps: ===%d===", lps);
    gfx_pos(10, 70);
    gfx_text(buf);
  }
#endif // MAGIC3
#else // NRF5
  serial_cb(false, "tty");
  if(char_recvd=='t') run_tests++;
  run_tests_maybe(config);
  run_colour_tests();
  time_end();
#endif

  properties_free(config, true);
}

// --------------------------------------------------------------------
