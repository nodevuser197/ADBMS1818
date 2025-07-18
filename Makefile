# ADBMS1818 / LTC6813 Library — GCC compile check
#
# Compiles the library and HAL wrapper against minimal STM32 stubs so the
# CI can verify syntax & types without a real ARM toolchain or SDK.
#
# ci/stubs/ provides stand-in headers for:
#   stm32g4xx_hal.h     – GPIO / delay / tick primitives
#   stm32g4xx_hal_spi.h – SPI handle & transfer primitives
#
# prog/src/application.c is excluded: it references symbols that live in
# the host STM32 project (parseMAX11616Data, MAX11616_ADDR, …) and are not
# part of this library repository.

CC     = gcc
CFLAGS = -Wall -Wextra -Wno-unused-parameter -std=c11 -c \
         -Ici/stubs -Ilib/inc -Iprog/inc

SRCS = lib/src/commHelper.c  \
       lib/src/genericType.c  \
       lib/src/parseCreate.c  \
       prog/src/wrapper.c

BUILD = build
OBJS  = $(patsubst %.c,$(BUILD)/%.o,$(SRCS))

.PHONY: all clean

all: $(OBJS)
	@echo "Build OK"

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD)
