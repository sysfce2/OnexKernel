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

#if !defined(LOG_TO_SERIAL)
#include <onex-kernel/log.h>
#endif
#include <onex-kernel/serial.h>

#include "../../lib/buffer.c"

static volatile bool initialised=false;

static serial_recv_cb recv_cb;

#ifndef USBD_POWER_DETECTION
#define USBD_POWER_DETECTION true
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
            buffer_clear();
            if(recv_cb) recv_cb(0,0);
            NRF_LOG_INFO("CDC ACM port opened");
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
        {
            buffer_clear();
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
        {
            buffer_write_chunk();
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
        {
            ret_code_t ret;
            static uint8_t index = 0;
            index++;

            do
            {
                if ((m_cdc_data_array[index - 1] == '\n') ||
                    (m_cdc_data_array[index - 1] == '\r') ||
                    (index >= (INPUT_BUF_SIZE)))
                {
                    if (index > 1)
                    {
                        uint16_t length = (uint16_t)index;
                        if(recv_cb) recv_cb(m_cdc_data_array, length);
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

#define MAX_TX_OCTETS NRFX_USBD_EPSIZE
static char chunk[MAX_TX_OCTETS];

static uint8_t write_a_chunk(size_t size)
{
  ret_code_t e=app_usbd_cdc_acm_write(&m_app_cdc_acm, chunk, size);

#if !defined(LOG_TO_SERIAL)
  if(e==NRF_ERROR_BUSY){
    log_write("busy\n");
  }
  else
  if(e==NRF_ERROR_INVALID_STATE){
    log_write("closed\n");
  }
  else{
    const char* ers=nrf_strerror_get(e);
    log_write("%s", ers+10);
  }
#endif

  return e==NRF_SUCCESS? BUFFER_WRITE_DONE: BUFFER_WRITE_FAILED;
}

bool serial_init(serial_recv_cb cb, uint32_t baudrate)
{
    recv_cb = cb;

    if(initialised) return true;

    app_usbd_serial_num_generate();

    ret_code_t ret;
    static const app_usbd_config_t usbd_config = {
        .ev_state_proc = usbd_user_ev_handler
    };

    ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);
    NRF_LOG_INFO("USBD CDC ACM example started.");

    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);

    if (USBD_POWER_DETECTION)
    {
        ret = app_usbd_power_events_enable();
        APP_ERROR_CHECK(ret);
    }
    else
    {
        NRF_LOG_INFO("No USB power detection enabled\r\nStarting USB now");

        app_usbd_enable();
        app_usbd_start();
    }

    buffer_init(chunk, MAX_TX_OCTETS, write_a_chunk);
    initialised=true;

    return true;
}

void serial_loop()
{
  app_usbd_event_queue_process();
}

void serial_cb(serial_recv_cb cb)
{
    recv_cb = cb;
}

void serial_putchar(unsigned char ch)
{
  if(!initialised) serial_init(0,0);
  serial_write(&ch, 1);
}

size_t serial_write(unsigned char* buf, size_t size)
{
  return buffer_write(buf, size);
}

size_t serial_printf(const char* fmt, ...)
{
  if(!initialised) serial_init(0,0);
  va_list args;
  va_start(args, fmt);
  size_t r=serial_vprintf(fmt,args);
  va_end(args);
  return r;
}

#define PRINT_BUF_SIZE 1024
static unsigned char print_buf[PRINT_BUF_SIZE];

size_t serial_vprintf(const char* fmt, va_list args)
{
  size_t r=vsnprintf((char*)print_buf, PRINT_BUF_SIZE, fmt, args);
  if(r>=PRINT_BUF_SIZE) r=PRINT_BUF_SIZE-1;
  return serial_write(print_buf, r);
}

