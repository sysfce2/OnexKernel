#-------------------------------------------------------------------------------
# Ubuntu + Ubuntu Touch Makefile

targets:
	@grep '^[a-zA-Z0-9\.#-]\+:' Makefile | grep -v '^\.' | grep -v targets | sed 's/:.*//' | uniq | sed 's/\.elf/.hex/' | sed 's/^/Make clean \&\& Make -j /'

MAKEFLAGS += --no-builtin-rules

#-------------------------------------------------------------------------------

INCLUDES = \
-I./include \
-I./include/vulkan \
-I./src/ \
-I./src/lib \
-I./src/onl \
-I./src/onl/desktop/vulkan \
-I./src/onn/ \
-I./src/onp/ \
-I./tests \
-I./tests/ont-examples/vulkan \

#-------------------------------------------------------------------------------

TESTS_SOURCES = \
./tests/test-properties.c \
./tests/test-list.c \
./tests/test-value.c \
./tests/test-onn.c \
./tests/main.c \

#-------------------------------------------------------------------------------

SHADERS = \
  tests/ont-examples/vulkan/onx.vert.spv \
  tests/ont-examples/vulkan/onx.frag.spv \

#-------------------------------------------------------------------------------

VULKAN_SOURCES = \
./tests/ont-examples/vulkan/onx-vk-rd.c \
./tests/ont-examples/vulkan/one-panel.c \


LIB_SOURCES = \
./src/lib/lib.c \
./src/lib/list.c \
./src/lib/value.c \
./src/lib/tests.c \
./src/lib/properties.c \
./src/onl/desktop/vulkan-xcb.c \
./src/onl/desktop/vulkan/vk.c \
./src/onl/desktop/vulkan/vk-rg.c \
./src/onn/onn.c \
./src/onp/onp.c \


UNIX_SOURCES = \
./src/onl/unix/serial.c \
./src/onl/unix/channel-serial.c \
./src/onl/unix/log.c \
./src/onl/unix/mem.c \
./src/onl/unix/time.c \
./src/onl/unix/random.c \
./src/onl/unix/persistence.c \

#-------------------------------------------------------------------------------

arm.lib: libonex-kernel-arm.a

x86.lib: libonex-kernel-x86.a

libonex-kernel-arm.a: COMPILE_LINE=$(ARM_FLAGS) $(CC_FLAGS) $(ARM_CC_SYMBOLS) $(INCLUDES)
libonex-kernel-arm.a: CC=/home/duncan/x-tools/aarch64-unknown-linux-gnu/bin/aarch64-unknown-linux-gnu-gcc
libonex-kernel-arm.a: LD=/home/duncan/x-tools/aarch64-unknown-linux-gnu/bin/aarch64-unknown-linux-gnu-gcc
libonex-kernel-arm.a: AR=/home/duncan/x-tools/aarch64-unknown-linux-gnu/bin/aarch64-unknown-linux-gnu-ar
libonex-kernel-arm.a: TARGET=TARGET_ARM
libonex-kernel-arm.a: CHANNELS=-DONP_CHANNEL_SERIAL
libonex-kernel-arm.a: $(UNIX_SOURCES:.c=.o) $(LIB_SOURCES:.c=.o)
	$(AR) rcs $@ $^

libonex-kernel-x86.a: COMPILE_LINE=$(X86_FLAGS) $(CC_FLAGS) $(X86_CC_SYMBOLS) $(INCLUDES)
libonex-kernel-x86.a: CC=/usr/bin/gcc
libonex-kernel-x86.a: LD=/usr/bin/gcc
libonex-kernel-x86.a: AR=/usr/bin/ar
libonex-kernel-x86.a: TARGET=TARGET_X86
libonex-kernel-x86.a: CHANNELS=-DONP_CHANNEL_SERIAL -DONP_DEBUG -DONP_OVER_SERIAL
libonex-kernel-x86.a: $(UNIX_SOURCES:.c=.o) $(LIB_SOURCES:.c=.o)
	$(AR) rcs $@ $^

tests.arm: COMPILE_LINE=$(ARM_FLAGS) $(CC_FLAGS) $(ARM_CC_SYMBOLS) $(INCLUDES)
tests.arm: CC=/home/duncan/x-tools/aarch64-unknown-linux-gnu/bin/aarch64-unknown-linux-gnu-gcc
tests.arm: LD=/home/duncan/x-tools/aarch64-unknown-linux-gnu/bin/aarch64-unknown-linux-gnu-gcc
tests.arm: TARGET=TARGET_ARM
tests.arm: CHANNELS=-DONP_CHANNEL_SERIAL
tests.arm: libonex-kernel-arm.a $(TESTS_SOURCES:.c=.o)
	$(LD) $(TESTS_SOURCES:.c=.o) -pthread -L. -lonex-kernel-arm -o $@

tests.x86: COMPILE_LINE=$(X86_FLAGS) $(CC_FLAGS) $(X86_CC_SYMBOLS) $(INCLUDES)
tests.x86: CC=/usr/bin/gcc
tests.x86: LD=/usr/bin/gcc
tests.x86: TARGET=TARGET_X86
tests.x86: CHANNELS=-DONP_CHANNEL_SERIAL
tests.x86: libonex-kernel-x86.a $(TESTS_SOURCES:.c=.o)
	$(LD) $(TESTS_SOURCES:.c=.o) -pthread -L. -lonex-kernel-x86 -o $@

vulkan.x86: COMPILE_LINE=$(X86_FLAGS) $(CC_FLAGS) $(X86_CC_SYMBOLS) $(INCLUDES)
vulkan.x86: CC=/usr/bin/gcc
vulkan.x86: LD=/usr/bin/gcc
vulkan.x86: TARGET=TARGET_X86
vulkan.x86: libonex-kernel-x86.a $(VULKAN_SOURCES:.c=.o) ${SHADERS:.spv=.o}
	@echo ================
	@echo $@ '<=' $(VULKAN_SOURCES:.c=.o) ${SHADERS:.spv=.o}
	@echo -----
	$(LD) $(VULKAN_SOURCES:.c=.o) ${SHADERS:.spv=.o} -pthread -L. -lonex-kernel-x86 -lvulkan -lxcb -lfreetype -lm -o $@


arm.tests: tests.arm
	mkdir -p ok
	cp -a ./tests.arm ok

x86.tests: tests.x86
	./tests.x86

x86.valgrind: tests.x86
	valgrind --leak-check=yes --undef-value-errors=no ./tests.x86

#-------------------------------------------------------------------------------

ARM_FLAGS=-ggdb3 -fPIC
ARM_CC_SYMBOLS = -D$(TARGET) $(CHANNELS)

X86_FLAGS=-ggdb3 -O2
X86_CC_SYMBOLS = -D$(TARGET) $(CHANNELS) -DVK_USE_PLATFORM_XCB_KHR

CC_FLAGS = -std=gnu17 -Wall -Werror -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -fno-strict-aliasing -fno-builtin-memcmp -Wimplicit-fallthrough=0 -fvisibility=hidden -Wno-unused-function -Wno-incompatible-pointer-types -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-result -Wno-switch


%.o: %.c
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	$(CC) $(COMPILE_LINE) -o $@ -c $<

%.o: %.cpp
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	$(CC) $(COMPILE_LINE) -o $@ -c $<

%.vert.spv: %.vert
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	glslc $< -o $@

%.frag.spv: %.frag
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	glslc $< -o $@

%.vert.c: %.vert.spv
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	xxd -i $< > $@

%.frag.c: %.frag.spv
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	xxd -i $< > $@

copy:
	rsync -ruav --stats --progress --delete ok/ phablet@dorold:ok

clean:
	find src tests -name '*.o' -o -name '*.d' | xargs rm -f
	rm -f core
	rm -rf *.arm *.x86 ok
	rm -rf ${TARGETS} tests/ont-examples/vulkan/*.{inc,spv,vert.c,frag.c}
	find . -name onex.ondb | xargs -r rm
	@echo "------------------------------"
	@echo "files not cleaned:"
	@git ls-files --others --exclude-from=.git/info/exclude | xargs -r ls -Fla

#-------------------------------------------------------------------------------
