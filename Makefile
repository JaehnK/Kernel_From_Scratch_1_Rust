# ============================================================
# KFS_1 Makefile
# 위치: kernel/Makefile
# ============================================================

TARGET      := i386-kernel
KERNEL_BIN  := kernel.bin
BUILD_BIN   := target/$(TARGET)/release/kernel
IMG         := ../kfs.img
MOUNT_POINT := /tmp/kfs_mount

ASM_SRC     := src/boot.s
ASM_OBJ     := src/boot.o

AS          := as
CARGO       := cargo +nightly

AS_FLAGS    := --32
CARGO_FLAGS := --release -Zjson-target-spec

.PHONY: all build asm install run clean re

all: build

build: $(KERNEL_BIN)

asm: $(ASM_OBJ)

$(ASM_OBJ): $(ASM_SRC)
	$(AS) $(AS_FLAGS) $< -o $@

$(KERNEL_BIN): $(ASM_OBJ) src/main.rs src/linker.ld build.rs i386-kernel.json Cargo.toml .cargo/config.toml
	$(CARGO) build $(CARGO_FLAGS)
	cp $(BUILD_BIN) $@
	grub-file --is-x86-multiboot $@

install: $(KERNEL_BIN)
	@set -eu; \
	echo ">>> Mounting $(IMG)..."; \
	loop=$$(sudo losetup --show -fP $(IMG)); \
	sudo mkdir -p $(MOUNT_POINT); \
	sudo mount "$${loop}p1" $(MOUNT_POINT); \
	sudo cp $(KERNEL_BIN) $(MOUNT_POINT)/boot/kernel; \
	sudo umount $(MOUNT_POINT); \
	sudo losetup -d "$$loop"; \
	echo ">>> Kernel installed"

run: install
	qemu-system-i386 -drive file=$(IMG),format=raw -display curses

clean:
	rm -f $(ASM_OBJ) $(KERNEL_BIN)
	$(CARGO) clean

re: clean all
