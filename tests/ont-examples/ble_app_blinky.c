
#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "boards.h"
#include "app_timer.h"
#include "app_button.h"
#include "ble_nus.h"
#include "ble_lbs.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_drv_usbd.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include <onex-kernel/serial.h>
#include <onex-kernel/blenus.h>
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <assert.h>

#define DEAD_BEEF                       0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}

/**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
static uint16_t m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;

void serial_in(unsigned char* buf, size_t size)
{
  ret_code_t err_code;
  if(size>= m_ble_nus_max_data_len) return;
  do
  {
    err_code = blenus_write(buf, size);

    if (err_code == NRF_ERROR_NOT_FOUND)
    {
        NRF_LOG_INFO("BLE NUS unavailable");
        break;
    }
    if (err_code == NRF_ERROR_RESOURCES)
    {
        NRF_LOG_ERROR("BLE NUS Too many notifications queued.");
        break;
    }
    if ((err_code != NRF_ERROR_INVALID_STATE) && (err_code != NRF_ERROR_BUSY))
    {
        APP_ERROR_CHECK(err_code);
    }
  } while (err_code == NRF_ERROR_BUSY);
}

extern void run_properties_tests();

// static unsigned char m_tx_buffer[NRF_DRV_USBD_EPSIZE];

int main(void)
{
    power_management_init();

    log_init();
    time_init();
    serial_init(serial_in,0);
    blenus_init(0);

    while (true)
    {
        serial_loop();
/*
        if(m_send_flag)
        {
            static int frame_counter;
            if(frame_counter==1000){
              run_properties_tests();
              onex_assert_summary();
            }
            size_t size = sprintf((char*)m_tx_buffer, "-- %u\r\n", frame_counter);
            serial_write(m_tx_buffer, size);
            frame_counter++;
        }
*/
        idle_state_handle();
    }
}


