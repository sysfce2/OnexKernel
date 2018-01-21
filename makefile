
############################################################################################
# makefile needs consolidating - it has un-merged lines from a Nordic example at the end
############################################################################################

targets:
	@grep '^[a-zA-Z0-9\.#-]\+:' makefile | grep -v '^\.' | grep -v targets | sed 's/:.*//' | uniq | sed 's/\.elf/.hex/' | sed 's/^/make clean /'

############################################################################################

INCLUDES = \
-I./include \
-I./src/ \
-I./src/lib/ \
-I./src/onp/ \
-I./tests \


NRF51_INCLUDES = \
-I./include \
-I./src/ \
-I./src/lib/ \
-I./src/onp/ \
-I./tests \
-I./src/platforms/nrf51/ \


SYS_C_SOURCE_FILES = \
./src/platforms/nrf51/system_nrf51.c \
./src/platforms/nrf51/syscalls.c \


NRF51_C_SOURCE_FILES = \
./src/platforms/nrf51/serial.c \
./src/platforms/nrf51/channel-serial.c \
./src/platforms/nrf51/log.c \
./src/platforms/nrf51/gpio.c \
./src/platforms/nrf51/time.c \
./src/platforms/nrf51/random.c \


UNIX_C_SOURCE_FILES = \
./src/platforms/unix/serial.c \
./src/platforms/unix/channel-serial.c \
./src/platforms/unix/log.c \
./src/platforms/unix/time.c \
./src/platforms/unix/random.c \


BLINKY_C_SOURCE_FILES = \
./src/ont-examples/microbit/blink.c \


ALL_BLINKY_C_SOURCE_FILES = $(SYS_C_SOURCE_FILES) $(NRF51_C_SOURCE_FILES) $(BLINKY_C_SOURCE_FILES)


NRF51_SYS_S_OBJECTS = \
./src/platforms/nrf51/gcc_startup_nrf51.s \


NRF51_SYS_C_OBJECTS = \
./src/platforms/nrf51/system_nrf51.c \
./src/platforms/nrf51/syscalls.c \


ASM_SOURCE_FILES = src/platforms/nrf51/gcc_startup_nrf51.s


LIB_OBJECTS = \
./src/lib/properties.c \
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
	$(LD) -static ${TESTS_OBJECTS:.c=.o} -L. -lOnexKernel -o $@

tests.microbit.elf: COMPILE_LINE=${M0_CPU} $(M0_CC_FLAGS) $(NRF51_CC_SYMBOLS) $(NRF51_INCLUDES)
tests.microbit.elf: TARGET=TARGET_MICRO_BIT
tests.microbit.elf: $(NRF51_SYS_S_OBJECTS:.s=.o) $(NRF51_SYS_C_OBJECTS:.c=.o) $(NRF51_C_SOURCE_FILES:.c=.o) $(LIB_OBJECTS:.c=.o) $(TESTS_OBJECTS:.c=.o)
	$(LD) $(M0_LD_FLAGS) -L${M0_TEMPLATE_PATH} -T$(LINKER_SCRIPT_16K) -o $@ $^

button.microbit.elf: COMPILE_LINE=${M0_CPU} $(M0_CC_FLAGS) $(NRF51_CC_SYMBOLS) $(NRF51_INCLUDES)
button.microbit.elf: TARGET=TARGET_MICRO_BIT
button.microbit.elf: CHANNELS=-DONP_CHANNEL_SERIAL
button.microbit.elf: $(NRF51_SYS_S_OBJECTS:.s=.o) $(NRF51_SYS_C_OBJECTS:.c=.o) $(NRF51_C_SOURCE_FILES:.c=.o) ${LIB_OBJECTS:.c=.o} $(BUTTON_OBJECTS:.c=.o)
	$(LD) $(M0_LD_FLAGS) -L${M0_TEMPLATE_PATH} -T$(LINKER_SCRIPT_16K) -o $@ $^

light.microbit.elf: COMPILE_LINE=${M0_CPU} $(M0_CC_FLAGS) $(NRF51_CC_SYMBOLS) $(NRF51_INCLUDES)
light.microbit.elf: TARGET=TARGET_MICRO_BIT
light.microbit.elf: CHANNELS=-DONP_CHANNEL_SERIAL
light.microbit.elf: $(NRF51_SYS_S_OBJECTS:.s=.o) $(NRF51_SYS_C_OBJECTS:.c=.o) $(NRF51_C_SOURCE_FILES:.c=.o) ${LIB_OBJECTS:.c=.o} $(LIGHT_OBJECTS:.c=.o)
	$(LD) $(M0_LD_FLAGS) -L${M0_TEMPLATE_PATH} -T$(LINKER_SCRIPT_16K) -o $@ $^

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

android.tests: android.library
	adb -d uninstall network.object.onexkernel
	adb -d install android/onexkernel/build/outputs/apk/onexkernel-debug.apk
	adb logcat OnexApp:D *:S

microbit.tests: tests.microbit.hex
	cp $< /media/duncan/MICROBIT/
	sleep 2
	miniterm.py -e --eol CR /dev/ttyACM0 115200


linux.button: button.linux
	./button.linux

linux.light: light.linux
	./light.linux

microbit.button: button.microbit.hex
	cp $< /media/duncan/MICROBIT/

microbit.light: light.microbit.hex
	cp $< /media/duncan/MICROBIT/

linux.library: libOnexKernel.a

android.library: libOnexAndroidKernel.a

############################################################################################

BUILD_DIRECTORY = build

LINUX_FLAGS=-g3 -ggdb
LINUX_CC_SYMBOLS = -D${TARGET} ${CHANNELS}

CC_FLAGS = -c    -std=gnu99              -fno-common                    -Wno-pointer-sign -Wno-format -Wno-sign-compare -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-write-strings -Wno-old-style-declaration -Wall -Wextra -Wno-strict-aliasing -fno-exceptions -ffunction-sections -fdata-sections -fomit-frame-pointer

M0_CPU = -mcpu=cortex-m0 -mthumb
M0_CC_FLAGS = -std=gnu99 -Werror -Wall -Wextra -Wno-pointer-sign -Wno-format -Wno-sign-compare -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-write-strings -Wno-old-style-declaration -Wno-strict-aliasing -fno-common -fno-exceptions -ffunction-sections -fdata-sections -fomit-frame-pointer -O0
NRF51_CC_SYMBOLS = -DNRF51 -D${TARGET} ${CHANNELS} -DTARGET_MCU_NRF51822

CFLAGS  = -DNRF51 -DTARGET_MICRO_BIT -DBSP_DEFINES_ONLY -mcpu=cortex-m0 -mthumb -mabi=aapcs --std=gnu99 -Wall -Werror -O3 -g3 -mfloat-abi=soft -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin --short-enums

LDFLAGS = -Xlinker -Map=$(BUILD_DIRECTORY)/$(OUTPUT_FILENAME).map -mthumb -mabi=aapcs -L $(M0_TEMPLATE_PATH) -T$(LINKER_SCRIPT_16K) -mcpu=cortex-m0 -Wl,--gc-sections --specs=nano.specs -lc -lnosys

ASMFLAGS = -x assembler-with-cpp -DNRF51 -DTARGET_MICRO_BIT -DBSP_DEFINES_ONLY

M0_LD_FLAGS = $(M0_CPU) -O0 --specs=nano.specs

M0_TEMPLATE_PATH := ./src/platforms/nrf51/

LINKER_SCRIPT_16K=./src/platforms/nrf51/memory-16K-no-sd.ld

remduplicates = $(strip $(if $1,$(firstword $1) $(call remduplicates,$(filter-out $(firstword $1),$1))))

############################################################################################

GNU_INSTALL_ROOT := /home/duncan/arm-gcc
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

############################################################################################

$(BUILD_DIRECTORY):
	mkdir $@

$(BUILD_DIRECTORY)/%.o: %.c
	$(CC) $(CFLAGS) $(NRF51_INCLUDES) -c -o $@ $<

$(BUILD_DIRECTORY)/%.o: %.s
	$(CC) $(ASMFLAGS) $(NRF51_INCLUDES) -c -o $@ $<

############################################################################################

#############################:

clean:
	-find src tests -name '*.o' -o -name '*.d' | xargs rm -f
	rm -f *.bin *.elf
	rm -f ,* core
	rm -rf $(BUILD_DIRECTORY)/*.o
	@echo "------------------------------"

cleanx: clean
	rm -f *.linux *.hex libOnex*.a
	rm -rf $(BUILD_DIRECTORY)
	rm -rf android/build android/*/build android/*/.externalNativeBuild/ android/.gradle/[0-9]*/task*/
	rm -rf CMakeCache.txt CMakeFiles */CMakeFiles Makefile */Makefile cmake_install.cmake */cmake_install.cmake

############################################################################################

