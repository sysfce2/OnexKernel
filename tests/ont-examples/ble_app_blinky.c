
#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "boards.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#ifdef HAS_SERIAL
#include <onex-kernel/serial.h>
#endif
#include <onex-kernel/blenus.h>
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <assert.h>

#define DEAD_BEEF 0xDEADBEEF  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

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

void on_recv(unsigned char* buf, size_t size)
{
  blenus_write(buf, size);
}

int main(void)
{
    power_management_init();

    log_init();
    time_init();
#ifdef HAS_SERIAL
    serial_init(on_recv,0);
    blenus_init(0); // should write to serial! (just use blenus_init(serial_write)?)
#else
    blenus_init(on_recv);
#endif

    while (true)
    {
#ifdef HAS_SERIAL
      serial_loop();
#endif
      idle_state_handle();
    }
}


