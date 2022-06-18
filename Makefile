# ------------- Linux

targets:
	@grep '^[a-zA-Z0-9\.#-]\+:' Makefile | grep -v '^\.' | grep -v targets | sed 's/:.*//' | uniq | sed 's/\.elf/.hex/' | sed 's/^/Make clean \&\& Make /'

INCLUDES = \
-I./include \
-I./src/ \
-I./src/onp/ \
-I./tests \

LIB_OBJECTS = \
./src/lib/list.c \
./src/lib/value.c \
./src/onp/onp.c \
./src/onn/onn.c \

UNIX_C_SOURCE_FILES = \
./src/platforms/unix/properties.c \
./src/platforms/unix/serial.c \
./src/platforms/unix/channel-serial.c \
./src/platforms/unix/log.c \
./src/platforms/unix/mem.c \
./src/platforms/unix/time.c \
./src/platforms/unix/random.c \

TESTS_OBJECTS = \
./tests/assert.c \
./tests/test-properties.c \
./tests/test-list.c \
./tests/test-value.c \
./tests/test-onn.c \
./tests/main.c \


X86_FLAGS=-g3 -ggdb
X86_CC_SYMBOLS = -D${TARGET} ${CHANNELS}

CC_FLAGS = -c -std=gnu99 -Werror -Wall -Wextra -Wno-unused-parameter -fno-common -fno-exceptions -ffunction-sections -fdata-sections -fomit-frame-pointer

.c.o:
	$(CC) ${COMPILE_LINE} -o $@ -c $<

x86.library: libonex-kernel-x86.a

	$(AR) rcs $@ $^

libonex-kernel-x86.a: COMPILE_LINE=${X86_FLAGS} ${CC_FLAGS} $(X86_CC_SYMBOLS) ${INCLUDES}
libonex-kernel-x86.a: CC=/usr/bin/gcc
libonex-kernel-x86.a: LD=/usr/bin/gcc
libonex-kernel-x86.a: AR=/usr/bin/ar
libonex-kernel-x86.a: TARGET=TARGET_X86
libonex-kernel-x86.a: CHANNELS=-DONP_CHANNEL_SERIAL
libonex-kernel-x86.a: $(UNIX_C_SOURCE_FILES:.c=.o) ${LIB_OBJECTS:.c=.o}
	$(AR) rcs $@ $^


tests.x86: COMPILE_LINE=${X86_FLAGS} ${CC_FLAGS} $(X86_CC_SYMBOLS) ${INCLUDES}
tests.x86: CC=/usr/bin/gcc
tests.x86: LD=/usr/bin/gcc
tests.x86: TARGET=TARGET_X86
tests.x86: CHANNELS=-DONP_CHANNEL_SERIAL
tests.x86: libonex-kernel-x86.a ${TESTS_OBJECTS:.c=.o}
	$(LD) ${TESTS_OBJECTS:.c=.o} -pthread -L. -lonex-kernel-x86 -o $@

x86.tests: tests.x86
	./tests.x86

x86.valgrind: tests.x86
	valgrind --leak-check=yes --undef-value-errors=no ./tests.x86


#############################:

clean:
	-find src tests -name '*.o' -o -name '*.d' | xargs rm -f
	rm -f ,* core
	find . -name onex.ondb | xargs rm -f
	@echo "------------------------------"

cleanx: clean
	rm -f *.x86

cleanlibs: cleanx
	rm -f libOnex*.a

############################################################################################

