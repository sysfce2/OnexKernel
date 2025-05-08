#include <stdio.h>

#include "nrf_log.h"

#include "boards.h"

#include "app_error.h"
#include "app_util.h"
#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"

#include <onex-kernel/serial.h>
#include <onex-kernel/log.h>
#include <onex-kernel/chunkbuf.h>

#if !defined(BOARD_MAGIC3)

#define SERIAL_READ_BUFFER_SIZE  2048
#define SERIAL_WRITE_BUFFER_SIZE 4096

static volatile bool initialised=false;

static volatile channel_recv_cb recv_cb = 0;
static volatile bool            pending_connect=false;

#ifndef USBD_POWER_DETECTION
#define USBD_POWER_DETECTION true // REVISIT
#endif

static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event);

#define CDC_ACM_COMM_INTERFACE  0
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE  1
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT1

APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm,
                            cdc_acm_user_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250
);

static volatile chunkbuf* serial_read_buf = 0;
static volatile chunkbuf* serial_write_buf = 0;

static volatile bool write_loop_in_progress=false;

static bool do_usb_write_block(bool first_write){

  if(first_write && write_loop_in_progress) return true;
  write_loop_in_progress=true;

  static char block[NRFX_USBD_EPSIZE]; // it's 64!
  uint16_t s = chunkbuf_read(serial_write_buf, block, NRFX_USBD_EPSIZE, -1);

  if(!s){
    write_loop_in_progress = false;
    return true;
  }

  ret_code_t e=app_usbd_cdc_acm_write(&m_app_cdc_acm, block, s);

  if(e!=NRF_SUCCESS){
    log_flash(1,0,0);
    write_loop_in_progress = false;
    return false;
  }
  return true;
}

static void buffer_readable(char* buf, uint16_t size){
  if(!chunkbuf_write(serial_read_buf, buf, size)){
    log_flash(1,0,0);
    return;
  }
  if(recv_cb) recv_cb(false, "serial");
}

#define INPUT_BUF_SIZE 1024
static unsigned char m_cdc_data_array[INPUT_BUF_SIZE];

static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event)
{
    app_usbd_cdc_acm_t const * p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);

    switch (event)
    {
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
        {
            ret_code_t ret = app_usbd_cdc_acm_read(&m_app_cdc_acm,
                                                   m_cdc_data_array,
                                                   1);
            UNUSED_VARIABLE(ret);
            chunkbuf_clear(serial_write_buf);
            if(recv_cb) recv_cb(true, "serial");
            else        pending_connect = true;
            NRF_LOG_INFO("CDC ACM port opened");
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
        {
            chunkbuf_clear(serial_write_buf);
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
        {
            do_usb_write_block(false);
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
        {
            ret_code_t ret;
            static uint16_t index = 0;
            index++;

            do
            {
                if ((m_cdc_data_array[index - 1] == '\n') ||
                    (m_cdc_data_array[index - 1] == '\r') ||
                    (index >= (INPUT_BUF_SIZE)))
                {
                    if (index > 1)
                    {
                        uint16_t size = index;
                        buffer_readable((char*)m_cdc_data_array, size);
                    }
                    index = 0;
                }

                size_t size = app_usbd_cdc_acm_rx_size(p_cdc_acm);
                NRF_LOG_DEBUG("RX: size: %lu char: %c", size, m_cdc_data_array[index - 1]);

                ret = app_usbd_cdc_acm_read(&m_app_cdc_acm,
                                            &m_cdc_data_array[index],
                                            1);
                if (ret == NRF_SUCCESS)
                {
                    index++;
                }
            }
            while (ret == NRF_SUCCESS);

            break;
        }
        default:
            break;
    }
}

static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    switch (event)
    {
        case APP_USBD_EVT_DRV_SUSPEND:
            break;
        case APP_USBD_EVT_DRV_RESUME:
            break;
        case APP_USBD_EVT_STARTED:
            break;
        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            break;
        case APP_USBD_EVT_POWER_DETECTED:
            NRF_LOG_INFO("USB power detected");

            if (!nrf_drv_usbd_is_enabled())
            {
                app_usbd_enable();
            }
            break;
        case APP_USBD_EVT_POWER_REMOVED:
            NRF_LOG_INFO("USB power removed");
            app_usbd_stop();
            break;
        case APP_USBD_EVT_POWER_READY:
            NRF_LOG_INFO("USB ready");
            app_usbd_start();
            break;
        default:
            break;
    }
}

bool serial_init(list* ttys, uint32_t baudrate, channel_recv_cb cb) {

    recv_cb = cb;

    if(initialised) return true;

    serial_read_buf  = chunkbuf_new(SERIAL_READ_BUFFER_SIZE);
    serial_write_buf = chunkbuf_new(SERIAL_WRITE_BUFFER_SIZE);

    app_usbd_serial_num_generate();

    ret_code_t ret;
    static const app_usbd_config_t usbd_config = {
        .ev_state_proc = usbd_user_ev_handler
    };

    ret = app_usbd_init(&usbd_config);
    if(ret != NRF_SUCCESS){ log_flash(1,0,0); return false; }

    NRF_LOG_INFO("USBD CDC ACM example started.");

    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    if(ret != NRF_SUCCESS){ log_flash(1,0,0); return false; }

    if (USBD_POWER_DETECTION) {
        ret = app_usbd_power_events_enable();
        if(ret != NRF_SUCCESS){ log_flash(1,0,0); return false; }
    } else {
        NRF_LOG_INFO("No USB power detection enabled\r\nStarting USB now");

        app_usbd_enable();
        app_usbd_start();
    }

    initialised=true;

    return true;
}

static bool init_ready=false;
uint8_t serial_ready_state(){
  if(!init_ready && time_ms() < 750) return SERIAL_STARTING; // REVISIT!
  init_ready=true;
  bool powered = NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk;
  bool ready   = NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_OUTPUTRDY_Msk;
  if(!powered && !ready) return SERIAL_NOT_POWERED_OR_READY;
  if( powered && !ready) return SERIAL_POWERED_NOT_READY;
  if( powered &&  ready) return SERIAL_READY;
  return 99; // not powered but still somehow ready!
}

bool serial_loop() {
  if(pending_connect && recv_cb){
    recv_cb(true, "serial");
    pending_connect = false;
  }
  return app_usbd_event_queue_process();
}

static char nl_delim = '\n';

uint16_t serial_read(char* buf, uint16_t size) {
  if(!initialised || !chunkbuf_current_size(serial_read_buf)) return 0;
  return chunkbuf_read(serial_read_buf, buf, size, nl_delim);
}

static uint16_t serial_write_delim(char* tty, char* buf, uint16_t size, bool delim);

uint16_t serial_write(char* tty, char* buf, uint16_t size) {
  return serial_write_delim(tty, buf, size, true);
}

int16_t serial_printf(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int16_t size=serial_vprintf(fmt,args);
  va_end(args);
  return size;
}

#define PRINT_BUF_SIZE 1024
static char print_buf[PRINT_BUF_SIZE];

int16_t serial_vprintf(const char* fmt, va_list args) {
  int16_t r=vsnprintf(print_buf, PRINT_BUF_SIZE, fmt, args);
  if(r>=PRINT_BUF_SIZE){
    log_flash(1,0,0);
    return 0;
  }
  return serial_write_delim("all", print_buf, r, false);
}

static uint16_t serial_write_delim(char* tty, char* buf, uint16_t size, bool delim) {
  if(!chunkbuf_write(serial_write_buf, buf, size)){
    log_flash(1,0,0);
    return 0;
  }
  if(delim){
    if(!chunkbuf_write(serial_write_buf, &nl_delim, 1)){
      log_flash(1,0,0);
      return 0;
    }
  }
  if(!do_usb_write_block(true)){
    // do_usb_write_block already flashed
    return 0;
  }
  return size;
}

#else // BOARD_MAGIC3

bool     serial_init(list* ttys, uint32_t baudrate, channel_recv_cb cb){ return false; }
uint8_t  serial_ready_state(){ return SERIAL_READY; }
uint16_t serial_read(char* buf, uint16_t size){ return 0; }
uint16_t serial_write(char* tty, char* buf, uint16_t size){ return 0; }
int16_t  serial_printf(const char* fmt, ...){ return 0; }
int16_t  serial_vprintf(const char* fmt, va_list args){ return 0; }
bool     serial_loop(){ return false; };

#endif
