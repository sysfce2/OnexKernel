
#include "viture_deps.h"
#include "viture.h"
#include "onl/drivers/viture/viture_imu.h"

#include <onex-kernel/log.h>
#include <onex-kernel/time.h>

static float deg_to_rads(float deg){
  return deg / 180.0f * 3.14159265358989f;
}

static float data_to_float(uint8_t* data) {

    uint8_t t[] = {
      data[3],
      data[2],
      data[1],
      data[0]
    };

    float f;
    memcpy(&f, t, 4);

    return f;
}

static head_rotations_cb_t head_rotations_cb=0;

static void imu_cb(uint8_t *data, uint16_t len, uint32_t ts) {

  if(!head_rotations_cb) return;

  float yaw   = -deg_to_rads(data_to_float(data + 8));
  float pitch =  deg_to_rads(data_to_float(data + 4));
  float roll  =  deg_to_rads(data_to_float(data));

  head_rotations_cb(ts, yaw, pitch, roll);
}

static void mcu_cb(uint16_t msgid, uint8_t *data, uint16_t len, uint32_t ts) {
}

// ------------------

void viture_init(head_rotations_cb_t cb){

  head_rotations_cb = cb;

  // Viture own the function name "init()"
  if(!init(imu_cb, mcu_cb)){
    log_write("Viture init failed\n");
    return;
  }

  int r;

  r=set_3d(true);
  if(r){
    log_write("set_3d(true) r=%d\n", r);
    return;
  }

  do{
    r=set_imu_fq(VITURE_IMU_FREQ_240HZ);
    if(r){
      log_write("set_imu_fq(60Hz) r=%d\n", r);
      time_delay_ms(200);
    }
  } while(r); // while waiting for set_3d(true) to finish

  r=set_imu(true);
  if(r){
    log_write("set_imu(true) r=%d\n", r);
    return;
  }
}

static bool unplugged=false;

void viture_end(){

  set_imu(false);
  set_3d(false);

  if(unplugged){
    deinit(); // hangs while plugged in
  }
}

// ------------------
