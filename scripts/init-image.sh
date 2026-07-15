#!/bin/sh
set -eu

IMG="${1:-kfs.img}"
MOUNT_POINT="/tmp/kfs_mount"

if [ -e "$IMG" ]; then
    echo "error: $IMG already exists"
    echo "remove it manually if you really want to recreate the image"
    exit 1
fi

dd if=/dev/zero of="$IMG" bs=1M count=10

loop=$(sudo losetup --show -fP "$IMG")
mounted=0

cleanup() {
    if [ "$mounted" -eq 1 ]; then
        sudo umount "$MOUNT_POINT" 2>/dev/null || true
    fi
    sudo losetup -d "$loop" 2>/dev/null || true
}
trap cleanup EXIT

sudo parted -s "$loop" mklabel msdos
sudo parted -s "$loop" mkpart primary ext2 1MiB 100%
sudo parted -s "$loop" set 1 boot on

sudo partprobe "$loop" || true
sleep 1

sudo mkfs.ext2 "${loop}p1"

sudo mkdir -p "$MOUNT_POINT"
sudo mount "${loop}p1" "$MOUNT_POINT"
mounted=1

sudo mkdir -p "$MOUNT_POINT/boot/grub"
sudo grub-install \
    --target=i386-pc \
    --boot-directory="$MOUNT_POINT/boot" \
    "$loop"

sudo tee "$MOUNT_POINT/boot/grub/grub.cfg" >/dev/null <<'EOF'
menuentry "kfs" {
    insmod multiboot
    multiboot /boot/kernel
    boot
}
EOF

echo "image initialized: $IMG"
