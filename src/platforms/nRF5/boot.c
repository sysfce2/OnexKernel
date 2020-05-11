
#include <nrf_pwr_mgmt.h>
#include <nrf_bootloader_info.h>
#include <nrf_soc.h>
#include <onex-kernel/boot.h>

void boot_dfu_start()
{
  sd_power_gpregret_clr(0, 0xffffffff);
  sd_power_gpregret_set(0, BOOTLOADER_DFU_START);
  nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_DFU);
}

