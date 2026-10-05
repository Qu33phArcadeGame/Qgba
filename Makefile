# Qu33ph GBA — builds qu33ph.gba with devkitARM (the GitHub Action does this for you)
ifeq ($(strip $(DEVKITARM)),)
$(error DEVKITARM is not set: install devkitPro's devkitARM, or let the GitHub Action build it)
endif
PREFIX  := $(DEVKITARM)/bin/arm-none-eabi-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
GBAFIX  := $(firstword $(wildcard $(DEVKITPRO)/tools/bin/gbafix) gbafix)
ARCH    := -mthumb -mthumb-interwork -mcpu=arm7tdmi -mtune=arm7tdmi
CFLAGS  := -O2 -Wall -Wno-unused-function -fno-strict-aliasing -ffunction-sections -fdata-sections $(ARCH)
LDFLAGS := $(ARCH) -specs=gba.specs -Wl,--gc-sections -Wl,-Map,qu33ph.map
SRC     := main game draw extras save casino mini gba gfx snd
OBJ     := $(addprefix build/,$(addsuffix .o,$(SRC)))

qu33ph.gba: qu33ph.elf
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@ -tQU33PH -cQU3E -mQU
	@ls -la $@

qu33ph.elf: $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@

build/%.o: source/%.c source/qu.h source/gfx.h source/snd.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build qu33ph.elf qu33ph.gba qu33ph.map
.PHONY: clean
