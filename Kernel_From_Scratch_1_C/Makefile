# Kernel From Scratch 1 - C port

PREFIX ?= /home/jinseo/opt/cross
TARGET ?= i686-elf
export PATH := $(PREFIX)/bin:$(PATH)

CC := $(TARGET)-gcc
AS := $(TARGET)-as
GDB ?= gdb
GRUB_FILE ?= grub-file
QEMU ?= qemu-system-i386

KERNEL_BIN := kernel.bin
BUILD_DIR := build
BUILD_STAMP := $(BUILD_DIR)/.stamp
GDB_KFS_INIT := .gdbinit-kfs
IMG ?= ../Kernel_From_Scratch_1_Rust/kfs.img
MOUNT_POINT ?= /tmp/kfs_mount

ASM_SRC := src/boot.s
C_SRCS := \
	src/vga.c \
	src/string.c \
	src/kernel.c \
	src/debug.c \

LINKER_SCRIPT := src/linker.ld

OPT ?= -O2
GDB_PORT ?= 1234
GDB_FLAGS ?= -q -nx
QEMU_MONITOR_HOST ?= 127.0.0.1
QEMU_MONITOR_PORT ?= 45454
QEMU_MACHINE ?= pc
QEMU_CPU ?= qemu32
QEMU_SMP ?= 1
QEMU_MEMORY ?= 64M
QEMU_DISPLAY ?= curses
QEMU_SERIAL ?= none
QEMU_NET ?= none
QEMU_BOOT ?= order=c
QEMU_EXTRA ?=

C_OBJS := $(C_SRCS:src/%.c=$(BUILD_DIR)/%.o)
OBJS := $(BUILD_DIR)/boot.o $(C_OBJS)

CFLAGS := -std=gnu11 -ffreestanding $(OPT) -g -Wall -Wextra
CFLAGS += -fno-builtin -fno-stack-protector -fno-pic -fno-pie
CFLAGS += -fno-omit-frame-pointer
LDFLAGS := -T $(LINKER_SCRIPT) -ffreestanding $(OPT) -nostdlib
LDFLAGS += -Wl,-z,max-page-size=0x1000 -Wl,--build-id=none
LDLIBS := -lgcc

QEMU_DRIVE_ARGS := -drive file=$(IMG),format=raw,if=ide
QEMU_BASE_ARGS := -machine $(QEMU_MACHINE) -cpu $(QEMU_CPU) -smp $(QEMU_SMP)
QEMU_BASE_ARGS += -m $(QEMU_MEMORY) $(QEMU_DRIVE_ARGS) -boot $(QEMU_BOOT)
QEMU_BASE_ARGS += -net $(QEMU_NET) -no-reboot -no-shutdown
QEMU_MONITOR_ARGS := -monitor telnet:$(QEMU_MONITOR_HOST):$(QEMU_MONITOR_PORT),server,nowait
QEMU_GDB_ARGS := -S -gdb tcp::$(GDB_PORT)

.PHONY: all build check install run run-monitor run-nographic debug debug-nographic gdb gdb-rich gdb-tui gdb-gef monitor clean re

all: build

build: $(KERNEL_BIN)

$(BUILD_STAMP):
	mkdir -p $(BUILD_DIR)
	touch $@

$(BUILD_DIR)/boot.o: $(ASM_SRC) | $(BUILD_STAMP)
	$(AS) $< -o $@

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_STAMP)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): $(OBJS) $(LINKER_SCRIPT)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

check: $(KERNEL_BIN)
	$(GRUB_FILE) --is-x86-multiboot $<

install: $(KERNEL_BIN)
	@set -eu; \
	loop=$$(sudo losetup --show -fP "$(IMG)"); \
	sudo mkdir -p "$(MOUNT_POINT)"; \
	sudo mount "$${loop}p1" "$(MOUNT_POINT)"; \
	sudo mkdir -p "$(MOUNT_POINT)/boot/grub"; \
	sudo cp "$(KERNEL_BIN)" "$(MOUNT_POINT)/boot/kernel"; \
	printf 'menuentry "kfs-c" {\n    multiboot /boot/kernel\n    boot\n}\n' | sudo tee "$(MOUNT_POINT)/boot/grub/grub.cfg" >/dev/null; \
	sudo umount "$(MOUNT_POINT)"; \
	sudo losetup -d "$$loop"

run: install
	$(QEMU) $(QEMU_BASE_ARGS) -display $(QEMU_DISPLAY) -serial $(QEMU_SERIAL) $(QEMU_EXTRA)

run-monitor: install
	$(QEMU) $(QEMU_BASE_ARGS) -display $(QEMU_DISPLAY) -serial $(QEMU_SERIAL) $(QEMU_MONITOR_ARGS) $(QEMU_EXTRA)

run-nographic: install
	$(QEMU) $(QEMU_BASE_ARGS) -nographic -serial mon:stdio $(QEMU_EXTRA)

debug: install
	$(QEMU) $(QEMU_BASE_ARGS) -display $(QEMU_DISPLAY) -serial $(QEMU_SERIAL) $(QEMU_MONITOR_ARGS) $(QEMU_GDB_ARGS) $(QEMU_EXTRA)

debug-nographic: install
	$(QEMU) $(QEMU_BASE_ARGS) -nographic -serial mon:stdio $(QEMU_GDB_ARGS) $(QEMU_EXTRA)

gdb: $(KERNEL_BIN)
	$(GDB) $(GDB_FLAGS) $(KERNEL_BIN) -ex "set architecture i386" -ex "target remote localhost:$(GDB_PORT)"

gdb-rich: $(KERNEL_BIN) $(GDB_KFS_INIT)
	$(GDB) -q -nx $(KERNEL_BIN) -ex "set architecture i386" -x $(GDB_KFS_INIT) -ex "target remote localhost:$(GDB_PORT)"

gdb-tui: $(KERNEL_BIN) $(GDB_KFS_INIT)
	$(GDB) -q -nx -tui $(KERNEL_BIN) -ex "set architecture i386" -x $(GDB_KFS_INIT) -ex "target remote localhost:$(GDB_PORT)" -ex "layout split"

gdb-gef: $(KERNEL_BIN)
	$(GDB) -q $(KERNEL_BIN) -ex "set architecture i386" -ex "target remote localhost:$(GDB_PORT)"

monitor:
	nc $(QEMU_MONITOR_HOST) $(QEMU_MONITOR_PORT)

clean:
	rm -rf $(BUILD_DIR) $(KERNEL_BIN)

re: clean all
