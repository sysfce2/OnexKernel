
#include <nrf_wdt.h>
#include <nrf_pwr_mgmt.h>
#include <nrf_bootloader_info.h>
#include <nrf_soc.h>
#include <onex-kernel/boot.h>

void boot_init()
{
  // 0=Pause in SLEEP and HALT
  // 1=Run in SLEEP, Pause in HALT
  // 8=Pause in SLEEP, Run in HALT
  // 9=Run in SLEEP and HALT
  nrf_wdt_behaviour_set(1);
  nrf_wdt_reload_value_set(5 * 32768); // 5s
  nrf_wdt_reload_request_enable(NRF_WDT_RR0);
  nrf_wdt_task_trigger(NRF_WDT_TASK_START);

  sd_power_mode_set(NRF_POWER_MODE_LOWPWR);
}

void boot_feed_watchdog()
{
  nrf_wdt_reload_request_set(NRF_WDT_RR0);
}

void boot_dfu_start()
{
  sd_power_gpregret_clr(0, 0xffffffff);
  sd_power_gpregret_set(0, BOOTLOADER_DFU_START);
  nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_DFU);
}

void boot_sleep()
{
  sd_app_evt_wait();
}
