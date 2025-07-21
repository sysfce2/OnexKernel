#-------------------------------------------------------------------------------
# Ubuntu + Ubuntu Touch Makefile

targets:
	@grep '^[a-zA-Z0-9\.#-]\+:' Makefile | grep -v '^\.' | grep -v targets | sed 's/:.*//' | uniq | sed 's/\.elf/.hex/' | sed 's/^/Make clean \&\& Make -j /'

MAKEFLAGS += --no-builtin-rules

#-------------------------------------------------------------------------------

INCLUDES = \
-I/usr/include/libevdev-1.0 \
-I./include \
-I./include/vulkan \
-I./libraries \
-I./src/ \
-I./src/lib \
-I./src/onl \
-I./src/onl/vulkan \
-I./src/onn/ \
-I./src/onp/ \

#-------------------------------------------------------------------------------

LIB_SOURCES = \
./src/lib/config.c \
./src/lib/lib.c \
./src/lib/show_bytes_n_chars.c \
./src/lib/colours.c \
./src/lib/chunkbuf.c \
./src/lib/database.c \
./src/lib/list.c \
./src/lib/value.c \
./src/lib/tests.c \
./src/lib/properties.c \


ONL_UNIX_SOURCES = \
./src/onl/unix/log.c \
./src/onl/unix/mem.c \
./src/onl/unix/time.c \
./src/onl/unix/random.c \
./src/onl/unix/persistence.c \
./src/onl/unix/serial.c \
./src/onl/unix/radio.c \
./src/onl/unix/ipv6.c \


ONL_VULKAN_XCB_SOURCES = \
./src/onl/drivers/onl.c \
./src/onl/drivers/libevdev/libevdev.c \
./src/onl/drivers/viture/viture_imu.c \
./src/onl/vulkan/vulkan-xcb.c \
./src/onl/vulkan/vk.c \
./src/onl/vulkan/vk-rendering.c \


ONL_VULKAN_DRM_SOURCES = \
./src/onl/drivers/onl.c \
./src/onl/drivers/libevdev/libevdev.c \
./src/onl/drivers/viture/viture_imu.c \
./src/onl/vulkan/vulkan-drm.c \
./src/onl/vulkan/vk.c \
./src/onl/vulkan/vk-rendering.c \


ONN_ONP_SOURCES = \
./src/onn/onn.c \
./src/onp/behaviours.c \
./src/onp/onp.c \


LIBONEX_XCB_SOURCES = \
 $(LIB_SOURCES) \
 $(ONL_UNIX_SOURCES) \
 $(ONL_VULKAN_XCB_SOURCES) \
 $(ONN_ONP_SOURCES) \


LIBONEX_DRM_SOURCES = \
 $(LIB_SOURCES) \
 $(ONL_UNIX_SOURCES) \
 $(ONL_VULKAN_DRM_SOURCES) \
 $(ONN_ONP_SOURCES) \

#-------------------------------------------------------------------------------

TESTS_SOURCES = \
./tests/test-properties.c \
./tests/test-list.c \
./tests/test-value.c \
./tests/test-database.c \
./tests/test-onn.c \
./tests/main.c \


PCR_BUTTON_TESTS_SOURCES = \
./tests/main-pcr-button.c \


PCR_TESTS_SOURCES = \
./tests/main-pcr.c \


PCR_LIGHT_TESTS_SOURCES = \
./tests/main-pcr-light.c \

#-------------------------------------------------------------------------------

ONT_VULKAN_SOURCES = \
./tests/ont-examples/vulkan/ont-vk.c \
./tests/ont-examples/vulkan/one-panel.c \


ONT_VULKAN_SHADERS = \
./tests/ont-examples/vulkan/onx.vert.spv \
./tests/ont-examples/vulkan/onx.frag.spv \


#-------------------------------------------------------------------------------

libonex-kernel-xcb.a: COMPILE_LINE=$(X86_FLAGS) $(CC_FLAGS) $(XCB_CC_SYMBOLS) $(INCLUDES)
libonex-kernel-xcb.a: CC=/usr/bin/gcc
libonex-kernel-xcb.a: LD=/usr/bin/gcc
libonex-kernel-xcb.a: AR=/usr/bin/ar
libonex-kernel-xcb.a: TARGET=TARGET_X86
libonex-kernel-xcb.a: $(LIBONEX_XCB_SOURCES:.c=.o)
	rm -f $@
	$(AR) rcs $@ $^

libonex-kernel-drm.a: COMPILE_LINE=$(X86_FLAGS) $(CC_FLAGS) $(DRM_CC_SYMBOLS) $(INCLUDES)
libonex-kernel-drm.a: CC=/usr/bin/gcc
libonex-kernel-drm.a: LD=/usr/bin/gcc
libonex-kernel-drm.a: AR=/usr/bin/ar
libonex-kernel-drm.a: TARGET=TARGET_X86
libonex-kernel-drm.a: $(LIBONEX_DRM_SOURCES:.c=.o)
	rm -f $@
	$(AR) rcs $@ $^

#-------------------------------------------------------------------------------

test-ok: COMPILE_LINE=$(X86_FLAGS) $(CC_FLAGS) $(XCB_CC_SYMBOLS) $(INCLUDES)
test-ok: CC=/usr/bin/gcc
test-ok: LD=/usr/bin/gcc
test-ok: TARGET=TARGET_X86
test-ok: libonex-kernel-xcb.a $(TESTS_SOURCES:.c=.o)
	$(LD) $(TESTS_SOURCES:.c=.o) -pthread -L. -lonex-kernel-xcb -o $@

run.tests: test-ok
	./test-ok

run.valgrind: test-ok
	valgrind --leak-check=yes --undef-value-errors=no ./test-ok

#-------------------------------------------------------------------------------

test-pcr-button: COMPILE_LINE=$(X86_FLAGS) $(CC_FLAGS) $(XCB_CC_SYMBOLS) $(INCLUDES)
test-pcr-button: CC=/usr/bin/gcc
test-pcr-button: LD=/usr/bin/gcc
test-pcr-button: TARGET=TARGET_X86
test-pcr-button: libonex-kernel-xcb.a $(PCR_BUTTON_TESTS_SOURCES:.c=.o)
	$(LD) $(PCR_BUTTON_TESTS_SOURCES:.c=.o) -pthread -L. -lonex-kernel-xcb -o $@

test-pcr: COMPILE_LINE=$(X86_FLAGS) $(CC_FLAGS) $(XCB_CC_SYMBOLS) $(INCLUDES)
test-pcr: CC=/usr/bin/gcc
test-pcr: LD=/usr/bin/gcc
test-pcr: TARGET=TARGET_X86
test-pcr: libonex-kernel-xcb.a $(PCR_TESTS_SOURCES:.c=.o)
	$(LD) $(PCR_TESTS_SOURCES:.c=.o) -pthread -L. -lonex-kernel-xcb -o $@

test-pcr-light: COMPILE_LINE=$(X86_FLAGS) $(CC_FLAGS) $(XCB_CC_SYMBOLS) $(INCLUDES)
test-pcr-light: CC=/usr/bin/gcc
test-pcr-light: LD=/usr/bin/gcc
test-pcr-light: TARGET=TARGET_X86
test-pcr-light: libonex-kernel-xcb.a $(PCR_LIGHT_TESTS_SOURCES:.c=.o)
	$(LD) $(PCR_LIGHT_TESTS_SOURCES:.c=.o) -pthread -L. -lonex-kernel-xcb -o $@

run.pcr-button: test-pcr-button
	./test-pcr-button /dev/nordic_dongle

run.pcr: test-pcr
	./test-pcr ff12::1234 ff12::4321

run.pcr-light: test-pcr-light
	./test-pcr-light ff12::1234

#-------------------------------------------------------------------------------

vulkan.xcb: COMPILE_LINE=$(X86_FLAGS) $(CC_FLAGS) $(XCB_CC_SYMBOLS) $(INCLUDES)
vulkan.xcb: CC=/usr/bin/gcc
vulkan.xcb: LD=/usr/bin/gcc
vulkan.xcb: TARGET=TARGET_X86
vulkan.xcb: libonex-kernel-xcb.a $(ONT_VULKAN_SOURCES:.c=.o) ${ONT_VULKAN_SHADERS:.spv=.o}
	@echo ================
	@echo $@ '<=' $(ONT_VULKAN_SOURCES:.c=.o) ${ONT_VULKAN_SHADERS:.spv=.o}
	@echo -----
	$(LD) $(ONT_VULKAN_SOURCES:.c=.o) ${ONT_VULKAN_SHADERS:.spv=.o} ${XCB_LIBS} -o $@

vulkan.drm: COMPILE_LINE=$(X86_FLAGS) $(CC_FLAGS) $(DRM_CC_SYMBOLS) $(INCLUDES)
vulkan.drm: CC=/usr/bin/gcc
vulkan.drm: LD=/usr/bin/gcc
vulkan.drm: TARGET=TARGET_X86
vulkan.drm: libonex-kernel-drm.a $(ONT_VULKAN_SOURCES:.c=.o) ${ONT_VULKAN_SHADERS:.spv=.o}
	@echo ================
	@echo $@ '<=' $(ONT_VULKAN_SOURCES:.c=.o) ${ONT_VULKAN_SHADERS:.spv=.o}
	@echo -----
	$(LD) $(ONT_VULKAN_SOURCES:.c=.o) ${ONT_VULKAN_SHADERS:.spv=.o} ${DRM_LIBS} -o $@

#-------------------------------------------------------------------------------

XCB_LIBS=-pthread -Wl,-rpath,./libraries -L. -L./libraries -lonex-kernel-xcb -lviture_one_sdk -lvulkan -lxcb -levdev -lfreetype -lm
DRM_LIBS=-pthread -Wl,-rpath,./libraries -L. -L./libraries -lonex-kernel-drm -lviture_one_sdk -lvulkan       -levdev -lfreetype -lm

X86_FLAGS=-ggdb3 -O2
XCB_CC_SYMBOLS = -D$(TARGET) $(CHANNELS) -DVK_USE_PLATFORM_XCB_KHR
DRM_CC_SYMBOLS = -D$(TARGET) $(CHANNELS) -DVK_USE_PLATFORM_DISPLAY_KHR

CC_FLAGS = -std=gnu17 -Wall -Werror -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -fno-strict-aliasing -fno-builtin-memcmp -Wimplicit-fallthrough=0 -fvisibility=hidden -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-result -Wno-switch

# remove these for thorough linting:
#CC_FLAGS += -Wno-incompatible-pointer-types
#CC_FLAGS += -Wno-sign-compare
CC_FLAGS += -Wno-discarded-qualifiers
CC_FLAGS += -Wno-array-bounds
CC_FLAGS += -Wno-char-subscripts
CC_FLAGS += -Wno-misleading-indentation

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
	rsync -ruav --stats --progress --delete ok/ duncan@arb:ok

clean:
	find src tests -name '*.o' -o -name '*.d' | xargs -r rm
	rm -f core*
	rm -rf test-ok test-pcr* *.xcb *.drm ok
	rm -rf ${TARGETS} tests/ont-examples/vulkan/*.{inc,spv,vert.c,frag.c}
	find . -name '*.ondb' | xargs -r rm
	@echo "------------------------------"
	@echo "files not cleaned:"
	@git ls-files --others --exclude-from=.git/info/exclude | xargs -r ls -Fla

#-------------------------------------------------------------------------------
