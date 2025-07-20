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

COMMON_DEFINES = \
-DAPP_TIMER_V2 \
-DAPP_TIMER_V2_RTC1_ENABLED \
-DCONFIG_GPIO_AS_PINRESET \
-DFLOAT_ABI_HARD \
-DNRF5 \


COMMON_DEFINES_MAGIC3 = \
$(COMMON_DEFINES) \
-DBOARD_MAGIC3 \
-DNRF52840_XXAA \
-D__HEAP_SIZE=8192 \
-D__STACK_SIZE=8192 \
#-DGRIND_THE_MEM \
#-DSPI_BLOCKING \


COMMON_DEFINES_ADAFRUIT_DONGLE = \
$(COMMON_DEFINES) \
-DBOARD_ADAFRUIT_DONGLE \
-DNRF52840_XXAA \
-D__HEAP_SIZE=8192 \
-D__STACK_SIZE=8192 \



COMMON_DEFINES_ITSYBITSY = \
$(COMMON_DEFINES) \
-DBOARD_ITSYBITSY \
-DNRF52840_XXAA \
-D__HEAP_SIZE=8192 \
-D__STACK_SIZE=8192 \



COMMON_DEFINES_FEATHER_SENSE = \
$(COMMON_DEFINES) \
-DBOARD_FEATHER_SENSE \
-DNRF52840_XXAA \
-D__HEAP_SIZE=8192 \
-D__STACK_SIZE=8192 \



COMMON_DEFINES_DONGLE = \
$(COMMON_DEFINES) \
-DBOARD_PCA10059 \
-DNRF52840_XXAA \
-D__HEAP_SIZE=8192 \
-D__STACK_SIZE=8192 \


ASSEMBLER_DEFINES_MAGIC3 = \
$(COMMON_DEFINES_MAGIC3) \


ASSEMBLER_DEFINES_ADAFRUIT_DONGLE = \
$(COMMON_DEFINES_ADAFRUIT_DONGLE) \


ASSEMBLER_DEFINES_ITSYBITSY = \
$(COMMON_DEFINES_ITSYBITSY) \


ASSEMBLER_DEFINES_FEATHER_SENSE = \
$(COMMON_DEFINES_FEATHER_SENSE) \


ASSEMBLER_DEFINES_DONGLE = \
$(COMMON_DEFINES_DONGLE) \


COMPILER_DEFINES_MAGIC3 = \
$(COMMON_DEFINES_MAGIC3) \


COMPILER_DEFINES_ADAFRUIT_DONGLE = \
$(COMMON_DEFINES_ADAFRUIT_DONGLE) \


COMPILER_DEFINES_ITSYBITSY = \
$(COMMON_DEFINES_ITSYBITSY) \


COMPILER_DEFINES_FEATHER_SENSE = \
$(COMMON_DEFINES_FEATHER_SENSE) \


COMPILER_DEFINES_DONGLE = \
$(COMMON_DEFINES_DONGLE) \


INCLUDES_MAGIC3 = \
-I./include \
-I./src/ \
-I./src/onl/nRF5/magic3 \
-I./src/onl/ \
-I./src/onn/ \
-I./src/onp/ \
-I./tests \
$(SDK_INCLUDES_MAGIC3) \


INCLUDES_ADAFRUIT_DONGLE = \
-I./include \
-I./src/ \
-I./src/onl/nRF5/adafruit-dongle \
-I./src/onl/ \
-I./src/onn/ \
-I./src/onp/ \
-I./tests \
$(SDK_INCLUDES_ADAFRUIT_DONGLE) \


INCLUDES_ITSYBITSY = \
-I./include \
-I./src/ \
-I./src/onl/nRF5/itsybitsy \
-I./src/onl/ \
-I./src/onn/ \
-I./src/onp/ \
-I./tests \
$(SDK_INCLUDES_ITSYBITSY) \


INCLUDES_FEATHER_SENSE = \
-I./include \
-I./src/ \
-I./src/onl/nRF5/feather-sense \
-I./src/onl/ \
-I./src/onn/ \
-I./src/onp/ \
-I./tests \
$(SDK_INCLUDES_FEATHER_SENSE) \


INCLUDES_DONGLE = \
-I./include \
-I./src/ \
-I./src/onl/nRF5/dongle \
-I./src/onl/ \
-I./src/onn/ \
-I./src/onp/ \
-I./tests \
$(SDK_INCLUDES_DONGLE) \


#-------------------------------------------------------------------------------

TESTS_SOURCES = \
./tests/test-properties.c \
./tests/test-list.c \
./tests/test-value.c \
./tests/test-database.c \
./tests/test-onn.c \
./tests/main.c \


LIB_SOURCES = \
./src/lib/lib.c \
./src/lib/colours.c \
./src/lib/chunkbuf.c \
./src/lib/database.c \
./src/lib/list.c \
./src/lib/value.c \
./src/lib/tests.c \
./src/lib/properties.c \
./src/onp/behaviours.c \
./src/onp/onp.c \
./src/onn/onn.c \


NRF5_SOURCES = \
./src/onl/nRF5/boot.c \
./src/onl/nRF5/time.c \
./src/onl/nRF5/random.c \
./src/onl/nRF5/gpio.c \
./src/onl/nRF5/log.c \
./src/onl/nRF5/mem.c \
./src/onl/nRF5/persistence.c \
./src/onl/nRF5/serial.c \
./src/onl/nRF5/radio.c \
./src/onl/nRF5/ipv6.c \


MAGIC3_SOURCES = \
./src/onl/nRF5/boot.c \
./src/onl/nRF5/i2c.c \
./src/onl/nRF5/spi.c \
./src/onl/nRF5/gfx.c \
./src/onl/nRF5/spi-flash.c \
./src/onl/drivers/touch-cst816s.c \
./src/onl/drivers/display-st7789.c \
$(NRF5_SOURCES) \


ADAFRUIT_TFT_SOURCES = \
./src/onl/drivers/tft.c \
./src/onl/drivers/adafruit_tft28.c \


ADAFRUIT_DONGLE_SOURCES = \
$(NRF5_SOURCES) \


ITSYBITSY_SOURCES = \
$(NRF5_SOURCES) \


FEATHER_SENSE_SOURCES = \
./src/onl/nRF5/i2c.c \
./src/onl/nRF5/seesaw.c \
./src/onl/nRF5/spi.c \
./src/onl/drivers/neopixel.c \
./src/onl/drivers/feather-dotstar.c \
./src/onl/drivers/compass-lis3mdl.c \
$(NRF5_SOURCES) \


DONGLE_SOURCES = \
$(NRF5_SOURCES) \


PCR_BUTTON_SOURCES = \
./tests/main-pcr-button-nrf.c \


PCR_SOURCES = \
./tests/main-pcr.c \


PCR_LIGHT_SOURCES = \
./tests/main-pcr-light-nrf.c \


MOON_SOURCES = \
./tests/ont-examples/moon/moon.c \

#-------------------------------------------------------------------------------

SDK_INCLUDES_MAGIC3 = \
-I./mod-sdk/components/libraries/gfx  \
-I./sdk/external/thedotfactory_fonts  \
-I./sdk/components/drivers_nrf/nrf_soc_nosd/ \
-I./sdk/components/softdevice/mbr/headers/ \
$(SDK_INCLUDES) \


SDK_INCLUDES_ADAFRUIT_DONGLE = \
-I./sdk/components/libraries/bsp \
-I./sdk/components/libraries/cli/uart \
-I./sdk/components/drivers_nrf/nrf_soc_nosd/ \
-I./sdk/components/softdevice/mbr/headers/ \
$(SDK_INCLUDES) \


SDK_INCLUDES_ITSYBITSY = \
-I./sdk/components/libraries/bsp \
-I./sdk/components/libraries/cli/uart \
-I./sdk/components/drivers_nrf/nrf_soc_nosd/ \
-I./sdk/components/softdevice/mbr/headers/ \
$(SDK_INCLUDES) \


SDK_INCLUDES_FEATHER_SENSE = \
-I./sdk/components/libraries/bsp \
-I./sdk/components/libraries/cli/uart \
-I./sdk/components/drivers_nrf/nrf_soc_nosd/ \
-I./sdk/components/softdevice/mbr/headers/ \
$(SDK_INCLUDES) \


SDK_INCLUDES_DONGLE = \
-I./sdk/components/libraries/bsp \
-I./sdk/components/libraries/cli/uart \
-I./sdk/components/drivers_nrf/nrf_soc_nosd/ \
-I./sdk/components/softdevice/mbr/headers/ \
$(SDK_INCLUDES) \


SDK_INCLUDES = \
-I./mod-sdk/components/boards \
-I./sdk/components \
-I./sdk/components/boards \
-I./sdk/components/libraries/atomic \
-I./sdk/components/libraries/atomic_fifo \
-I./sdk/components/libraries/atomic_flags \
-I./sdk/components/libraries/balloc \
-I./sdk/components/libraries/bootloader/ \
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
-I./sdk/components/nfc/ndef/conn_hand_parser/le_oob_rec_parser \
-I./sdk/components/nfc/ndef/connection_handover/ac_rec \
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


SDK_ASSEMBLER_SOURCES_52840 = \
./sdk/modules/nrfx/mdk/gcc_startup_nrf52840.S \


SDK_C_SOURCES_MAGIC3 = \
$(SDK_C_SOURCES) \
./mod-sdk/components/libraries/gfx/nrf_gfx.c \
./sdk/external/thedotfactory_fonts/orkney8pts.c \
./sdk/modules/nrfx/drivers/src/nrfx_spim.c \
./sdk/modules/nrfx/drivers/src/nrfx_qspi.c \
./sdk/modules/nrfx/drivers/src/nrfx_twi.c \
./sdk/modules/nrfx/mdk/system_nrf52840.c \


SDK_C_SOURCES_ADAFRUIT_DONGLE = \
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


SDK_C_SOURCES_ITSYBITSY = \
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


SDK_C_SOURCES_FEATHER_SENSE = \
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
./mod-sdk/components/boards/boards.c \
./sdk/components/libraries/crypto/backend/nrf_hw/nrf_hw_backend_rng.c \
./sdk/components/libraries/crypto/backend/nrf_hw/nrf_hw_backend_init.c \
./sdk/components/libraries/crypto/nrf_crypto_init.c \
./sdk/components/libraries/crypto/nrf_crypto_rng.c \
./sdk/components/libraries/atomic/nrf_atomic.c \
./sdk/components/libraries/atomic_fifo/nrf_atfifo.c \
./sdk/components/libraries/atomic_flags/nrf_atflags.c \
./sdk/components/libraries/balloc/nrf_balloc.c \
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
./sdk/modules/nrfx/drivers/src/nrfx_saadc.c \
./sdk/modules/nrfx/drivers/src/nrfx_rng.c \
./sdk/modules/nrfx/drivers/src/nrfx_clock.c \
./sdk/modules/nrfx/drivers/src/prs/nrfx_prs.c \
./sdk/modules/nrfx/soc/nrfx_atomic.c \


#-------------------------------------------------------------------------------
# Targets

libonex-kernel-magic3.a: ASSEMBLER_LINE=${M4_CPU} $(ASSEMBLER_DEFINES_MAGIC3)
libonex-kernel-magic3.a: COMPILE_LINE=$(M4_CC_FLAGS) $(COMPILER_DEFINES_MAGIC3) $(INCLUDES_MAGIC3)
libonex-kernel-magic3.a: $(LIB_SOURCES:.c=.o) $(MAGIC3_SOURCES:.c=.o) $(SDK_C_SOURCES_MAGIC3:.c=.o) $(SDK_ASSEMBLER_SOURCES_52840:.S=.o)
	rm -f $@
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-ar rcs $@ $^
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-strip -g $@


libonex-kernel-feather-sense.a: ASSEMBLER_LINE=${M4_CPU} $(ASSEMBLER_DEFINES_FEATHER_SENSE)
libonex-kernel-feather-sense.a: COMPILE_LINE=$(M4_CC_FLAGS) $(COMPILER_DEFINES_FEATHER_SENSE) $(INCLUDES_FEATHER_SENSE)
libonex-kernel-feather-sense.a: $(LIB_SOURCES:.c=.o) $(FEATHER_SENSE_SOURCES:.c=.o) $(SDK_C_SOURCES_FEATHER_SENSE:.c=.o) $(SDK_ASSEMBLER_SOURCES_52840:.S=.o)
	rm -f $@
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-ar rcs $@ $^
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-strip -g $@


libonex-kernel-dongle.a: ASSEMBLER_LINE=${M4_CPU} $(ASSEMBLER_DEFINES_DONGLE)
libonex-kernel-dongle.a: COMPILE_LINE=$(M4_CC_FLAGS) $(COMPILER_DEFINES_DONGLE) $(INCLUDES_DONGLE)
libonex-kernel-dongle.a: $(LIB_SOURCES:.c=.o) $(DONGLE_SOURCES:.c=.o) $(SDK_C_SOURCES_DONGLE:.c=.o) $(SDK_ASSEMBLER_SOURCES_52840:.S=.o)
	rm -f $@
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-ar rcs $@ $^
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-strip -g $@

libonex-kernel-adafruit-dongle.a: ASSEMBLER_LINE=${M4_CPU} $(ASSEMBLER_DEFINES_ADAFRUIT_DONGLE)
libonex-kernel-adafruit-dongle.a: COMPILE_LINE=$(M4_CC_FLAGS) $(COMPILER_DEFINES_ADAFRUIT_DONGLE) $(INCLUDES_ADAFRUIT_DONGLE)
libonex-kernel-adafruit-dongle.a: $(LIB_SOURCES:.c=.o) $(ADAFRUIT_DONGLE_SOURCES:.c=.o) $(SDK_C_SOURCES_ADAFRUIT_DONGLE:.c=.o) $(SDK_ASSEMBLER_SOURCES_52840:.S=.o)
	rm -f $@
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-ar rcs $@ $^
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-strip -g $@


libonex-kernel-itsybitsy.a: ASSEMBLER_LINE=${M4_CPU} $(ASSEMBLER_DEFINES_ITSYBITSY)
libonex-kernel-itsybitsy.a: COMPILE_LINE=$(M4_CC_FLAGS) $(COMPILER_DEFINES_ITSYBITSY) $(INCLUDES_ITSYBITSY)
libonex-kernel-itsybitsy.a: $(LIB_SOURCES:.c=.o) $(ITSYBITSY_SOURCES:.c=.o) $(SDK_C_SOURCES_ITSYBITSY:.c=.o) $(SDK_ASSEMBLER_SOURCES_52840:.S=.o)
	rm -f $@
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-ar rcs $@ $^
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-strip -g $@


#-------------------------------:

nrf.tests.magic3: ASSEMBLER_LINE=${M4_CPU} $(ASSEMBLER_DEFINES_MAGIC3)
nrf.tests.magic3: COMPILE_LINE=$(M4_CC_FLAGS) $(COMPILER_DEFINES_MAGIC3) $(INCLUDES_MAGIC3)
nrf.tests.magic3: libonex-kernel-magic3.a $(TESTS_SOURCES:.c=.o)
	rm -rf oko
	mkdir oko
	ar x ./libonex-kernel-magic3.a --output oko
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(M4_LD_FLAGS) $(LD_FILES_MAGIC3) -Wl,-Map=./onex-kernel.map -o ./onex-kernel.out $(TESTS_SOURCES:.c=.o) oko/* -lm
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size ./onex-kernel.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onex-kernel.out ./onex-kernel.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onex-kernel.out ./onex-kernel.hex


nrf.tests.feather-sense: ASSEMBLER_LINE=${M4_CPU} $(ASSEMBLER_DEFINES_FEATHER_SENSE)
nrf.tests.feather-sense: COMPILE_LINE=$(M4_CC_FLAGS) $(COMPILER_DEFINES_FEATHER_SENSE) $(INCLUDES_FEATHER_SENSE)
nrf.tests.feather-sense: libonex-kernel-feather-sense.a $(TESTS_SOURCES:.c=.o)
	rm -rf oko
	mkdir oko
	ar x ./libonex-kernel-feather-sense.a --output oko
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(M4_LD_FLAGS) $(LD_FILES_FEATHER_SENSE) -Wl,-Map=./onex-kernel.map -o ./onex-kernel.out $(TESTS_SOURCES:.c=.o) oko/* -lm
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size ./onex-kernel.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onex-kernel.out ./onex-kernel.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onex-kernel.out ./onex-kernel.hex


nrf.tests.dongle: ASSEMBLER_LINE=${M4_CPU} $(ASSEMBLER_DEFINES_DONGLE)
nrf.tests.dongle: COMPILE_LINE=$(M4_CC_FLAGS) $(COMPILER_DEFINES_DONGLE) $(INCLUDES_DONGLE)
nrf.tests.dongle: libonex-kernel-dongle.a $(TESTS_SOURCES:.c=.o)
	rm -rf oko
	mkdir oko
	ar x ./libonex-kernel-dongle.a --output oko
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(M4_LD_FLAGS) $(LD_FILES_DONGLE) -Wl,-Map=./onex-kernel.map -o ./onex-kernel.out $(TESTS_SOURCES:.c=.o) oko/* -lm
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size ./onex-kernel.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onex-kernel.out ./onex-kernel.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onex-kernel.out ./onex-kernel.hex

nrf.tests.adafruit-dongle: ASSEMBLER_LINE=${M4_CPU} $(ASSEMBLER_DEFINES_ADAFRUIT_DONGLE)
nrf.tests.adafruit-dongle: COMPILE_LINE=$(M4_CC_FLAGS) $(COMPILER_DEFINES_ADAFRUIT_DONGLE) $(INCLUDES_ADAFRUIT_DONGLE)
nrf.tests.adafruit-dongle: libonex-kernel-adafruit-dongle.a $(TESTS_SOURCES:.c=.o)
	rm -rf oko
	mkdir oko
	ar x ./libonex-kernel-adafruit-dongle.a --output oko
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(M4_LD_FLAGS) $(LD_FILES_ADAFRUIT_DONGLE) -Wl,-Map=./onex-kernel.map -o ./onex-kernel.out $(TESTS_SOURCES:.c=.o) oko/* -lm
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size ./onex-kernel.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onex-kernel.out ./onex-kernel.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onex-kernel.out ./onex-kernel.hex


nrf.tests.itsybitsy: ASSEMBLER_LINE=${M4_CPU} $(ASSEMBLER_DEFINES_ITSYBITSY)
nrf.tests.itsybitsy: COMPILE_LINE=$(M4_CC_FLAGS) $(COMPILER_DEFINES_ITSYBITSY) $(INCLUDES_ITSYBITSY)
nrf.tests.itsybitsy: libonex-kernel-itsybitsy.a $(TESTS_SOURCES:.c=.o)
	rm -rf oko
	mkdir oko
	ar x ./libonex-kernel-itsybitsy.a --output oko
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(M4_LD_FLAGS) $(LD_FILES_ITSYBITSY) -Wl,-Map=./onex-kernel.map -o ./onex-kernel.out $(TESTS_SOURCES:.c=.o) oko/* -lm
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size ./onex-kernel.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onex-kernel.out ./onex-kernel.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onex-kernel.out ./onex-kernel.hex


#-------------------------------:

dongle-pcr: ASSEMBLER_LINE=${M4_CPU} $(ASSEMBLER_DEFINES_DONGLE)
dongle-pcr: COMPILE_LINE=$(M4_CC_FLAGS) $(COMPILER_DEFINES_DONGLE) $(INCLUDES_DONGLE)
dongle-pcr: libonex-kernel-dongle.a $(PCR_SOURCES:.c=.o)
	rm -rf oko
	mkdir oko
	ar x ./libonex-kernel-dongle.a --output oko
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(M4_LD_FLAGS) $(LD_FILES_DONGLE) -Wl,-Map=./onex-kernel.map -o ./onex-kernel.out $(PCR_SOURCES:.c=.o) oko/* -lm
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size ./onex-kernel.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onex-kernel.out ./onex-kernel.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onex-kernel.out ./onex-kernel.hex

feather-pcr: ASSEMBLER_LINE=${M4_CPU} $(ASSEMBLER_DEFINES_FEATHER_SENSE)
feather-pcr: COMPILE_LINE=$(M4_CC_FLAGS) $(COMPILER_DEFINES_FEATHER_SENSE) $(INCLUDES_FEATHER_SENSE)
feather-pcr: libonex-kernel-feather-sense.a $(PCR_SOURCES:.c=.o)
	rm -rf oko
	mkdir oko
	ar x ./libonex-kernel-feather-sense.a --output oko
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(M4_LD_FLAGS) $(LD_FILES_FEATHER_SENSE) -Wl,-Map=./onex-kernel.map -o ./onex-kernel.out $(PCR_SOURCES:.c=.o) oko/* -lm
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size ./onex-kernel.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onex-kernel.out ./onex-kernel.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onex-kernel.out ./onex-kernel.hex

feather-pcr-light: ASSEMBLER_LINE=${M4_CPU} $(ASSEMBLER_DEFINES_FEATHER_SENSE)
feather-pcr-light: COMPILE_LINE=$(M4_CC_FLAGS) $(COMPILER_DEFINES_FEATHER_SENSE) $(INCLUDES_FEATHER_SENSE)
feather-pcr-light: libonex-kernel-feather-sense.a $(PCR_LIGHT_SOURCES:.c=.o)
	rm -rf oko
	mkdir oko
	ar x ./libonex-kernel-feather-sense.a --output oko
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(M4_LD_FLAGS) $(LD_FILES_FEATHER_SENSE) -Wl,-Map=./onex-kernel.map -o ./onex-kernel.out $(PCR_LIGHT_SOURCES:.c=.o) oko/* -lm
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size ./onex-kernel.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onex-kernel.out ./onex-kernel.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onex-kernel.out ./onex-kernel.hex

feather-pcr-button: ASSEMBLER_LINE=${M4_CPU} $(ASSEMBLER_DEFINES_FEATHER_SENSE)
feather-pcr-button: COMPILE_LINE=$(M4_CC_FLAGS) $(COMPILER_DEFINES_FEATHER_SENSE) $(INCLUDES_FEATHER_SENSE)
feather-pcr-button: libonex-kernel-feather-sense.a $(PCR_BUTTON_SOURCES:.c=.o)
	rm -rf oko
	mkdir oko
	ar x ./libonex-kernel-feather-sense.a --output oko
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(M4_LD_FLAGS) $(LD_FILES_FEATHER_SENSE) -Wl,-Map=./onex-kernel.map -o ./onex-kernel.out $(PCR_BUTTON_SOURCES:.c=.o) oko/* -lm
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size ./onex-kernel.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onex-kernel.out ./onex-kernel.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onex-kernel.out ./onex-kernel.hex

feather-moon: ASSEMBLER_LINE=${M4_CPU} $(ASSEMBLER_DEFINES_FEATHER_SENSE)
feather-moon: COMPILE_LINE=$(M4_CC_FLAGS) $(COMPILER_DEFINES_FEATHER_SENSE) $(INCLUDES_FEATHER_SENSE)
feather-moon: libonex-kernel-feather-sense.a $(MOON_SOURCES:.c=.o)
	rm -rf oko
	mkdir oko
	ar x ./libonex-kernel-feather-sense.a --output oko
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(M4_LD_FLAGS) $(LD_FILES_FEATHER_SENSE) -Wl,-Map=./feather-moon.map -o ./feather-moon.out $(MOON_SOURCES:.c=.o) oko/* -lm
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size ./feather-moon.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./feather-moon.out ./feather-moon.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./feather-moon.out ./feather-moon.hex

#-------------------------------:

magic3-tests-flash: nrf.tests.magic3
	openocd -f ./doc/openocd-stlink.cfg -c init -c "reset halt" -c "program ./onex-kernel.hex" -c "reset run" -c exit

feather-sense-tests-flash: nrf.tests.feather-sense
	uf2conv.py onex-kernel.hex --family 0xada52840 --output onex-kernel.uf2

dongle-tests-flash: nrf.tests.dongle
	nrfutil pkg generate --hw-version 52 --sd-req 0x00 --application-version 1 --application ./onex-kernel.hex --key-file $(PRIVATE_PEM) dfu.zip
	nrfutil dfu usb-serial -pkg dfu.zip -p /dev/`ls -l /dev/nordic_dongle_flash | sed 's/.*-> //'` -b 115200

adafruit-dongle-tests-flash: nrf.tests.adafruit-dongle
	uf2conv.py onex-kernel.hex --family 0xada52840 --output onex-kernel.uf2

itsybitsy-tests-flash: nrf.tests.itsybitsy
	uf2conv.py onex-kernel.hex --family 0xada52840 --output onex-kernel.uf2

#-------------------------------:

feather-pcr-button-flash: feather-pcr-button
	uf2conv.py onex-kernel.hex --family 0xada52840 --output onex-kernel.uf2

feather-pcr-flash: feather-pcr
	uf2conv.py onex-kernel.hex --family 0xada52840 --output onex-kernel.uf2

feather-pcr-light-flash: feather-pcr-light
	uf2conv.py onex-kernel.hex --family 0xada52840 --output onex-kernel.uf2

dongle-pcr-flash: dongle-pcr
	nrfutil pkg generate --hw-version 52 --sd-req 0x00 --application-version 1 --application ./onex-kernel.hex --key-file $(PRIVATE_PEM) dfu.zip
	nrfutil dfu usb-serial -pkg dfu.zip -p /dev/`ls -l /dev/nordic_dongle_flash | sed 's/.*-> //'` -b 115200

#-------------------------------:

feather-moon-flash: feather-moon
	uf2conv.py feather-moon.hex --family 0xada52840 --output feather-moon.uf2

#-------------------------------:

device-halt:
	openocd -f ./doc/openocd-stlink.cfg -c init -c "reset halt" -c exit

device-reset:
	openocd -f ./doc/openocd-stlink.cfg -c init -c "reset halt" -c "reset run" -c exit

device-erase:
	openocd -f ./doc/openocd-stlink.cfg -c init -c "reset halt" -c "nrf5 mass_erase" -c "reset run" -c exit

#-------------------------------:

DEBUG_FLAGS = -g -O0
OPTIM_FLAGS = -g3 -O3 # REVISIT: buggy? O2?

#-------------------------------:

M4_CPU = $(OPTIM_FLAGS) -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mabi=aapcs

# REVISIT: is it OK to leave -fno-omit-frame-pointer on?
M4_CC_FLAGS = ${M4_CPU} -std=gnu17
M4_CC_FLAGS += -Wall -Werror -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable
M4_CC_FLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin -fshort-enums
M4_CC_FLAGS += -fno-omit-frame-pointer

# do these for thorough linting:
#M4_CC_FLAGS += -Wextra
#M4_CC_FLAGS += -Wpedantic
#M4_CC_FLAGS += -Wstrict-prototypes

# undo these for thorough linting:
#M4_CC_FLAGS += -Wno-incompatible-pointer-types
#M4_CC_FLAGS += -Wno-sign-compare
M4_CC_FLAGS += -Wno-discarded-qualifiers
M4_CC_FLAGS += -Wno-array-bounds
M4_CC_FLAGS += -Wno-char-subscripts
M4_CC_FLAGS += -Wno-misleading-indentation

M4_LD_FLAGS = $(M4_CPU) -Wl,--gc-sections #-specs=nano.specs

LD_FILES_MAGIC3          = -L./sdk/modules/nrfx/mdk -T./src/onl/nRF5/magic3/onex.ld
LD_FILES_ADAFRUIT_DONGLE = -L./sdk/modules/nrfx/mdk -T./src/onl/nRF5/adafruit-dongle/onex.ld
LD_FILES_ITSYBITSY       = -L./sdk/modules/nrfx/mdk -T./src/onl/nRF5/itsybitsy/onex.ld
LD_FILES_FEATHER_SENSE   = -L./sdk/modules/nrfx/mdk -T./src/onl/nRF5/feather-sense/onex.ld
LD_FILES_DONGLE          = -L./sdk/modules/nrfx/mdk -T./src/onl/nRF5/dongle/onex.ld

############################################################################################

.S.o:
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(ASSEMBLER_LINE) -o $@ -c $<

.c.o:
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(COMPILE_LINE) -o $@ -c $<

%.hex: %.elf
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex -R .eeprom $< $@

clean:
	find src tests mod-sdk -name '*.o' -o -name '*.d' | xargs rm -f
	rm -f *.bin *.elf *.hex *.map *.out *.uf2
	find . -name onex.ondb | xargs rm -f
	find ./sdk/ -name '*.o' | xargs -r rm
	rm -rf onex-kernel*.??? dfu.zip core oko
	rm -f ,*
	@echo "------------------------------"
	@echo "files not cleaned:"
	@git ls-files --others --exclude-from=.git/info/exclude | xargs -r ls -Fla

#-------------------------------------------------------------------------------
