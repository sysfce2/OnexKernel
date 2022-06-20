##########################################################################
# nRF5 Makefile

targets:
	@grep '^[a-zA-Z0-9\.#-]\+:' makefile | grep -v '^\.' | grep -v targets | sed 's/:.*//' | uniq | sed 's/\.elf/.hex/' | sed 's/^/make clean \&\& make /'

##########################################################################
# set a link to the nordic SDK, something like:
# ./sdk -> /home/<username>/nordic-platform/nRF5_SDK_16.0.0_98a08e2/

GCC_ARM_TOOLCHAIN = /home/duncan/gcc-arm/bin/
GCC_ARM_PREFIX = arm-none-eabi

PRIVATE_PEM = ./doc/local/private.pem

#######################

COMMON_DEFINES = \
-DAPP_TIMER_V2 \
-DAPP_TIMER_V2_RTC1_ENABLED \
-DBOARD_PCA10059 \
-DCONFIG_GPIO_AS_PINRESET \
-DFLOAT_ABI_HARD \
-DNRF52840_XXAA \
-DNRF5 \
-DNRF_SD_BLE_API_VERSION=7 \
-DS140 \
-DSOFTDEVICE_PRESENT \
-D__HEAP_SIZE=8192 \
-D__STACK_SIZE=8192 \


ASSEMBLER_DEFINES = \
$(COMMON_DEFINES) \


COMPILER_DEFINES = \
$(COMMON_DEFINES) \
-DLOG_TO_SERIAL \
-DHAS_SERIAL \
-DONP_CHANNEL_SERIAL \
-DONP_OVER_SERIAL \


INCLUDES = \
-I./include \
-I./src/platforms/nRF5/ \
-I./src/ \
-I./src/onp/ \
-I./tests \
$(SDK_INCLUDES) \

#######################

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
./src/platforms/nRF5/properties.c \
./src/platforms/nRF5/time.c \
./src/platforms/nRF5/random.c \
./src/platforms/nRF5/gpio.c \
./src/platforms/nRF5/serial.c \
./src/platforms/nRF5/blenus.c \
./src/platforms/nRF5/log.c \
./src/platforms/nRF5/mem.c \
./src/platforms/nRF5/channel-serial.c \

############################################

SDK_INCLUDES = \
-I./sdk/modules/nrfx/soc \
-I./sdk/components/nfc/ndef/generic/message \
-I./sdk/components/nfc/t2t_lib \
-I./sdk/components/nfc/t4t_parser/hl_detection_procedure \
-I./sdk/components/ble/ble_services/ble_ancs_c \
-I./sdk/components/ble/ble_services/ble_ias_c \
-I./sdk/components/libraries/pwm \
-I./sdk/components/libraries/usbd/class/cdc/acm \
-I./sdk/components/libraries/usbd/class/hid/generic \
-I./sdk/components/libraries/usbd/class/msc \
-I./sdk/components/libraries/usbd/class/hid \
-I./sdk/modules/nrfx/hal \
-I./sdk/components/nfc/ndef/conn_hand_parser/le_oob_rec_parser \
-I./sdk/components/libraries/log \
-I./sdk/components/ble/ble_services/ble_gls \
-I./sdk/components/libraries/fstorage \
-I./sdk/components/nfc/ndef/text \
-I./sdk/components/libraries/mutex \
-I./sdk/components/libraries/gpiote \
-I./sdk/components/libraries/bootloader/ble_dfu \
-I./sdk/components/nfc/ndef/connection_handover/common \
-I./sdk/components/boards \
-I./sdk/components/nfc/ndef/generic/record \
-I./sdk/components/nfc/t4t_parser/cc_file \
-I./sdk/components/ble/ble_advertising \
-I./sdk/components/ble/ble_link_ctx_manager \
-I./sdk/external/utf_converter \
-I./sdk/components/ble/ble_services/ble_bas_c \
-I./sdk/modules/nrfx/drivers/include \
-I./sdk/components/libraries/experimental_task_manager \
-I./sdk/components/ble/ble_services/ble_hrs_c \
-I./sdk/components/softdevice/s140/headers/nrf52 \
-I./sdk/components/nfc/ndef/connection_handover/le_oob_rec \
-I./sdk/components/libraries/queue \
-I./sdk/components/libraries/pwr_mgmt \
-I./sdk/components/ble/ble_dtm \
-I./sdk/components/toolchain/cmsis/include \
-I./sdk/components/ble/ble_services/ble_rscs_c \
-I./sdk/components/ble/common \
-I./sdk/components/ble/ble_services/ble_lls \
-I./sdk/components/nfc/platform \
-I./sdk/components/nfc/ndef/connection_handover/ac_rec \
-I./sdk/components/ble/ble_services/ble_bas \
-I./sdk/components/libraries/mpu \
-I./sdk/components/libraries/experimental_section_vars \
-I./sdk/components/ble/ble_services/ble_ans_c \
-I./sdk/components/libraries/slip \
-I./sdk/components/libraries/delay \
-I./sdk/components/libraries/csense_drv \
-I./sdk/components/libraries/memobj \
-I./sdk/components/ble/ble_services/ble_nus_c \
-I./sdk/components/softdevice/common \
-I./sdk/components/ble/ble_services/ble_ias \
-I./sdk/components/libraries/usbd/class/hid/mouse \
-I./sdk/components/libraries/low_power_pwm \
-I./sdk/components/nfc/ndef/conn_hand_parser/ble_oob_advdata_parser \
-I./sdk/components/ble/ble_services/ble_dfu \
-I./sdk/external/fprintf \
-I./sdk/components/libraries/svc \
-I./sdk/components/libraries/atomic \
-I./sdk/components \
-I./sdk/components/libraries/scheduler \
-I./sdk/components/libraries/cli \
-I./sdk/components/ble/ble_services/ble_lbs \
-I./sdk/components/ble/ble_services/ble_hts \
-I./sdk/components/libraries/crc16 \
-I./sdk/components/nfc/t4t_parser/apdu \
-I./sdk/components/libraries/util \
-I./sdk/components/libraries/bsp \
-I./sdk/components/libraries/usbd/class/cdc \
-I./sdk/components/libraries/csense \
-I./sdk/components/libraries/balloc \
-I./sdk/components/libraries/ecc \
-I./sdk/components/libraries/hardfault \
-I./sdk/components/libraries/cli/uart \
-I./sdk/components/ble/ble_services/ble_cscs \
-I./sdk/components/libraries/hci \
-I./sdk/components/libraries/timer \
-I./sdk/components/softdevice/s140/headers \
-I./sdk/integration/nrfx \
-I./sdk/components/nfc/t4t_parser/tlv \
-I./sdk/components/libraries/sortlist \
-I./sdk/components/libraries/spi_mngr \
-I./sdk/components/libraries/led_softblink \
-I./sdk/components/nfc/ndef/conn_hand_parser \
-I./sdk/components/libraries/sdcard \
-I./sdk/components/nfc/ndef/parser/record \
-I./sdk/modules/nrfx/mdk \
-I./sdk/components/ble/ble_services/ble_cts_c \
-I./sdk/components/ble/ble_services/ble_nus \
-I./sdk/components/libraries/twi_mngr \
-I./sdk/components/ble/ble_services/ble_hids \
-I./sdk/components/libraries/strerror \
-I./sdk/components/libraries/crc32 \
-I./sdk/components/nfc/ndef/connection_handover/ble_oob_advdata \
-I./sdk/components/nfc/t2t_parser \
-I./sdk/components/nfc/ndef/connection_handover/ble_pair_msg \
-I./sdk/components/libraries/usbd/class/audio \
-I./sdk/components/nfc/t4t_lib \
-I./sdk/components/ble/peer_manager \
-I./sdk/components/libraries/mem_manager \
-I./sdk/components/libraries/ringbuf \
-I./sdk/components/ble/ble_services/ble_tps \
-I./sdk/components/nfc/ndef/parser/message \
-I./sdk/components/ble/ble_services/ble_dis \
-I./sdk/components/nfc/ndef/uri \
-I./sdk/components/ble/nrf_ble_gatt \
-I./sdk/components/ble/nrf_ble_qwr \
-I./sdk/components/libraries/gfx \
-I./sdk/components/libraries/button \
-I./sdk/modules/nrfx \
-I./sdk/components/libraries/twi_sensor \
-I./sdk/integration/nrfx/legacy \
-I./sdk/components/libraries/usbd/class/hid/kbd \
-I./sdk/components/nfc/ndef/connection_handover/ep_oob_rec \
-I./sdk/external/segger_rtt \
-I./sdk/components/libraries/atomic_fifo \
-I./sdk/components/ble/ble_services/ble_lbs_c \
-I./sdk/components/nfc/ndef/connection_handover/ble_pair_lib \
-I./sdk/components/libraries/crypto \
-I./sdk/components/ble/ble_racp \
-I./sdk/components/libraries/fds \
-I./sdk/components/nfc/ndef/launchapp \
-I./sdk/components/libraries/atomic_flags \
-I./sdk/components/ble/ble_services/ble_hrs \
-I./sdk/components/ble/ble_services/ble_rscs \
-I./sdk/components/nfc/ndef/connection_handover/hs_rec \
-I./sdk/components/libraries/usbd \
-I./sdk/components/nfc/ndef/conn_hand_parser/ac_rec_parser \
-I./sdk/components/libraries/stack_guard \
-I./sdk/components/libraries/log/src \


SDK_ASSEMBLER_SOURCES = \
./sdk/modules/nrfx/mdk/gcc_startup_nrf52840.S \


SDK_C_SOURCES = \
./sdk/components/libraries/mem_manager/mem_manager.c \
./sdk/components/libraries/log/src/nrf_log_backend_rtt.c \
./sdk/components/libraries/log/src/nrf_log_backend_serial.c \
./sdk/components/libraries/log/src/nrf_log_backend_uart.c \
./sdk/components/libraries/log/src/nrf_log_default_backends.c \
./sdk/components/libraries/log/src/nrf_log_frontend.c \
./sdk/components/libraries/log/src/nrf_log_str_formatter.c \
./sdk/components/libraries/button/app_button.c \
./sdk/components/libraries/util/app_error.c \
./sdk/components/libraries/util/app_error_handler_gcc.c \
./sdk/components/libraries/util/app_error_weak.c \
./sdk/components/libraries/scheduler/app_scheduler.c \
./sdk/components/libraries/timer/app_timer2.c \
./sdk/components/libraries/util/app_util_platform.c \
./sdk/components/libraries/timer/drv_rtc.c \
./sdk/components/libraries/hardfault/hardfault_implementation.c \
./sdk/components/libraries/util/nrf_assert.c \
./sdk/components/libraries/atomic_fifo/nrf_atfifo.c \
./sdk/components/libraries/atomic_flags/nrf_atflags.c \
./sdk/components/libraries/atomic/nrf_atomic.c \
./sdk/components/libraries/balloc/nrf_balloc.c \
./sdk/components/libraries/cli/nrf_cli.c \
./sdk/components/libraries/cli/uart/nrf_cli_uart.c \
./sdk/external/fprintf/nrf_fprintf.c \
./sdk/external/fprintf/nrf_fprintf_format.c \
./sdk/components/libraries/memobj/nrf_memobj.c \
./sdk/components/libraries/pwr_mgmt/nrf_pwr_mgmt.c \
./sdk/components/libraries/queue/nrf_queue.c \
./sdk/components/libraries/ringbuf/nrf_ringbuf.c \
./sdk/components/libraries/experimental_section_vars/nrf_section_iter.c \
./sdk/components/libraries/sortlist/nrf_sortlist.c \
./sdk/components/libraries/strerror/nrf_strerror.c \
./sdk/components/libraries/usbd/app_usbd.c \
./sdk/components/libraries/usbd/class/cdc/acm/app_usbd_cdc_acm.c \
./sdk/components/libraries/usbd/app_usbd_core.c \
./sdk/components/libraries/usbd/app_usbd_serial_num.c \
./sdk/components/libraries/usbd/app_usbd_string_desc.c \
./sdk/modules/nrfx/mdk/system_nrf52840.c \
./sdk/components/boards/boards.c \
./sdk/integration/nrfx/legacy/nrf_drv_clock.c \
./sdk/integration/nrfx/legacy/nrf_drv_power.c \
./sdk/integration/nrfx/legacy/nrf_drv_uart.c \
./sdk/modules/nrfx/soc/nrfx_atomic.c \
./sdk/modules/nrfx/drivers/src/nrfx_clock.c \
./sdk/modules/nrfx/drivers/src/prs/nrfx_prs.c \
./sdk/modules/nrfx/drivers/src/nrfx_systick.c \
./sdk/modules/nrfx/drivers/src/nrfx_power.c \
./sdk/modules/nrfx/drivers/src/nrfx_uart.c \
./sdk/modules/nrfx/drivers/src/nrfx_uarte.c \
./sdk/components/libraries/bsp/bsp.c \
./sdk/components/libraries/bsp/bsp_cli.c \
./sdk/external/segger_rtt/SEGGER_RTT.c \
./sdk/external/segger_rtt/SEGGER_RTT_Syscalls_GCC.c \
./sdk/external/segger_rtt/SEGGER_RTT_printf.c \
./sdk/components/ble/common/ble_advdata.c \
./sdk/components/ble/common/ble_conn_params.c \
./sdk/modules/nrfx/drivers/src/nrfx_usbd.c \
./sdk/components/ble/common/ble_conn_state.c \
./sdk/components/ble/common/ble_srv_common.c \
./sdk/components/ble/nrf_ble_gatt/nrf_ble_gatt.c \
./sdk/components/ble/nrf_ble_qwr/nrf_ble_qwr.c \
./sdk/external/utf_converter/utf.c \
./sdk/components/ble/ble_services/ble_nus/ble_nus.c \
./sdk/components/ble/ble_services/ble_lbs/ble_lbs.c \
./sdk/components/ble/ble_link_ctx_manager/ble_link_ctx_manager.c \
./sdk/components/softdevice/common/nrf_sdh.c \
./sdk/components/softdevice/common/nrf_sdh_ble.c \
./sdk/components/softdevice/common/nrf_sdh_soc.c \


##########################################################################
# Targets

nrf.lib: libonex-kernel-nrf.a

libonex-kernel-nrf.a: $(LIB_SOURCES:.c=.o) $(NRF5_SOURCES:.c=.o) $(SDK_C_SOURCES:.c=.o) $(SDK_ASSEMBLER_SOURCES:.S=.o)
	rm -f $@
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-ar rcs $@ $^
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-strip -g $@

nrf.tests: $(TESTS_SOURCES:.c=.o) $(LIB_SOURCES:.c=.o) $(NRF5_SOURCES:.c=.o) $(SDK_C_SOURCES:.c=.o) $(SDK_ASSEMBLER_SOURCES:.S=.o)
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(LINKER_FLAGS) $(LD_FILES) -Wl,-Map=./onex-kernel.map -o ./onex-kernel.out $^
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size ./onex-kernel.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onex-kernel.out ./onex-kernel.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onex-kernel.out ./onex-kernel.hex

flash0: nrf.tests
	nrfutil pkg generate --hw-version 52 --sd-req 0xCA --application-version 1 --application ./onex-kernel.hex --key-file $(PRIVATE_PEM) dfu.zip
	nrfutil dfu usb-serial -pkg dfu.zip -p /dev/ttyACM0 -b 115200

############################################

ASSEMBLER_FLAGS = -x assembler-with-cpp -MP -MD -c -g3 -mcpu=cortex-m4 -mthumb -mabi=aapcs -mfloat-abi=hard -mfpu=fpv4-sp-d16

COMPILER_FLAGS = -std=c99 -MP -MD -O3 -g3 -mcpu=cortex-m4 -mthumb -mabi=aapcs -Wall -Werror -mfloat-abi=hard -mfpu=fpv4-sp-d16 -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin -fshort-enums

LINKER_FLAGS = -O3 -g3 -mthumb -mabi=aapcs -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,--gc-sections --specs=nano.specs

LD_FILES = -L./sdk/modules/nrfx/mdk -T./src/platforms/nRF5/onex.ld

.S.o:
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(ASSEMBLER_FLAGS) $(ASSEMBLER_DEFINES) $(INCLUDES) -o $@ -c $<

.c.o:
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(COMPILER_FLAGS) $(COMPILER_DEFINES) $(INCLUDES) -o $@ -c $<

clean:
	find src tests -name '*.o' -o -name '*.d' | xargs rm -f
	find . -name onex.ondb | xargs rm -f
	rm -f onex-kernel.??? dfu.zip core
	@echo "------------------------------"
	@echo "files not cleaned:"
	@git ls-files --others --exclude-from=.git/info/exclude | xargs -r ls -Fla

############################################
