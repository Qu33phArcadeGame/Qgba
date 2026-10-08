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
SRC     := main game draw extras save casino mini ball fidget bowl stack flip dozer pin jump gba gfx snd
OBJ     := $(addprefix build/,$(addsuffix .o,$(SRC)))

qu33ph.gba: qu33ph.elf
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@ -tQU33PH -cQU3E -mQU
	@ls -la $@

# The compiler's own floating-point and divide routines, moved into the GBA's fast RAM (they're
# ARM code: from the cartridge they ran about 4x slower, and every game leans on them).
LIBGCC  := $(shell $(CC) $(ARCH) -print-libgcc-file-name)
FASTLIB := _arm_addsubsf3.o _arm_muldivsf3.o _arm_cmpsf2.o _arm_fixsfsi.o _arm_fixunssfsi.o _arm_mulsf3.o _divsi3.o _udivsi3.o _arm_floatdisf.o _arm_floatundisf.o
build/fastlib.stamp:
	@mkdir -p build/fastlib
	for m in $(FASTLIB); do (cd build/fastlib && $(PREFIX)ar x $(LIBGCC) $$m 2>/dev/null) || true; done
	for o in build/fastlib/*.o; do [ -f "$$o" ] && $(OBJCOPY) --rename-section .text=.iwram $$o build/fast_$$(basename $$o); done; true
	@touch $@

qu33ph.elf: $(OBJ) build/fastlib.stamp
	$(CC) $(OBJ) $$(ls build/fast_*.o 2>/dev/null) $(LDFLAGS) -o $@

build/%.o: source/%.c source/qu.h source/gfx.h source/snd.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build qu33ph.elf qu33ph.gba qu33ph.map
.PHONY: clean
