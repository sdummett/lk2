# >> LINUX SYSCALLS << #


LINUX_ARCHIVE			:= linux-6.17.1.tar.xz
LINUX_DIR				:= linux-6.17.1
LINUX_CONFIG			:= linux-6.17.1/.config
LINUX_IMG				:= linux-6.17.1/arch/x86/boot/bzImage
DISKS_DIR				:= disks
ROOTFS_IMG				:= $(DISKS_DIR)/rootfs.ext4
JOBS					:= $(shell echo $$(( $$(nproc) - 1 )))
# INCLUDE					:= src/get_pid_info/userspace/


.PHONY: all linux dev clean fclean vm


all: dev


linux: $(LINUX_ARCHIVE) $(LINUX_DIR) $(LINUX_CONFIG) $(LINUX_IMG)


# using qemu to test the kernel with a minimal root filesystem
dev: $(ROOTFS_IMG) linux
	@echo "[COMPILE] kernel"
	$(MAKE) -C $(LINUX_DIR) -j$(JOBS) CC="gcc -std=gnu11" HOSTCC="gcc -std=gnu11"
	@echo "[EXEC] qemu: kernel+rootfs"
	./scripts/kernel-exec.sh $(LINUX_IMG) $(ROOTFS_IMG)


$(LINUX_IMG):
	@echo "[COMPILE] linux"
	yes | $(MAKE) -C $(LINUX_DIR) -j$(JOBS) CC="gcc -std=gnu11" HOSTCC="gcc -std=gnu11"


# Setup du fichier de config
# Partiel car a la compilation on nous promptera afin d'editer .config
# Ca aura ete preferable d'avoir fini la configuration de .config ici
$(LINUX_CONFIG):
	@echo "[CONFIG] linux .config (defconfig)"
	make -C $(LINUX_DIR) defconfig
	echo "CONFIG_VIRTIO_PCI=y" >> $(LINUX_DIR)/.config


$(LINUX_ARCHIVE):
	@echo "[DOWNLOAD] $(LINUX_ARCHIVE)"
	wget -O $(LINUX_ARCHIVE) https://cdn.kernel.org/pub/linux/kernel/v6.x/$(LINUX_ARCHIVE)


## gcc + flex + bison needed, need to check it
# sudo dnf install openssl-devel openssl-devel-engine
## en gros il faut connaitre les paquets requis par la compilation du kernel
$(LINUX_DIR):
	@echo "[UNPACK] $(LINUX_ARCHIVE)"
	tar -xvf $(LINUX_ARCHIVE)


$(ROOTFS_IMG):
	@mkdir -p $(DISKS_DIR)
	@echo "[INSTALL] $(ROOTFS_IMG)"
	./scripts/busybox.sh --reinstall $(ROOTFS_IMG)


# Creation d'une vm pour les besoins de l'ecole
vm-install:
	./scripts/vm.sh --install ./disks/debian.img
# Lancement d'une vm pour les besoins de l'ecole
vm-launch:
	./scripts/vm.sh --launch ./disks/debian.img

clean:
	@echo "[CLEAN]"
	$(MAKE) -C $(LINUX_DIR) mrproper || true # supprime .config arch/x86/boot/bzImage


fclean: clean
	@echo "[FCLEAN]"
	rm -f $(ROOTFS_IMG)
	rm -rf $(LINUX_DIR)
	rm -rf $(LINUX_ARCHIVE)
