# **************************************************************************** #
#  LK2 — Kernel 6.18.5 + BusyBox rootfs + module dev                           #
#  Misc Device : /dev/keylogs                                                  #
# **************************************************************************** #

LINUX_ARCHIVE := linux-6.18.5.tar.xz
LINUX_DIR     := linux-6.18.5
LINUX_CONFIG  := $(LINUX_DIR)/.config
LINUX_IMG     := $(LINUX_DIR)/arch/x86/boot/bzImage

DISKS_DIR     := disks
ROOTFS_IMG    := $(DISKS_DIR)/rootfs.ext4

SCRIPTS_DIR   := scripts
BUSYBOX_SH    := $(SCRIPTS_DIR)/busybox.sh
KEXEC_SH      := $(SCRIPTS_DIR)/kernel-exec.sh

# Module
MODULE_DIR    := module
MODULE_NAME   := lk2
MODULE_KO     := $(MODULE_DIR)/$(MODULE_NAME).ko
# Where to install the module in the image (requires a patched busybox.sh)
MODULE_DEST   := /lib/modules/extra/$(MODULE_NAME).ko

# Jobs (robust when nproc=1)
NPROC := $(shell nproc 2>/dev/null || echo 1)
JOBS  := $(shell if [ $(NPROC) -gt 1 ]; then echo $$(( $(NPROC) - 1 )); else echo 1; fi)

.PHONY: all help \
        linux run dev dev-busybox dev-kernel \
        rootfs rootfs-reinstall rootfs-add \
        module module-clean module-install dev-module \
        vm-install vm-launch \
        clean fclean

all: dev

help:
	@echo "Targets:"
	@echo "  dev             : build kernel (if needed) + build rootfs (if needed) + run"
	@echo "  run             : launch QEMU with bzImage + rootfs (no rebuild)"
	@echo "  dev-busybox     : recreate rootfs + run (useful if /init/rootfs changes)"
	@echo "  dev-kernel      : force kernel rebuild + run (rootfs unchanged)"
	@echo "  rootfs          : create rootfs if missing"
	@echo "  rootfs-reinstall: recreate rootfs (destructive)"
	@echo "  rootfs-add BIN=... [DEST=/path/in/img] : inject a file into the image"
	@echo "  module          : build the module (M=...)"
	@echo "  module-install  : inject the .ko into the image ($(MODULE_DEST))"
	@echo "  dev-module      : module + install + run (fast iteration)"
	@echo ""
	@echo "In the VM: insmod $(MODULE_DEST) ; cat /dev/keylogs ; rmmod $(MODULE_NAME)"

# ---------- "linux ready" pipeline ----------
linux: $(LINUX_ARCHIVE) $(LINUX_DIR) $(LINUX_CONFIG) $(LINUX_IMG)

# ---------- Run (no rebuild) ----------
run: $(LINUX_IMG) $(ROOTFS_IMG)
	@echo "[EXEC] QEMU: kernel+rootfs"
	./$(KEXEC_SH) $(LINUX_IMG) $(ROOTFS_IMG)

# ---------- Dev workflows ----------
dev: linux rootfs
	@$(MAKE) run

dev-busybox: linux rootfs-reinstall
	@$(MAKE) run

dev-kernel: rootfs
	@$(MAKE) -B $(LINUX_IMG)
	@$(MAKE) run

# ---------- Kernel ----------
$(LINUX_IMG): $(LINUX_CONFIG)
	@echo "[COMPILE] Linux kernel"
	yes | $(MAKE) -C $(LINUX_DIR) -j$(JOBS) CC="gcc -std=gnu11" HOSTCC="gcc -std=gnu11"

$(LINUX_CONFIG): $(LINUX_DIR)
	@echo "[CONFIG] Linux defconfig"
	$(MAKE) -C $(LINUX_DIR) defconfig
	@# Keep the VirtIO addition; avoid duplicates
	@grep -q '^CONFIG_VIRTIO_PCI=y' $(LINUX_CONFIG) || echo "CONFIG_VIRTIO_PCI=y" >> $(LINUX_CONFIG)

$(LINUX_ARCHIVE):
	@echo "[DOWNLOAD] $(LINUX_ARCHIVE)"
	wget -O $(LINUX_ARCHIVE) https://cdn.kernel.org/pub/linux/kernel/v6.x/$(LINUX_ARCHIVE)

$(LINUX_DIR): $(LINUX_ARCHIVE)
	@echo "[UNPACK] $(LINUX_ARCHIVE)"
	tar -xvf $(LINUX_ARCHIVE)

# ---------- Rootfs / BusyBox ----------
rootfs: $(ROOTFS_IMG)

$(ROOTFS_IMG): $(BUSYBOX_SH)
	@mkdir -p $(DISKS_DIR)
	@echo "[INSTALL] rootfs image (create if absent): $(ROOTFS_IMG)"
	./$(BUSYBOX_SH) --install $(ROOTFS_IMG)

rootfs-reinstall: $(BUSYBOX_SH)
	@mkdir -p $(DISKS_DIR)
	@echo "[REINSTALL] rootfs image (recreate): $(ROOTFS_IMG)"
	./$(BUSYBOX_SH) --reinstall $(ROOTFS_IMG)

# Quick injection into the image
# Usage:
#   make rootfs-add BIN=module/lk2.ko DEST=/lib/modules/extra/lk2.ko
#   make rootfs-add BIN=./some_script.sh DEST=/bin/
rootfs-add: $(ROOTFS_IMG)
	@if [ -z "$(BIN)" ]; then \
		echo "Usage: make rootfs-add BIN=./path/to/file [DEST=/path/in/img]"; \
		exit 1; \
	fi
	@echo "[ADD] $(BIN) -> $(if $(DEST),$(DEST),/bin/$(notdir $(BIN))) in $(ROOTFS_IMG)"
	./$(BUSYBOX_SH) --add $(ROOTFS_IMG) $(BIN) $(DEST)

# ---------- Kernel module ----------
module: $(LINUX_DIR) $(LINUX_CONFIG)
	@echo "[COMPILE] module: $(MODULE_KO)"
	$(MAKE) -C $(LINUX_DIR) M=$$(pwd)/$(MODULE_DIR) modules

module-clean:
	@echo "[CLEAN] module build artifacts"
	$(MAKE) -C $(LINUX_DIR) M=$$(pwd)/$(MODULE_DIR) clean || true

module-install: module rootfs
	@echo "[INSTALL] module into rootfs: $(MODULE_DEST)"
	@$(MAKE) rootfs-add BIN=$(MODULE_KO) DEST=$(MODULE_DEST)

# Fastest cycle to iterate on the module:
# build .ko -> inject -> run qemu
dev-module: linux module-install
	@$(MAKE) run

# ---------- VM (if needed) ----------
vm-install:
	./scripts/vm.sh --install ./disks/debian.img

vm-launch:
	./scripts/vm.sh --launch ./disks/debian.img

# ---------- Cleaning ----------
clean:
	@echo "[CLEAN] kernel tree (mrproper)"
	$(MAKE) -C $(LINUX_DIR) mrproper || true

fclean: clean module-clean
	@echo "[FCLEAN] remove kernel sources/archive + rootfs image"
	rm -f $(ROOTFS_IMG)
	rm -rf $(LINUX_DIR)
	rm -f $(LINUX_ARCHIVE)
