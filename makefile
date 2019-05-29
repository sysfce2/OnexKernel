
############################################################################################
# makefile needs consolidating - it has un-merged lines from a Nordic example at the end
############################################################################################

targets:
	@grep '^[a-zA-Z0-9\.#-]\+:' makefile | grep -v '^\.' | grep -v targets | sed 's/:.*//' | uniq | sed 's/\.elf/.hex/' | sed 's/^/make clean /'

############################################################################################

INCLUDES = \
-I./include \
-I./src/ \
-I./src/onp/ \
-I./tests \


NRF5_INCLUDES = \
-I./include \
-I./src/ \
-I./src/onp/ \
-I./tests \
-I./src/platforms/nrf5/ \


SYS_C_SOURCE_FILES = \
./src/platforms/nrf5/system_nrf51.c \
./src/platforms/nrf5/syscalls.c \


NRF5_C_SOURCE_FILES = \
./src/platforms/nrf5/properties.c \
./src/platforms/nrf5/serial.c \
./src/platforms/nrf5/channel-serial.c \
./src/platforms/nrf5/log.c \
./src/platforms/nrf5/gpio.c \
./src/platforms/nrf5/time.c \
./src/platforms/nrf5/random.c \
./src/platforms/nrf5/radio.c \


UNIX_C_SOURCE_FILES = \
./src/platforms/unix/properties.c \
./src/platforms/unix/serial.c \
./src/platforms/unix/channel-serial.c \
./src/platforms/unix/log.c \
./src/platforms/unix/time.c \
./src/platforms/unix/random.c \


BLINKY_C_SOURCE_FILES = \
./src/ont-examples/microbit/blink.c \


ALL_BLINKY_C_SOURCE_FILES = $(SYS_C_SOURCE_FILES) $(NRF5_C_SOURCE_FILES) $(BLINKY_C_SOURCE_FILES)


NRF51_SYS_S_OBJECTS = \
./src/platforms/nrf5/gcc_startup_nrf51.s \

NRF52_SYS_S_OBJECTS = \
./src/platforms/nrf5/gcc_startup_nrf52.S \


NRF51_SYS_C_OBJECTS = \
./src/platforms/nrf5/system_nrf51.c \
./src/platforms/nrf5/syscalls.c \

NRF52_SYS_C_OBJECTS = \
./src/platforms/nrf5/system_nrf52.c \
./src/platforms/nrf5/syscalls.c \


ASM_SOURCE_FILES = src/platforms/nrf5/gcc_startup_nrf51.s


LIB_OBJECTS = \
./src/lib/list.c \
./src/lib/value.c \
./src/onp/onp.c \
./src/onf/onf.c \


TESTS_OBJECTS = \
./tests/assert.c \
./tests/test-properties.c \
./tests/test-list.c \
./tests/test-value.c \
./tests/test-onf.c \
./tests/main.c \


LIGHT_OBJECTS = \
./src/ont-examples/button-light/light.c \


BUTTON_OBJECTS = \
./src/ont-examples/button-light/button.c \


TAG_OBJECTS = \
./src/ont-examples/tag/tag.c \


############################################################################################

libOnexKernel.a: COMPILE_LINE=${LINUX_FLAGS} ${CC_FLAGS} $(LINUX_CC_SYMBOLS) ${INCLUDES}
libOnexKernel.a: CC=/usr/bin/gcc
libOnexKernel.a: LD=/usr/bin/gcc
libOnexKernel.a: AR=/usr/bin/ar
libOnexKernel.a: TARGET=TARGET_LINUX
libOnexKernel.a: CHANNELS=-DONP_CHANNEL_SERIAL
libOnexKernel.a: $(UNIX_C_SOURCE_FILES:.c=.o) ${LIB_OBJECTS:.c=.o}
	$(AR) rcs $@ $^

libOnexAndroidKernel.a: android/onexkernel/src/main/jni/OnexApp.cpp
	(cd android; ./gradlew build)
	cp android/onexkernel/build/intermediates/ndkBuild/debug/obj/local/armeabi-v7a/libOnexAndroidKernel.a .

tests.linux: COMPILE_LINE=${LINUX_FLAGS} ${CC_FLAGS} $(LINUX_CC_SYMBOLS) ${INCLUDES}
tests.linux: CC=/usr/bin/gcc
tests.linux: LD=/usr/bin/gcc
tests.linux: TARGET=TARGET_LINUX
tests.linux: CHANNELS=-DONP_CHANNEL_SERIAL
tests.linux: libOnexKernel.a ${TESTS_OBJECTS:.c=.o}
	$(LD) ${TESTS_OBJECTS:.c=.o} -L. -lOnexKernel -o $@

tests.microbit.elf: COMPILE_LINE=${M0_CPU} $(M0_CC_FLAGS) $(NRF51_CC_SYMBOLS) $(NRF5_INCLUDES)
tests.microbit.elf: TARGET=TARGET_MICRO_BIT
tests.microbit.elf: $(NRF51_SYS_S_OBJECTS:.s=.o) $(NRF51_SYS_C_OBJECTS:.c=.o) $(NRF5_C_SOURCE_FILES:.c=.o) $(LIB_OBJECTS:.c=.o) $(TESTS_OBJECTS:.c=.o)
	$(LD) $(M0_LD_FLAGS) -L${M0_TEMPLATE_PATH} -T$(LINKER_SCRIPT_16K) -o $@ $^

tests.nano.elf: COMPILE_LINE=${M0_CPU} $(M0_CC_FLAGS) $(NRF51_CC_SYMBOLS) $(NRF5_INCLUDES)
tests.nano.elf: TARGET=TARGET_RBLAB_BLENANO
tests.nano.elf: $(NRF51_SYS_S_OBJECTS:.s=.o) $(NRF51_SYS_C_OBJECTS:.c=.o) $(NRF5_C_SOURCE_FILES:.c=.o) $(LIB_OBJECTS:.c=.o) $(TESTS_OBJECTS:.c=.o)
	$(LD) $(M0_LD_FLAGS) -L${M0_TEMPLATE_PATH} -T$(LINKER_SCRIPT_16K) -o $@ $^

button.microbit.elf: COMPILE_LINE=${M0_CPU} $(M0_CC_FLAGS) $(NRF51_CC_SYMBOLS) $(NRF5_INCLUDES)
button.microbit.elf: TARGET=TARGET_MICRO_BIT
button.microbit.elf: CHANNELS=-DONP_CHANNEL_SERIAL
button.microbit.elf: $(NRF51_SYS_S_OBJECTS:.s=.o) $(NRF51_SYS_C_OBJECTS:.c=.o) $(NRF5_C_SOURCE_FILES:.c=.o) ${LIB_OBJECTS:.c=.o} $(BUTTON_OBJECTS:.c=.o)
	$(LD) $(M0_LD_FLAGS) -L${M0_TEMPLATE_PATH} -T$(LINKER_SCRIPT_16K) -o $@ $^

button.nano.elf: COMPILE_LINE=${M0_CPU} $(M0_CC_FLAGS) $(NRF51_CC_SYMBOLS) $(NRF5_INCLUDES)
button.nano.elf: TARGET=TARGET_RBLAB_BLENANO
button.nano.elf: CHANNELS=-DONP_CHANNEL_SERIAL
button.nano.elf: $(NRF51_SYS_S_OBJECTS:.s=.o) $(NRF51_SYS_C_OBJECTS:.c=.o) $(NRF5_C_SOURCE_FILES:.c=.o) ${LIB_OBJECTS:.c=.o} $(BUTTON_OBJECTS:.c=.o)
	$(LD) $(M0_LD_FLAGS) -L${M0_TEMPLATE_PATH} -T$(LINKER_SCRIPT_16K) -o $@ $^

button.nrf51usb.elf: COMPILE_LINE=${M0_CPU} $(M0_CC_FLAGS) $(NRF51_CC_SYMBOLS) $(NRF5_INCLUDES)
button.nrf51usb.elf: TARGET=TARGET_NRF51_USB
button.nrf51usb.elf: CHANNELS=-DONP_CHANNEL_SERIAL
button.nrf51usb.elf: $(NRF51_SYS_S_OBJECTS:.s=.o) $(NRF51_SYS_C_OBJECTS:.c=.o) $(NRF5_C_SOURCE_FILES:.c=.o) ${LIB_OBJECTS:.c=.o} $(BUTTON_OBJECTS:.c=.o)
	$(LD) $(M0_LD_FLAGS) -L${M0_TEMPLATE_PATH} -T$(LINKER_SCRIPT_16K) -o $@ $^

button.nrf52usb.elf: COMPILE_LINE=${M4_CPU} $(M4_CC_FLAGS) $(NRF52_CC_SYMBOLS) $(NRF5_INCLUDES)
button.nrf52usb.elf: TARGET=TARGET_NRF52_USB
button.nrf52usb.elf: CHANNELS=-DONP_CHANNEL_SERIAL
button.nrf52usb.elf: $(NRF52_SYS_S_OBJECTS:.S=.o) $(NRF52_SYS_C_OBJECTS:.c=.o) $(NRF5_C_SOURCE_FILES:.c=.o) ${LIB_OBJECTS:.c=.o} $(BUTTON_OBJECTS:.c=.o)
	$(LD) $(M4_LD_FLAGS) -L${M4_TEMPLATE_PATH} -T$(LINKER_SCRIPT_256K) -o $@ $^

button.nrf52dk.elf: COMPILE_LINE=${M4_CPU} $(M4_CC_FLAGS) $(NRF52_CC_SYMBOLS) $(NRF5_INCLUDES)
button.nrf52dk.elf: TARGET=TARGET_NRF52_DK
button.nrf52dk.elf: CHANNELS=-DONP_CHANNEL_SERIAL
button.nrf52dk.elf: $(NRF52_SYS_S_OBJECTS:.S=.o) $(NRF52_SYS_C_OBJECTS:.c=.o) $(NRF5_C_SOURCE_FILES:.c=.o) ${LIB_OBJECTS:.c=.o} $(BUTTON_OBJECTS:.c=.o)
	$(LD) $(M4_LD_FLAGS) -L${M4_TEMPLATE_PATH} -T$(LINKER_SCRIPT_256K) -o $@ $^

tag.microbit.elf: COMPILE_LINE=${M0_CPU} $(M0_CC_FLAGS) $(NRF51_CC_SYMBOLS) $(NRF5_INCLUDES)
tag.microbit.elf: TARGET=TARGET_MICRO_BIT
tag.microbit.elf: CHANNELS=-DONP_CHANNEL_SERIAL
tag.microbit.elf: $(NRF51_SYS_S_OBJECTS:.s=.o) $(NRF51_SYS_C_OBJECTS:.c=.o) $(NRF5_C_SOURCE_FILES:.c=.o) ${LIB_OBJECTS:.c=.o} $(TAG_OBJECTS:.c=.o)
	$(LD) $(M0_LD_FLAGS) -L${M0_TEMPLATE_PATH} -T$(LINKER_SCRIPT_16K) -o $@ $^

tag.nano.elf: COMPILE_LINE=${M0_CPU} $(M0_CC_FLAGS) $(NRF51_CC_SYMBOLS) $(NRF5_INCLUDES)
tag.nano.elf: TARGET=TARGET_RBLAB_BLENANO
tag.nano.elf: CHANNELS=-DONP_CHANNEL_SERIAL
tag.nano.elf: $(NRF51_SYS_S_OBJECTS:.s=.o) $(NRF51_SYS_C_OBJECTS:.c=.o) $(NRF5_C_SOURCE_FILES:.c=.o) ${LIB_OBJECTS:.c=.o} $(TAG_OBJECTS:.c=.o)
	$(LD) $(M0_LD_FLAGS) -L${M0_TEMPLATE_PATH} -T$(LINKER_SCRIPT_16K) -o $@ $^

light.microbit.elf: COMPILE_LINE=${M0_CPU} $(M0_CC_FLAGS) $(NRF51_CC_SYMBOLS) $(NRF5_INCLUDES)
light.microbit.elf: TARGET=TARGET_MICRO_BIT
light.microbit.elf: CHANNELS=-DONP_CHANNEL_SERIAL
light.microbit.elf: $(NRF51_SYS_S_OBJECTS:.s=.o) $(NRF51_SYS_C_OBJECTS:.c=.o) $(NRF5_C_SOURCE_FILES:.c=.o) ${LIB_OBJECTS:.c=.o} $(LIGHT_OBJECTS:.c=.o)
	$(LD) $(M0_LD_FLAGS) -L${M0_TEMPLATE_PATH} -T$(LINKER_SCRIPT_16K) -o $@ $^

light.nano.elf: COMPILE_LINE=${M0_CPU} $(M0_CC_FLAGS) $(NRF51_CC_SYMBOLS) $(NRF5_INCLUDES)
light.nano.elf: TARGET=TARGET_RBLAB_BLENANO
light.nano.elf: CHANNELS=-DONP_CHANNEL_SERIAL
light.nano.elf: $(NRF51_SYS_S_OBJECTS:.s=.o) $(NRF51_SYS_C_OBJECTS:.c=.o) $(NRF5_C_SOURCE_FILES:.c=.o) ${LIB_OBJECTS:.c=.o} $(LIGHT_OBJECTS:.c=.o)
	$(LD) $(M0_LD_FLAGS) -L${M0_TEMPLATE_PATH} -T$(LINKER_SCRIPT_16K) -o $@ $^

light.nrf51usb.elf: COMPILE_LINE=${M0_CPU} $(M0_CC_FLAGS) $(NRF51_CC_SYMBOLS) $(NRF5_INCLUDES)
light.nrf51usb.elf: TARGET=TARGET_NRF51_USB
light.nrf51usb.elf: CHANNELS=-DONP_CHANNEL_SERIAL
light.nrf51usb.elf: $(NRF51_SYS_S_OBJECTS:.s=.o) $(NRF51_SYS_C_OBJECTS:.c=.o) $(NRF5_C_SOURCE_FILES:.c=.o) ${LIB_OBJECTS:.c=.o} $(LIGHT_OBJECTS:.c=.o)
	$(LD) $(M0_LD_FLAGS) -L${M0_TEMPLATE_PATH} -T$(LINKER_SCRIPT_16K) -o $@ $^

light.nrf52usb.elf: COMPILE_LINE=${M4_CPU} $(M4_CC_FLAGS) $(NRF52_CC_SYMBOLS) $(NRF5_INCLUDES)
light.nrf52usb.elf: TARGET=TARGET_NRF52_USB
light.nrf52usb.elf: CHANNELS=-DONP_CHANNEL_SERIAL
light.nrf52usb.elf: $(NRF52_SYS_S_OBJECTS:.S=.o) $(NRF52_SYS_C_OBJECTS:.c=.o) $(NRF5_C_SOURCE_FILES:.c=.o) ${LIB_OBJECTS:.c=.o} $(LIGHT_OBJECTS:.c=.o)
	$(LD) $(M4_LD_FLAGS) -L${M4_TEMPLATE_PATH} -T$(LINKER_SCRIPT_256K) -o $@ $^

light.nrf52dk.elf: COMPILE_LINE=${M4_CPU} $(M4_CC_FLAGS) $(NRF52_CC_SYMBOLS) $(NRF5_INCLUDES)
light.nrf52dk.elf: TARGET=TARGET_NRF52_DK
light.nrf52dk.elf: CHANNELS=-DONP_CHANNEL_SERIAL
light.nrf52dk.elf: $(NRF52_SYS_S_OBJECTS:.S=.o) $(NRF52_SYS_C_OBJECTS:.c=.o) $(NRF5_C_SOURCE_FILES:.c=.o) ${LIB_OBJECTS:.c=.o} $(LIGHT_OBJECTS:.c=.o)
	$(LD) $(M4_LD_FLAGS) -L${M4_TEMPLATE_PATH} -T$(LINKER_SCRIPT_256K) -o $@ $^

button.linux: COMPILE_LINE=${LINUX_FLAGS} ${CC_FLAGS} $(LINUX_CC_SYMBOLS) ${INCLUDES}
button.linux: CC=/usr/bin/gcc
button.linux: LD=/usr/bin/gcc
button.linux: TARGET=TARGET_LINUX
button.linux: CHANNELS=-DONP_CHANNEL_SERIAL
button.linux: libOnexKernel.a ${BUTTON_OBJECTS:.c=.o}
	$(LD) -static ${BUTTON_OBJECTS:.c=.o} -L. -lOnexKernel -o $@

light.linux: COMPILE_LINE=${LINUX_FLAGS} ${CC_FLAGS} $(LINUX_CC_SYMBOLS) ${INCLUDES}
light.linux: CC=/usr/bin/gcc
light.linux: LD=/usr/bin/gcc
light.linux: TARGET=TARGET_LINUX
light.linux: CHANNELS=-DONP_CHANNEL_SERIAL
light.linux: libOnexKernel.a ${LIGHT_OBJECTS:.c=.o}
	$(LD) -static ${LIGHT_OBJECTS:.c=.o} -L. -lOnexKernel -o $@

#############################:

linux.tests: tests.linux
	./tests.linux

linux.valgrind: tests.linux
	valgrind --leak-check=yes --undef-value-errors=no ./tests.linux

android.tests: android.library
	adb -d uninstall network.object.onexkernel
	adb -d install android/onexkernel/build/outputs/apk/onexkernel-debug.apk
	adb logcat OnexApp:D *:S

microbit.tests: tests.microbit.hex
	cp $< /media/duncan/MICROBIT/
	sleep 2
	miniterm.py -e --eol CR /dev/ttyACM0 115200

nano.tests: tests.nano.hex
	cp $< /media/duncan/MBED/
	sleep 2
	miniterm.py -e --eol CR /dev/ttyACM0 115200


linux.button: button.linux
	./button.linux

linux.light: light.linux
	./light.linux

microbit.button: button.microbit.hex
	cp $< /media/duncan/MICROBIT/

nano.button: button.nano.hex
	cp $< /media/duncan/MBED/

nrf51usb.button: button.nrf51usb.hex
	cp $< /media/duncan/JLINK/

nrf52usb.button: button.nrf52usb.hex
	nrfutil pkg generate --hw-version 52 --sd-req 0x00 --application-version 1 --application $< app_dfu_package.zip
	nrfutil dfu usb-serial -pkg app_dfu_package.zip -p /dev/ttyACM0

nrf52dk.button: button.nrf52dk.hex
	cp $< /media/duncan/JLINK/

microbit.tag: tag.microbit.hex
	cp $< /media/duncan/MICROBIT/

nano.tag: tag.nano.hex
	cp $< /media/duncan/MBED/

microbit.light: light.microbit.hex
	cp $< /media/duncan/MICROBIT/

nano.light: light.nano.hex
	cp $< /media/duncan/MBED/

nrf51usb.light: light.nrf51usb.hex
	cp $< /media/duncan/JLINK/

nrf52usb.light: light.nrf52usb.hex
	nrfutil pkg generate --hw-version 52 --sd-req 0x00 --application-version 1 --application $< app_dfu_package.zip
	nrfutil dfu usb-serial -pkg app_dfu_package.zip -p /dev/ttyACM0

nrf52dk.light: light.nrf52dk.hex
	cp $< /media/duncan/JLINK/

linux.library: libOnexKernel.a

android.library: libOnexAndroidKernel.a

############################################################################################

BUILD_DIRECTORY = build

LINUX_FLAGS=-g3 -ggdb
LINUX_CC_SYMBOLS = -D${TARGET} ${CHANNELS}

CC_FLAGS = -c -std=gnu99 -Werror -Wall -Wextra -Wno-pointer-sign -Wno-format -Wno-sign-compare -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-write-strings -Wno-old-style-declaration -Wno-strict-aliasing -fno-common -fno-exceptions -ffunction-sections -fdata-sections -fomit-frame-pointer

M4_CPU = -mcpu=cortex-m4 -mthumb -mabi=aapcs
M4_CC_FLAGS = -std=c99 -MP -MD -Werror -Wall -Wextra -Wno-pointer-sign -Wno-format -Wno-sign-compare -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-write-strings -Wno-old-style-declaration -Wno-strict-aliasing -fno-common -fshort-enums -fno-builtin -fno-exceptions -ffunction-sections -fdata-sections -fomit-frame-pointer -O3 -g3 -mfloat-abi=hard -mfpu=fpv4-sp-d16
NRF52_CC_SYMBOLS = -DNRF5 -DNRF52 -D${TARGET} ${CHANNELS} -DTARGET_MCU_NRF52832 -DFLOAT_ABI_HARD -DNRF52840_XXAA -D__HEAP_SIZE=8192 -D__STACK_SIZE=8192

M0_CPU = -mcpu=cortex-m0 -mthumb
M0_CC_FLAGS = -std=gnu99 -Werror -Wall -Wextra -Wno-pointer-sign -Wno-format -Wno-sign-compare -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-write-strings -Wno-old-style-declaration -Wno-strict-aliasing -fno-common -fno-exceptions -ffunction-sections -fdata-sections -fomit-frame-pointer -O0
NRF51_CC_SYMBOLS = -DNRF5 -DNRF51 -D${TARGET} ${CHANNELS} -DTARGET_MCU_NRF51822

CFLAGS  = -DNRF51 -DTARGET_MICRO_BIT -DBSP_DEFINES_ONLY -mcpu=cortex-m0 -mthumb -mabi=aapcs --std=gnu99 -Wall -Werror -O3 -g3 -mfloat-abi=soft -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin --short-enums

LDFLAGS = -Xlinker -Map=$(BUILD_DIRECTORY)/$(OUTPUT_FILENAME).map -mthumb -mabi=aapcs -L $(M0_TEMPLATE_PATH) -T$(LINKER_SCRIPT_16K) -mcpu=cortex-m0 -Wl,--gc-sections --specs=nano.specs -lc -lnosys

ASMFLAGS = -x assembler-with-cpp -DNRF51 -DTARGET_MICRO_BIT -DBSP_DEFINES_ONLY

M0_LD_FLAGS = $(M0_CPU) -O0 --specs=nano.specs

M4_LD_FLAGS = $(M4_CPU) -O3 -g3 -mthumb -mabi=aapcs -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,--gc-sections --specs=nano.specs

M0_TEMPLATE_PATH := ./src/platforms/nrf5/

M4_TEMPLATE_PATH := ./src/platforms/nrf5/

LINKER_SCRIPT_16K=./src/platforms/nrf5/memory-16K-no-sd.ld

LINKER_SCRIPT_256K=./src/platforms/nrf5/memory-256K-no-sd.ld

remduplicates = $(strip $(if $1,$(firstword $1) $(call remduplicates,$(filter-out $(firstword $1),$1))))

############################################################################################

GNU_INSTALL_ROOT := /home/duncan/gcc-arm
GCC_BIN=$(GNU_INSTALL_ROOT)/bin

CC      = $(GCC_BIN)/arm-none-eabi-gcc
LD      = $(GCC_BIN)/arm-none-eabi-gcc
OBJCOPY = $(GCC_BIN)/arm-none-eabi-objcopy
SIZE    = $(GCC_BIN)/arm-none-eabi-size

############################################################################################

.s.o:
	$(CC) $(M0_CPU) -c -x assembler-with-cpp -D__STACK_SIZE=4096 -D__HEAP_SIZE=8192 -o $@ $<

.c.o:
	$(CC) ${COMPILE_LINE} -o $@ -c $<

%.hex: %.elf
	$(SIZE) "$<"
	$(OBJCOPY) -O ihex $< $@


############################################################################################

BLINKY_C_SOURCE_FILE_NAMES = $(notdir $(ALL_BLINKY_C_SOURCE_FILES))
BLINKY_C_PATHS = $(call remduplicates, $(dir $(ALL_BLINKY_C_SOURCE_FILES) ) )
BLINKY_C_OBJECTS = $(addprefix $(BUILD_DIRECTORY)/, $(BLINKY_C_SOURCE_FILE_NAMES:.c=.o) )

ASM_SOURCE_FILE_NAMES = $(notdir $(ASM_SOURCE_FILES))
ASM_PATHS = $(call remduplicates, $(dir $(ASM_SOURCE_FILES) ))
ASM_OBJECTS = $(addprefix $(BUILD_DIRECTORY)/, $(ASM_SOURCE_FILE_NAMES:.s=.o) )

vpath %.c $(BLINKY_C_PATHS)
vpath %.s $(ASM_PATHS)

BLINKY_OBJECTS = $(BLINKY_C_OBJECTS) $(ASM_OBJECTS)

############################################################################################

blinky.microbit: OUTPUT_FILENAME := blinky.microbit
blinky.microbit: $(BUILD_DIRECTORY) $(BLINKY_OBJECTS)
	$(CC) $(LDFLAGS) $(BLINKY_OBJECTS) $(LIBS) -lm -o $(BUILD_DIRECTORY)/$(OUTPUT_FILENAME).out

microbit.blinky: OUTPUT_FILENAME := blinky.microbit
microbit.blinky: blinky.microbit
	$(OBJCOPY) -O binary $(BUILD_DIRECTORY)/$(OUTPUT_FILENAME).out $(BUILD_DIRECTORY)/$(OUTPUT_FILENAME).bin
	$(OBJCOPY) -O ihex $(BUILD_DIRECTORY)/$(OUTPUT_FILENAME).out $(BUILD_DIRECTORY)/$(OUTPUT_FILENAME).hex
	$(SIZE) $(BUILD_DIRECTORY)/$(OUTPUT_FILENAME).out
	cp build/$(OUTPUT_FILENAME).hex /media/duncan/MICROBIT/
	sleep 2
	miniterm.py -e --eol CR /dev/ttyACM0 9600

############################################################################################

$(BUILD_DIRECTORY):
	mkdir $@

$(BUILD_DIRECTORY)/%.o: %.c
	$(CC) $(CFLAGS) $(NRF5_INCLUDES) -c -o $@ $<

$(BUILD_DIRECTORY)/%.o: %.s
	$(CC) $(ASMFLAGS) $(NRF5_INCLUDES) -c -o $@ $<

############################################################################################

#############################:

clean:
	-find src tests -name '*.o' -o -name '*.d' | xargs rm -f
	rm -f *.bin *.elf
	rm -f ,* core
	rm -rf $(BUILD_DIRECTORY)/*.o
	find . -name onex.ondb | xargs rm -f
	@echo "------------------------------"

cleanx: clean
	rm -f *.linux *.hex
	rm -rf $(BUILD_DIRECTORY)
	rm -rf android/build android/*/build android/*/.externalNativeBuild/ android/.gradle/*/*

cleanlibs: cleanx
	rm -f libOnex*.a

############################################################################################

