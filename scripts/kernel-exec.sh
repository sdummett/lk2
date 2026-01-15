#!/bin/bash

# Ce script execute un kernel linux avec un root filesystem

# Afin de quitter le script des qu'une erreur se produit
set -e #x

CMD=$(basename $0)
# the path to where root filesystem image lives
ROOTFS_IMG=$2
# # the path to where the kernel image lives
LINUX_KERNEL_IMAGE=$1

# -z: test si la chaine est vide
if [[ -z "$LINUX_KERNEL_IMAGE" || -z "$ROOTFS_IMG" ]]; then
	echo "$0: usage: $CMD <linux_kernel_image_path> <root_filesystem_path>"
	exit 1
fi

# on lance le kernel avec un rootfs grace a qemu
# qemu-system-x86_64 \
#   -kernel $LINUX_KERNEL_IMAGE \
#   -append "root=/dev/vda rw console=ttyS0" \
#   -drive file=$ROOTFS_IMG,if=virtio,format=raw \
#   -m 1024 \
#   -nographic

qemu-system-x86_64 \
  -kernel "$LINUX_KERNEL_IMAGE" \
  -append "root=/dev/vda rw console=tty0 console=ttyS0" \
  -drive file="$ROOTFS_IMG",if=virtio,format=raw \
  -m 1024 \
  -serial mon:stdio

# Bonus: la commande pour lancer le noyau sans root filesystem
#qemu-system-x86_64 \
#    -kernel "$BUILD/arch/x86/boot/bzImage" \
#    -append "root=/dev/sda1 rw console=ttyS0" \
#    -hda ft_linux.img \
#    -nographic
