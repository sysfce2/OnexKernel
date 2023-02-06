#-------------------------------------------------------------------------------
# nRF5 Makefile

targets:
	@grep '^[a-zA-Z0-9\.#-]\+:' makefile | grep -v '^\.' | grep -v targets | sed 's/:.*//' | uniq | sed 's/\.elf/.hex/' | sed 's/^/make clean \&\& make -j /'

#-------------------------------------------------------------------------------
# set a link to the nordic SDK, something like:
# ./sdk -> /home/<username>/nordic-platform/nRF5_SDK_16.0.0_98a08e2/

GCC_ARM_TOOLCHAIN = /home/duncan/gcc-arm/bin/
GCC_ARM_PREFIX = arm-none-eabi

PRIVATE_PEM = ./doc/local/private.pem

#-------------------------------------------------------------------------------

COMMON_DEFINES_NO_SD = \
-DAPP_TIMER_V2 \
-DAPP_TIMER_V2_RTC1_ENABLED \
-DCONFIG_GPIO_AS_PINRESET \
-DFLOAT_ABI_HARD \
-DNRF5 \


COMMON_DEFINES = \
-DAPP_TIMER_V2 \
-DAPP_TIMER_V2_RTC1_ENABLED \
-DCONFIG_GPIO_AS_PINRESET \
-DFLOAT_ABI_HARD \
-DNRF5 \
-DNRF_SD_BLE_API_VERSION=7 \
-DSOFTDEVICE_PRESENT \


COMMON_DEFINES_PINETIME_BL = \
$(COMMON_DEFINES) \
-DBOARD_PINETIME \
-DNRF52832_XXAA \
-DS132 \
-DNRF52 \
-DNRF52_PAN_74 \
-DBLE_STACK_SUPPORT_REQD \
-DNRF_DFU_SETTINGS_VERSION=2 \
-DNRF_DFU_SVCI_ENABLED \
-DSVC_INTERFACE_CALL_AS_NORMAL_FUNCTION \
-DuECC_ENABLE_VLI_API=0 \
-DuECC_OPTIMIZATION_LEVEL=3 \
-DuECC_SQUARE_FUNC=0 \
-DuECC_SUPPORT_COMPRESSED_POINT=0 \
-DuECC_VLI_NATIVE_LITTLE_ENDIAN=1 \
-D__HEAP_SIZE=0 \


COMMON_DEFINES_PINETIME = \
$(COMMON_DEFINES) \
-DBOARD_PINETIME \
-DNRF52832_XXAA \
-DS132 \
-DNRF52_PAN_74 \
-D__HEAP_SIZE=8192 \
-D__STACK_SIZE=8192 \
#-DSPI_BLOCKING \
#-DLOG_TO_GFX \



COMMON_DEFINES_MAGIC3 = \
$(COMMON_DEFINES_NO_SD) \
-DBOARD_MAGIC3 \
-DNRF52840_XXAA \
-D__HEAP_SIZE=8192 \
-D__STACK_SIZE=8192 \
#-DSPI_BLOCKING \
#-DLOG_TO_GFX \



COMMON_DEFINES_DONGLE = \
$(COMMON_DEFINES) \
-DBOARD_PCA10059 \
-DNRF52840_XXAA \
-DS140 \
-D__HEAP_SIZE=8192 \
-D__STACK_SIZE=8192 \



ASSEMBLER_DEFINES_PINETIME_BL = \
$(COMMON_DEFINES_PINETIME_BL) \


ASSEMBLER_DEFINES_PINETIME = \
$(COMMON_DEFINES_PINETIME) \


ASSEMBLER_DEFINES_MAGIC3 = \
$(COMMON_DEFINES_MAGIC3) \


ASSEMBLER_DEFINES_DONGLE = \
$(COMMON_DEFINES_DONGLE) \


COMPILER_DEFINES_PINETIME_BL = \
$(COMMON_DEFINES_PINETIME_BL) \


COMPILER_DEFINES_PINETIME = \
$(COMMON_DEFINES_PINETIME) \
-DONP_CHANNEL_SERIAL \


COMPILER_DEFINES_MAGIC3 = \
$(COMMON_DEFINES_MAGIC3) \


COMPILER_DEFINES_DONGLE = \
$(COMMON_DEFINES_DONGLE) \
-DLOG_TO_SERIAL \
-DHAS_SERIAL \
-DONP_CHANNEL_SERIAL \
-DONP_DEBUG \
-DONP_OVER_SERIAL \
# above are baked in but need to be runtime options!


INCLUDES_PINETIME_BL = \
-I./include \
-I./src/onl/nRF5/pinetime-bl \
-I./src/ \
$(SDK_INCLUDES_S132_BL) \


INCLUDES_PINETIME = \
-I./include \
-I./src/onl/nRF5/pinetime \
-I./src/ \
-I./src/onp/ \
-I./tests \
$(SDK_INCLUDES_PINETIME) \


INCLUDES_MAGIC3 = \
-I./include \
-I./src/onl/nRF5/magic3 \
-I./src/ \
-I./src/onp/ \
-I./tests \
$(SDK_INCLUDES_MAGIC3) \


INCLUDES_DONGLE = \
-I./include \
-I./src/onl/nRF5/dongle \
-I./src/ \
-I./src/onp/ \
-I./tests \
$(SDK_INCLUDES_DONGLE) \

#-------------------------------------------------------------------------------

BOOTLOADER_SOURCES = \
./src/onl/nRF5/gpio.c \
./src/onl/nRF5/spi.c \
./src/onl/nRF5/display-st7789.c \
./src/onl/nRF5/gfx.c \
./src/onl/nRF5/dfu_public_key.c \
./src/onl/nRF5/bootloader.c \


TESTS_SOURCES = \
./tests/test-properties.c \
./tests/test-list.c \
./tests/test-value.c \
./tests/test-onn.c \
./tests/main.c \


LIB_SOURCES = \
./src/lib/list.c \
./src/lib/value.c \
./src/lib/tests.c \
./src/onp/onp.c \
./src/onn/onn.c \


NRF5_SOURCES = \
./src/onl/nRF5/properties.c \
./src/onl/nRF5/time.c \
./src/onl/nRF5/random.c \
./src/onl/nRF5/gpio.c \
./src/onl/nRF5/log.c \
./src/onl/nRF5/mem.c \
./src/onl/nRF5/channel-serial.c \


PINETIME_SOURCES = \
./src/onl/nRF5/boot.c \
./src/onl/nRF5/i2c.c \
./src/onl/nRF5/spi.c \
./src/onl/nRF5/touch-cst816s.c \
./src/onl/nRF5/motion-bma421.c \
./src/onl/nRF5/display-st7789.c \
./src/onl/nRF5/gfx.c \
./src/onl/nRF5/blenus.c \
$(NRF5_SOURCES) \


MAGIC3_SOURCES = \
./src/onl/nRF5/boot.c \
./src/onl/nRF5/i2c.c \
./src/onl/nRF5/spi.c \
./src/onl/nRF5/touch-cst816s.c \
./src/onl/nRF5/display-st7789.c \
./src/onl/nRF5/gfx.c \
$(NRF5_SOURCES) \


DONGLE_SOURCES = \
./src/onl/nRF5/serial.c \
./src/onl/nRF5/blenus.c \
$(NRF5_SOURCES) \

#-------------------------------------------------------------------------------

SDK_INCLUDES_S132_BL = \
-I./mod-sdk/components/libraries/gfx \
-I./mod-sdk/components/boards \
-I./mod-sdk/components/libraries/mem_manager \
-I./sdk/external/thedotfactory_fonts \
-I./sdk/modules/nrfx/drivers/include \
-I./sdk/components/libraries/gfx \
-I./sdk/examples/dfu/secure_bootloader \
-I./sdk/components/libraries/crypto/backend/micro_ecc \
-I./sdk/components/softdevice/s132/headers \
-I./sdk/components/libraries/memobj \
-I./sdk/components/libraries/sha256 \
-I./sdk/components/libraries/crc32 \
-I./sdk/components/libraries/experimental_section_vars \
-I./sdk/components/libraries/mem_manager \
-I./sdk/components/libraries/fstorage \
-I./sdk/components/libraries/util \
-I./sdk/modules/nrfx \
-I./sdk/external/nrf_oberon/include \
-I./sdk/components/libraries/crypto/backend/oberon \
-I./sdk/components/libraries/crypto/backend/cifra \
-I./sdk/components/libraries/atomic \
-I./sdk/integration/nrfx \
-I./sdk/components/libraries/crypto/backend/cc310_bl \
-I./sdk/components/softdevice/s132/headers/nrf52 \
-I./sdk/components/libraries/log/src \
-I./sdk/components/libraries/bootloader/dfu \
-I./sdk/components/ble/common \
-I./sdk/components/libraries/delay \
-I./sdk/components/libraries/svc \
-I./sdk/components/libraries/stack_info \
-I./sdk/components/libraries/crypto/backend/nrf_hw \
-I./sdk/components/libraries/log \
-I./sdk/external/nrf_oberon \
-I./sdk/components/libraries/strerror \
-I./sdk/components/libraries/crypto/backend/mbedtls \
-I./sdk/components/boards \
-I./sdk/components/libraries/crypto/backend/cc310 \
-I./sdk/components/libraries/bootloader \
-I./sdk/external/fprintf \
-I./sdk/components/libraries/crypto \
-I./sdk/components/libraries/crypto/backend/optiga \
-I./sdk/components/libraries/scheduler \
-I./sdk/modules/nrfx/hal \
-I./sdk/components/toolchain/cmsis/include \
-I./sdk/components/libraries/balloc \
-I./sdk/components/libraries/atomic_fifo \
-I/home/duncan/nordic/micro-ecc \
-I./sdk/components/libraries/crypto/backend/nrf_sw \
-I./sdk/modules/nrfx/mdk \
-I./sdk/components/libraries/bootloader/ble_dfu \
-I./sdk/components/softdevice/common \
-I./sdk/external/nano-pb \
-I./sdk/components/libraries/queue \
-I./sdk/components/libraries/ringbuf \


SDK_INCLUDES_MAGIC3 = \
-I./mod-sdk/components/libraries/gfx \
-I./sdk/external/thedotfactory_fonts \
-I./sdk/components/drivers_nrf/nrf_soc_nosd/ \
-I./sdk/components/softdevice/mbr/headers/ \
$(SDK_INCLUDES) \


SDK_INCLUDES_PINETIME = \
-I./mod-sdk/components/libraries/gfx \
-I./sdk/external/thedotfactory_fonts \
-I./sdk/components/softdevice/s132/headers \
-I./sdk/components/softdevice/s132/headers/nrf52 \
$(SDK_INCLUDES) \


SDK_INCLUDES_DONGLE = \
-I./sdk/components/libraries/bsp \
-I./sdk/components/libraries/cli/uart \
-I./sdk/components/softdevice/s140/headers \
-I./sdk/components/softdevice/s140/headers/nrf52 \
$(SDK_INCLUDES) \


SDK_INCLUDES = \
-I./mod-sdk/components/boards \
-I./mod-sdk/components/libraries/mem_manager \
-I./sdk/components \
-I./sdk/components/ble/ble_advertising \
-I./sdk/components/ble/ble_dtm \
-I./sdk/components/ble/ble_link_ctx_manager \
-I./sdk/components/ble/ble_racp \
-I./sdk/components/ble/ble_services/ble_ancs_c \
-I./sdk/components/ble/ble_services/ble_ans_c \
-I./sdk/components/ble/ble_services/ble_bas \
-I./sdk/components/ble/ble_services/ble_bas_c \
-I./sdk/components/ble/ble_services/ble_cscs \
-I./sdk/components/ble/ble_services/ble_cts_c \
-I./sdk/components/ble/ble_services/ble_dfu \
-I./sdk/components/ble/ble_services/ble_dis \
-I./sdk/components/ble/ble_services/ble_gls \
-I./sdk/components/ble/ble_services/ble_hids \
-I./sdk/components/ble/ble_services/ble_hrs \
-I./sdk/components/ble/ble_services/ble_hrs_c \
-I./sdk/components/ble/ble_services/ble_hts \
-I./sdk/components/ble/ble_services/ble_ias \
-I./sdk/components/ble/ble_services/ble_ias_c \
-I./sdk/components/ble/ble_services/ble_lbs \
-I./sdk/components/ble/ble_services/ble_lbs_c \
-I./sdk/components/ble/ble_services/ble_lls \
-I./sdk/components/ble/ble_services/ble_nus \
-I./sdk/components/ble/ble_services/ble_nus_c \
-I./sdk/components/ble/ble_services/ble_rscs \
-I./sdk/components/ble/ble_services/ble_rscs_c \
-I./sdk/components/ble/ble_services/ble_tps \
-I./sdk/components/ble/common \
-I./sdk/components/ble/nrf_ble_gatt \
-I./sdk/components/ble/nrf_ble_qwr \
-I./sdk/components/ble/peer_manager \
-I./sdk/components/boards \
-I./sdk/components/libraries/atomic \
-I./sdk/components/libraries/atomic_fifo \
-I./sdk/components/libraries/atomic_flags \
-I./sdk/components/libraries/balloc \
-I./sdk/components/libraries/bootloader/ \
-I./sdk/components/libraries/bootloader/ble_dfu \
-I./sdk/components/libraries/bsp \
-I./sdk/components/libraries/button \
-I./sdk/components/libraries/cli \
-I./sdk/components/libraries/cli/uart \
-I./sdk/components/libraries/crc16 \
-I./sdk/components/libraries/crc32 \
-I./sdk/components/libraries/crypto \
-I./sdk/components/libraries/crypto/backend/cc310 \
-I./sdk/components/libraries/crypto/backend/cc310_bl \
-I./sdk/components/libraries/crypto/backend/mbedtls \
-I./sdk/components/libraries/crypto/backend/oberon \
-I./sdk/components/libraries/crypto/backend/micro_ecc \
-I./sdk/components/libraries/crypto/backend/optiga \
-I./sdk/components/libraries/crypto/backend/cifra \
-I./sdk/components/libraries/crypto/backend/nrf_sw \
-I./sdk/components/libraries/crypto/backend/nrf_hw \
-I./sdk/components/libraries/csense \
-I./sdk/components/libraries/csense_drv \
-I./sdk/components/libraries/delay \
-I./sdk/components/libraries/ecc \
-I./sdk/components/libraries/experimental_section_vars \
-I./sdk/components/libraries/experimental_task_manager \
-I./sdk/components/libraries/fds \
-I./sdk/components/libraries/fstorage \
-I./sdk/components/libraries/gpiote \
-I./sdk/components/libraries/hardfault \
-I./sdk/components/libraries/hci \
-I./sdk/components/libraries/led_softblink \
-I./sdk/components/libraries/log \
-I./sdk/components/libraries/log/src \
-I./sdk/components/libraries/low_power_pwm \
-I./sdk/components/libraries/mem_manager \
-I./sdk/components/libraries/memobj \
-I./sdk/components/libraries/mpu \
-I./sdk/components/libraries/mutex \
-I./sdk/components/libraries/pwm \
-I./sdk/components/libraries/pwr_mgmt \
-I./sdk/components/libraries/queue \
-I./sdk/components/libraries/ringbuf \
-I./sdk/components/libraries/scheduler \
-I./sdk/components/libraries/sdcard \
-I./sdk/components/libraries/slip \
-I./sdk/components/libraries/sortlist \
-I./sdk/components/libraries/spi_mngr \
-I./sdk/components/libraries/stack_guard \
-I./sdk/components/libraries/stack_info \
-I./sdk/components/libraries/strerror \
-I./sdk/components/libraries/svc \
-I./sdk/components/libraries/timer \
-I./sdk/components/libraries/twi_mngr \
-I./sdk/components/libraries/twi_sensor \
-I./sdk/components/libraries/usbd \
-I./sdk/components/libraries/usbd/class/audio \
-I./sdk/components/libraries/usbd/class/cdc \
-I./sdk/components/libraries/usbd/class/cdc/acm \
-I./sdk/components/libraries/usbd/class/hid \
-I./sdk/components/libraries/usbd/class/hid/generic \
-I./sdk/components/libraries/usbd/class/hid/kbd \
-I./sdk/components/libraries/usbd/class/hid/mouse \
-I./sdk/components/libraries/usbd/class/msc \
-I./sdk/components/libraries/util \
-I./sdk/components/nfc/ndef/conn_hand_parser \
-I./sdk/components/nfc/ndef/conn_hand_parser/ac_rec_parser \
-I./sdk/components/nfc/ndef/conn_hand_parser/ble_oob_advdata_parser \
-I./sdk/components/nfc/ndef/conn_hand_parser/le_oob_rec_parser \
-I./sdk/components/nfc/ndef/connection_handover/ac_rec \
-I./sdk/components/nfc/ndef/connection_handover/ble_oob_advdata \
-I./sdk/components/nfc/ndef/connection_handover/ble_pair_lib \
-I./sdk/components/nfc/ndef/connection_handover/ble_pair_msg \
-I./sdk/components/nfc/ndef/connection_handover/common \
-I./sdk/components/nfc/ndef/connection_handover/ep_oob_rec \
-I./sdk/components/nfc/ndef/connection_handover/hs_rec \
-I./sdk/components/nfc/ndef/connection_handover/le_oob_rec \
-I./sdk/components/nfc/ndef/generic/message \
-I./sdk/components/nfc/ndef/generic/record \
-I./sdk/components/nfc/ndef/launchapp \
-I./sdk/components/nfc/ndef/parser/message \
-I./sdk/components/nfc/ndef/parser/record \
-I./sdk/components/nfc/ndef/text \
-I./sdk/components/nfc/ndef/uri \
-I./sdk/components/nfc/platform \
-I./sdk/components/nfc/t2t_lib \
-I./sdk/components/nfc/t2t_parser \
-I./sdk/components/nfc/t4t_lib \
-I./sdk/components/nfc/t4t_parser/apdu \
-I./sdk/components/nfc/t4t_parser/cc_file \
-I./sdk/components/nfc/t4t_parser/hl_detection_procedure \
-I./sdk/components/nfc/t4t_parser/tlv \
-I./sdk/components/softdevice/common \
-I./sdk/components/toolchain/cmsis/include \
-I./sdk/external/fprintf \
-I./sdk/external/segger_rtt \
-I./sdk/external/utf_converter \
-I./sdk/integration/nrfx \
-I./sdk/integration/nrfx/legacy \
-I./sdk/modules/nrfx \
-I./sdk/modules/nrfx/drivers/include \
-I./sdk/modules/nrfx/hal \
-I./sdk/modules/nrfx/mdk \
-I./sdk/modules/nrfx/soc \


SDK_ASSEMBLER_SOURCES_52832 = \
./sdk/modules/nrfx/mdk/gcc_startup_nrf52.S \


SDK_ASSEMBLER_SOURCES_52840 = \
./sdk/modules/nrfx/mdk/gcc_startup_nrf52840.S \


SDK_C_SOURCES_PINETIME_BL = \
./mod-sdk/components/libraries/bootloader/nrf_bootloader.c \
./mod-sdk/components/libraries/gfx/nrf_gfx.c \
./sdk/components/ble/common/ble_srv_common.c \
./sdk/components/libraries/atomic/nrf_atomic.c \
./sdk/components/libraries/atomic_fifo/nrf_atfifo.c \
./sdk/components/libraries/balloc/nrf_balloc.c \
./sdk/components/libraries/experimental_section_vars/nrf_section_iter.c \
./sdk/components/libraries/scheduler/app_scheduler.c \
./sdk/components/libraries/util/app_util_platform.c \
./sdk/components/softdevice/common/nrf_sdh.c \
./sdk/components/softdevice/common/nrf_sdh_ble.c \
./sdk/modules/nrfx/drivers/src/prs/nrfx_prs.c \
./sdk/components/libraries/bootloader/ble_dfu/nrf_dfu_ble.c \
./sdk/components/libraries/bootloader/dfu/dfu-cc.pb.c \
./sdk/components/libraries/bootloader/dfu/nrf_dfu.c \
./sdk/components/libraries/bootloader/dfu/nrf_dfu_flash.c \
./sdk/components/libraries/bootloader/dfu/nrf_dfu_handling_error.c \
./sdk/components/libraries/bootloader/dfu/nrf_dfu_mbr.c \
./sdk/components/libraries/bootloader/dfu/nrf_dfu_req_handler.c \
./sdk/components/libraries/bootloader/dfu/nrf_dfu_settings.c \
./sdk/components/libraries/bootloader/dfu/nrf_dfu_settings_svci.c \
./sdk/components/libraries/bootloader/dfu/nrf_dfu_svci.c \
./sdk/components/libraries/bootloader/dfu/nrf_dfu_svci_handler.c \
./sdk/components/libraries/bootloader/dfu/nrf_dfu_transport.c \
./sdk/components/libraries/bootloader/dfu/nrf_dfu_utils.c \
./sdk/components/libraries/bootloader/dfu/nrf_dfu_validation.c \
./sdk/components/libraries/bootloader/dfu/nrf_dfu_ver_validation.c \
./sdk/components/libraries/bootloader/nrf_bootloader_app_start.c \
./sdk/components/libraries/bootloader/nrf_bootloader_app_start_final.c \
./sdk/components/libraries/bootloader/nrf_bootloader_dfu_timers.c \
./sdk/components/libraries/bootloader/nrf_bootloader_fw_activation.c \
./sdk/components/libraries/bootloader/nrf_bootloader_info.c \
./sdk/components/libraries/bootloader/nrf_bootloader_wdt.c \
./sdk/components/libraries/crc32/crc32.c \
./sdk/components/libraries/crypto/backend/micro_ecc/micro_ecc_backend_ecc.c \
./sdk/components/libraries/crypto/backend/micro_ecc/micro_ecc_backend_ecdsa.c \
./sdk/components/libraries/crypto/backend/nrf_sw/nrf_sw_backend_hash.c \
./sdk/components/libraries/crypto/backend/oberon/oberon_backend_ecc.c \
./sdk/components/libraries/crypto/backend/oberon/oberon_backend_ecdsa.c \
./sdk/components/libraries/crypto/nrf_crypto_ecc.c \
./sdk/components/libraries/crypto/nrf_crypto_ecdsa.c \
./sdk/components/libraries/crypto/nrf_crypto_hash.c \
./sdk/components/libraries/crypto/nrf_crypto_init.c \
./sdk/components/libraries/crypto/nrf_crypto_shared.c \
./sdk/components/libraries/fstorage/nrf_fstorage.c \
./sdk/components/libraries/fstorage/nrf_fstorage_nvmc.c \
./sdk/components/libraries/fstorage/nrf_fstorage_sd.c \
./sdk/components/libraries/sha256/sha256.c \
./sdk/components/libraries/svc/nrf_svc_handler.c \
./sdk/components/softdevice/common/nrf_sdh_soc.c \
/home/duncan/nordic/micro-ecc/uECC.c \
./sdk/external/nano-pb/pb_common.c \
./sdk/external/nano-pb/pb_decode.c \
./sdk/modules/nrfx/drivers/src/nrfx_spim.c \
./sdk/modules/nrfx/hal/nrf_nvmc.c \
./sdk/modules/nrfx/mdk/system_nrf52.c \



SDK_C_SOURCES_PINETIME = \
$(SDK_C_SOURCES) \
./mod-sdk/components/libraries/gfx/nrf_gfx.c \
./sdk/external/thedotfactory_fonts/orkney8pts.c \
./sdk/modules/nrfx/drivers/src/nrfx_saadc.c \
./sdk/modules/nrfx/drivers/src/nrfx_spim.c \
./sdk/modules/nrfx/drivers/src/nrfx_twi.c \
./sdk/modules/nrfx/mdk/system_nrf52.c \


SDK_C_SOURCES_MAGIC3 = \
$(SDK_C_SOURCES_NO_SD) \
./mod-sdk/components/libraries/gfx/nrf_gfx.c \
./sdk/external/thedotfactory_fonts/orkney8pts.c \
./sdk/modules/nrfx/drivers/src/nrfx_saadc.c \
./sdk/modules/nrfx/drivers/src/nrfx_spim.c \
./sdk/modules/nrfx/drivers/src/nrfx_twi.c \
./sdk/modules/nrfx/mdk/system_nrf52840.c \


SDK_C_SOURCES_DONGLE = \
$(SDK_C_SOURCES) \
./sdk/components/libraries/bsp/bsp.c \
./sdk/components/libraries/bsp/bsp_cli.c \
./sdk/components/libraries/cli/nrf_cli.c \
./sdk/components/libraries/cli/uart/nrf_cli_uart.c \
./sdk/components/libraries/log/src/nrf_log_backend_uart.c \
./sdk/components/libraries/queue/nrf_queue.c \
./sdk/components/libraries/usbd/app_usbd.c \
./sdk/components/libraries/usbd/app_usbd_core.c \
./sdk/components/libraries/usbd/app_usbd_serial_num.c \
./sdk/components/libraries/usbd/app_usbd_string_desc.c \
./sdk/components/libraries/usbd/class/cdc/acm/app_usbd_cdc_acm.c \
./sdk/integration/nrfx/legacy/nrf_drv_power.c \
./sdk/integration/nrfx/legacy/nrf_drv_uart.c \
./sdk/modules/nrfx/drivers/src/nrfx_power.c \
./sdk/modules/nrfx/drivers/src/nrfx_systick.c \
./sdk/modules/nrfx/drivers/src/nrfx_uart.c \
./sdk/modules/nrfx/drivers/src/nrfx_uarte.c \
./sdk/modules/nrfx/drivers/src/nrfx_usbd.c \
./sdk/modules/nrfx/mdk/system_nrf52840.c \


SDK_C_SOURCES = \
./sdk/components/ble/ble_link_ctx_manager/ble_link_ctx_manager.c \
./sdk/components/ble/ble_services/ble_lbs/ble_lbs.c \
./sdk/components/ble/ble_services/ble_nus/ble_nus.c \
./sdk/components/ble/common/ble_advdata.c \
./sdk/components/ble/common/ble_conn_params.c \
./sdk/components/ble/common/ble_conn_state.c \
./sdk/components/ble/common/ble_srv_common.c \
./sdk/components/ble/nrf_ble_gatt/nrf_ble_gatt.c \
./sdk/components/ble/nrf_ble_qwr/nrf_ble_qwr.c \
./sdk/components/softdevice/common/nrf_sdh.c \
./sdk/components/softdevice/common/nrf_sdh_ble.c \
./sdk/components/softdevice/common/nrf_sdh_soc.c \
$(SDK_C_SOURCES_NO_SD) \


SDK_C_SOURCES_NO_SD = \
./mod-sdk/components/libraries/mem_manager/mem_manager.c \
./mod-sdk/components/boards/boards.c \
./sdk/components/libraries/crypto/backend/nrf_hw/nrf_hw_backend_rng.c \
./sdk/components/libraries/crypto/backend/nrf_hw/nrf_hw_backend_init.c \
./sdk/components/libraries/crypto/nrf_crypto_init.c \
./sdk/components/libraries/crypto/nrf_crypto_rng.c \
./sdk/components/libraries/atomic/nrf_atomic.c \
./sdk/components/libraries/atomic_fifo/nrf_atfifo.c \
./sdk/components/libraries/atomic_flags/nrf_atflags.c \
./sdk/components/libraries/balloc/nrf_balloc.c \
./sdk/components/libraries/button/app_button.c \
./sdk/components/libraries/experimental_section_vars/nrf_section_iter.c \
./sdk/components/libraries/hardfault/hardfault_implementation.c \
./sdk/components/libraries/log/src/nrf_log_backend_rtt.c \
./sdk/components/libraries/log/src/nrf_log_backend_serial.c \
./sdk/components/libraries/log/src/nrf_log_default_backends.c \
./sdk/components/libraries/log/src/nrf_log_frontend.c \
./sdk/components/libraries/log/src/nrf_log_str_formatter.c \
./sdk/components/libraries/memobj/nrf_memobj.c \
./sdk/components/libraries/pwr_mgmt/nrf_pwr_mgmt.c \
./sdk/components/libraries/ringbuf/nrf_ringbuf.c \
./sdk/components/libraries/scheduler/app_scheduler.c \
./sdk/components/libraries/sortlist/nrf_sortlist.c \
./sdk/components/libraries/strerror/nrf_strerror.c \
./sdk/components/libraries/timer/app_timer2.c \
./sdk/components/libraries/timer/drv_rtc.c \
./sdk/components/libraries/util/app_error.c \
./sdk/components/libraries/util/app_error_handler_gcc.c \
./sdk/components/libraries/util/app_error_weak.c \
./sdk/components/libraries/util/app_util_platform.c \
./sdk/components/libraries/util/nrf_assert.c \
./sdk/components/libraries/queue/nrf_queue.c \
./sdk/external/fprintf/nrf_fprintf.c \
./sdk/external/fprintf/nrf_fprintf_format.c \
./sdk/external/segger_rtt/SEGGER_RTT.c \
./sdk/external/segger_rtt/SEGGER_RTT_Syscalls_GCC.c \
./sdk/external/segger_rtt/SEGGER_RTT_printf.c \
./sdk/external/utf_converter/utf.c \
./sdk/integration/nrfx/legacy/nrf_drv_clock.c \
./sdk/integration/nrfx/legacy/nrf_drv_rng.c \
./sdk/modules/nrfx/drivers/src/nrfx_rng.c \
./sdk/modules/nrfx/drivers/src/nrfx_clock.c \
./sdk/modules/nrfx/drivers/src/prs/nrfx_prs.c \
./sdk/modules/nrfx/soc/nrfx_atomic.c \

#-------------------------------------------------------------------------------
# Targets

libonex-kernel-pinetime.a: INCLUDES=$(INCLUDES_PINETIME)
libonex-kernel-pinetime.a: ASSEMBLER_DEFINES=$(ASSEMBLER_DEFINES_PINETIME)
libonex-kernel-pinetime.a: COMPILER_DEFINES=$(COMPILER_DEFINES_PINETIME)
libonex-kernel-pinetime.a: $(LIB_SOURCES:.c=.o) $(PINETIME_SOURCES:.c=.o) $(SDK_C_SOURCES_PINETIME:.c=.o) $(SDK_ASSEMBLER_SOURCES_52832:.S=.o)
	rm -f $@
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-ar rcs $@ $^
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-strip -g $@


libonex-kernel-magic3.a: INCLUDES=$(INCLUDES_MAGIC3)
libonex-kernel-magic3.a: ASSEMBLER_DEFINES=$(ASSEMBLER_DEFINES_MAGIC3)
libonex-kernel-magic3.a: COMPILER_DEFINES=$(COMPILER_DEFINES_MAGIC3)
libonex-kernel-magic3.a: $(LIB_SOURCES:.c=.o) $(MAGIC3_SOURCES:.c=.o) $(SDK_C_SOURCES_MAGIC3:.c=.o) $(SDK_ASSEMBLER_SOURCES_52840:.S=.o)
	rm -f $@
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-ar rcs $@ $^
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-strip -g $@


libonex-kernel-dongle.a: INCLUDES=$(INCLUDES_DONGLE)
libonex-kernel-dongle.a: ASSEMBLER_DEFINES=$(ASSEMBLER_DEFINES_DONGLE)
libonex-kernel-dongle.a: COMPILER_DEFINES=$(COMPILER_DEFINES_DONGLE)
libonex-kernel-dongle.a: $(LIB_SOURCES:.c=.o) $(DONGLE_SOURCES:.c=.o) $(SDK_C_SOURCES_DONGLE:.c=.o) $(SDK_ASSEMBLER_SOURCES_52840:.S=.o)
	rm -f $@
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-ar rcs $@ $^
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-strip -g $@


nrf.bootloader.pinetime: INCLUDES=$(INCLUDES_PINETIME_BL)
nrf.bootloader.pinetime: ASSEMBLER_DEFINES=$(ASSEMBLER_DEFINES_PINETIME_BL)
nrf.bootloader.pinetime: COMPILER_DEFINES=$(COMPILER_DEFINES_PINETIME_BL)
nrf.bootloader.pinetime: $(BOOTLOADER_SOURCES:.c=.o) $(SDK_C_SOURCES_PINETIME_BL:.c=.o) $(SDK_ASSEMBLER_SOURCES_52832:.S=.o)
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(LINKER_FLAGS) $(LD_FILES_PINETIME_BL) -Wl,-Map=./onex-kernel-bootloader.map -o ./onex-kernel-bootloader.out $^
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size ./onex-kernel-bootloader.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onex-kernel-bootloader.out ./onex-kernel-bootloader.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onex-kernel-bootloader.out ./onex-kernel-bootloader.hex


nrf.tests.pinetime: INCLUDES=$(INCLUDES_PINETIME)
nrf.tests.pinetime: ASSEMBLER_DEFINES=$(ASSEMBLER_DEFINES_PINETIME)
nrf.tests.pinetime: COMPILER_DEFINES=$(COMPILER_DEFINES_PINETIME)
nrf.tests.pinetime: libonex-kernel-pinetime.a $(TESTS_SOURCES:.c=.o)
	rm -rf oko
	mkdir oko
	ar x ./libonex-kernel-pinetime.a --output oko
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(LINKER_FLAGS) $(LD_FILES_PINETIME) -Wl,-Map=./onex-kernel.map -o ./onex-kernel.out $(TESTS_SOURCES:.c=.o) oko/* -lm
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size ./onex-kernel.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onex-kernel.out ./onex-kernel.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onex-kernel.out ./onex-kernel.hex


nrf.tests.magic3: INCLUDES=$(INCLUDES_MAGIC3)
nrf.tests.magic3: ASSEMBLER_DEFINES=$(ASSEMBLER_DEFINES_MAGIC3)
nrf.tests.magic3: COMPILER_DEFINES=$(COMPILER_DEFINES_MAGIC3)
nrf.tests.magic3: libonex-kernel-magic3.a $(TESTS_SOURCES:.c=.o)
	rm -rf oko
	mkdir oko
	ar x ./libonex-kernel-magic3.a --output oko
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(LINKER_FLAGS) $(LD_FILES_MAGIC3) -Wl,-Map=./onex-kernel.map -o ./onex-kernel.out $(TESTS_SOURCES:.c=.o) oko/* -lm
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size ./onex-kernel.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onex-kernel.out ./onex-kernel.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onex-kernel.out ./onex-kernel.hex


nrf.tests.dongle: INCLUDES=$(INCLUDES_DONGLE)
nrf.tests.dongle: ASSEMBLER_DEFINES=$(ASSEMBLER_DEFINES_DONGLE)
nrf.tests.dongle: COMPILER_DEFINES=$(COMPILER_DEFINES_DONGLE)
nrf.tests.dongle: libonex-kernel-dongle.a $(TESTS_SOURCES:.c=.o)
	rm -rf oko
	mkdir oko
	ar x ./libonex-kernel-dongle.a --output oko
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(LINKER_FLAGS) $(LD_FILES_DONGLE) -Wl,-Map=./onex-kernel.map -o ./onex-kernel.out $(TESTS_SOURCES:.c=.o) oko/* -lm
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size ./onex-kernel.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onex-kernel.out ./onex-kernel.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onex-kernel.out ./onex-kernel.hex

bootloader: nrf.bootloader.pinetime
	echo $$(($$(cat bootloader-number.txt) + 1)) > bootloader-number.txt

pinetime-erase-flash-sd-and-bl: bootloader
	openocd -f ./doc/openocd-stlink.cfg -c init -c "reset halt" -c "nrf5 mass_erase" -c "reset run" -c exit
	openocd -f ./doc/openocd-stlink.cfg -c init -c "reset halt" -c "program ./sdk/components/softdevice/s132/hex/s132_nrf52_7.0.1_softdevice.hex" -c "reset run" -c exit
	openocd -f ./doc/openocd-stlink.cfg -c init -c "reset halt" -c "program ./onex-kernel-bootloader.hex" -c "reset run" -c exit

pinetime-flash-bl: nrf.bootloader.pinetime
	openocd -f ./doc/openocd-stlink.cfg -c init -c "reset halt" -c "program ./onex-kernel-bootloader.hex" -c "reset run" -c exit

#-------------------------------:

pinetime-flash: nrf.tests.pinetime
	openocd -f ./doc/openocd-stlink.cfg -c init -c "reset halt" -c "program ./onex-kernel.hex" -c "reset run" -c exit

magic3-flash: nrf.tests.magic3
	openocd -f ./doc/openocd-stlink.cfg -c init -c "reset halt" -c "program ./onex-kernel.hex" -c "reset run" -c exit

dongle-flash: nrf.tests.dongle
	nrfutil pkg generate --hw-version 52 --sd-req 0xCA --application-version 1 --application ./onex-kernel.hex --key-file $(PRIVATE_PEM) dfu.zip
	nrfutil dfu usb-serial -pkg dfu.zip -p /dev/ttyACM0 -b 115200

#-------------------------------:

device-halt:
	openocd -f ./doc/openocd-stlink.cfg -c init -c "reset halt" -c exit

device-reset:
	openocd -f ./doc/openocd-stlink.cfg -c init -c "reset halt" -c "reset run" -c exit

device-erase:
	openocd -f ./doc/openocd-stlink.cfg -c init -c "reset halt" -c "nrf5 mass_erase" -c "reset run" -c exit

flash-sd132:
	openocd -f ./doc/openocd-stlink.cfg -c init -c "reset halt" -c "program ./sdk/components/softdevice/s132/hex/s132_nrf52_7.0.1_softdevice.hex" -c "reset run" -c exit

#-------------------------------------------------------------------------------

# for bootloader: -O2 -ggdb
# for bootloader: -Os -g3
LINKER_FLAGS = -O3 -g3 -mthumb -mabi=aapcs -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,--gc-sections --specs=nano.specs

LD_FILES_PINETIME_BL = -L./sdk/modules/nrfx/mdk -T./src/onl/nRF5/pinetime-bl/onex.ld
LD_FILES_PINETIME    = -L./sdk/modules/nrfx/mdk -T./src/onl/nRF5/pinetime/onex.ld
LD_FILES_DONGLE      = -L./sdk/modules/nrfx/mdk -T./src/onl/nRF5/dongle/onex.ld
LD_FILES_MAGIC3      = -L./sdk/modules/nrfx/mdk -T./src/onl/nRF5/magic3/onex.ld

ASSEMBLER_FLAGS = -c -g3 -mcpu=cortex-m4 -mthumb -mabi=aapcs -mfloat-abi=hard -mfpu=fpv4-sp-d16

COMPILER_FLAGS = -std=c99 -O3 -g3 -mcpu=cortex-m4 -mthumb -mabi=aapcs -Wall -Werror -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -mfloat-abi=hard -mfpu=fpv4-sp-d16 -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin -fshort-enums

.S.o:
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(ASSEMBLER_FLAGS) $(ASSEMBLER_DEFINES) $(INCLUDES) -o $@ -c $<

.c.o:
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(COMPILER_FLAGS) $(COMPILER_DEFINES) $(INCLUDES) -o $@ -c $<

clean:
	find src tests mod-sdk -name '*.o' -o -name '*.d' | xargs rm -f
	find . -name onex.ondb | xargs rm -f
	touch ./sdk/banana-mango.o; find ./sdk/ -name '*.o' | xargs rm
	rm -rf onex-kernel*.??? dfu.zip core oko
	rm -f ,*
	@echo "------------------------------"
	@echo "files not cleaned:"
	@git ls-files --others --exclude-from=.git/info/exclude | xargs -r ls -Fla

#-------------------------------------------------------------------------------
