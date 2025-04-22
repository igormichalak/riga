PROGRAM_NAME := main

# Toolchain
RISCV_PREFIX ?= riscv64-unknown-elf-
CC      := $(RISCV_PREFIX)gcc
OBJCOPY := $(RISCV_PREFIX)objcopy
OBJDUMP := $(RISCV_PREFIX)objdump

# Flags
CFLAGS  := -march=rv64im -mabi=lp64 -nostdlib -ffreestanding -Wall -Wextra -g
LDFLAGS := -T memmap.ld -nostdlib

# Files
SOURCES    := main.s
OBJECTS    := $(SOURCES:.s=.o)
TARGET_ELF := $(PROGRAM_NAME).elf
TARGET_BIN := $(PROGRAM_NAME).bin

.PHONY: all
all: $(TARGET_BIN)

%.o: %.s
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET_ELF): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

$(TARGET_BIN): $(TARGET_ELF)
	$(OBJCOPY) -O binary $< $@

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(TARGET_ELF) $(TARGET_BIN)

.PHONY: disassemble
disassemble: $(TARGET_BIN)
	$(OBJDUMP) -b binary -m riscv:rv64 -D $(TARGET_BIN)

