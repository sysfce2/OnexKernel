
#include <nrf_pwr_mgmt.h>
#include <nrf_bootloader_info.h>
#include <nrf_soc.h>
#include <onex-kernel/boot.h>

void boot_init()
{
  // see errata [88] WDT: Increased current consumption when configured to pause in System ON idle
  NRF_WDT->CONFIG = NRF_WDT->CONFIG = (WDT_CONFIG_SLEEP_Run  << WDT_CONFIG_SLEEP_Pos) |
                                      (WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos );
  NRF_WDT->CRV = (5*32768); // 5s
  NRF_WDT->RREN |= WDT_RREN_RR0_Msk;
  NRF_WDT->TASKS_START = 1;

  sd_power_mode_set(NRF_POWER_MODE_LOWPWR);
}

void boot_feed_watchdog()
{
  NRF_WDT->RR[0] = WDT_RR_RR_Reload;
}

void boot_dfu_start()
{
  sd_power_gpregret_clr(0, 0xffffffff);
  sd_power_gpregret_set(0, BOOTLOADER_DFU_START);
  nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_DFU);
}

void boot_sleep()
{
  nrf_pwr_mgmt_run();
}
