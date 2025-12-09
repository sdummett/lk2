#!/bin/bash

## NOTE: busybox doit etre installer sur la machine hote

# - arrete le script des qu’une commande echoue (-e)
# - interdis l’usage de variables non définies (-u)
# - fais échouer un pipeline si une commande au milieu se plante (pipefail)
set -e #x

CMD=$1
ROOTFS_IMG=$2
BIN=$3

if [[ -z "$ROOTFS_IMG" ]]; then
	echo "$0: usage :"
	echo "  $0 --install   <ROOTFS_IMG>"
	echo "  $0 --reinstall <ROOTFS_IMG>"
	echo "  $0 --add       <ROOTFS_IMG> <BIN>"
	exit 1
fi

function install
{
	local img="$ROOTFS_IMG"

	if [ -f $img ]; then
		echo "$0: $img existe deja."
		exit 0
	fi
	### (1) preparer l'aborescence de base

	local rootfs_dir=$(mktemp -d /tmp/rootfs.ext4.XXXXXX)

	sudo mkdir -p "$rootfs_dir"/{bin,sbin,etc,proc,sys,dev,dev/pts,tmp,root}
	sudo mkdir -p "$rootfs_dir"/usr/{bin,sbin}
	sudo chmod 1777 "$rootfs_dir/tmp"   # /tmp
	if ! BUSYBOX_BIN=$(command -v busybox 2>/dev/null); then
		echo "$0: Erreur : busybox n'est pas installe ou introuvable dans le PATH." >&2
		exit 1
	fi

	echo "$0: BusyBox = $BUSYBOX_BIN"

	# mettre busybox dans /bin
	sudo cp "$BUSYBOX_BIN" "$rootfs_dir/bin/busybox"

	# copier les libs necessaires si busybox est dynamique
	for lib in $(ldd "$BUSYBOX_BIN" | awk '{if (substr($3,1,1)=="/") print $3}'); do
		sudo cp --parents "$lib" "$rootfs_dir"
	done

	### (3) creer quelques symlinks importants (sh, ls, etc.)

	for app in sh ash ls cat mount umount dmesg echo ps top uname \
			mkdir rmdir mv cp rm vi sleep kill ping; do
		sudo ln -sf /bin/busybox "$rootfs_dir/bin/$app"
	done

	### (4) /dev minimal (au cas ou devtmpfs ne monte pas tout seul)

	sudo mknod -m 600 "$rootfs_dir/dev/console" c 5 1 || true
	sudo mknod -m 666 "$rootfs_dir/dev/null"    c 1 3 || true

	### (5) script /init ultra simple

	cat << 'EOF' | sudo tee "$rootfs_dir/init" >/dev/null
#!/bin/sh

echo "[init] Booting minimal BusyBox rootfs"

# monter les pseudo-fs
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev 2>/dev/null || echo "[init] devtmpfs mount failed"

# s'assurer que /dev/console existe
[ -c /dev/console ] || mknod -m 600 /dev/console c 5 1

echo "[init] Spawning shell on /dev/console"
exec /bin/sh </dev/console >/dev/console 2>&1
EOF

	sudo chmod +x "$rootfs_dir/init"

	echo "$0: [+] Rootfs BusyBox construit dans $rootfs_dir"

	### 6) construire l'image ext4

	# taille du rootfs en Mo
	local size_mb=$(sudo du -s -BM "$rootfs_dir" | cut -f1 | tr -d M)
	local img_mb=$((size_mb + 64))   # marge de 64 Mo

	echo "$0: [+] Taille rootfs: ${size_mb}M, image: ${img_mb}M"

	sudo rm -f "$img"
	sudo dd if=/dev/zero of="$img" bs=1M count="$img_mb"
	sudo mkfs.ext4 "$img"

	# copier le rootfs dans l'image
	local mp=$(mktemp -d /tmp/busyrootfs.XXXXXX)

	sudo mkdir -p "$mp"
	sudo mount -o loop "$img" "$mp"

	sudo rsync -aHAX --numeric-ids "$rootfs_dir"/ "$mp"

	sudo umount "$mp"
	sudo rmdir "$mp"
	sudo rm -rf "$rootfs_dir"

	sudo chown $USER:$USER "$img"

	echo "$0: [+] Image rootfs prête: $img"
	exit 0
}

function reinstall
{
	echo "$0: [+] Re-installation de busybox"
	sudo rm -rf "$ROOTFS_IMG"

	install
}

function add()
{
	local img="$ROOTFS_IMG"
	local bin="$BIN"

	if [[ -z "$img" || -z "$bin" ]]; then
		echo "$0: usage : $0 --add <ROOTFS_IMG> <BIN>" >&2
		exit 1
	fi

	if [[ ! -f "$img" ]]; then
		echo "$0: Erreur : image $img introuvable." >&2
		exit 1
	fi

	if [[ ! -f "$bin" ]]; then
		echo "$0: Erreur : binaire $bin introuvable." >&2
		exit 1
	fi

	echo "$0: [+] Montage de l'image $img"

	local mp=$(mktemp -d /tmp/busyrootfs.XXXXXX)

	sudo mount -o loop "$img" "$mp"

	echo "$0: [+] Copie de $bin vers $mp/bin/"
	sudo cp "$bin" "$mp/bin/"
	sudo chmod +x "$mp/bin/$(basename "$bin")"

	echo "$0: [+] Démontage"
	sudo umount "$mp"
	rmdir "$mp"

	echo "$0: [+] Binaire ajouté : /bin/$(basename "$bin") dans $img"
}


case "$CMD" in
	--install)
		if [[ -z "$ROOTFS_IMG" ]]; then
			echo "$0: usage : $0 --install <ROOTFS_IMG>" >&2
			exit 1
		fi
		install
		;;

	--reinstall)
		if [[ -z "$ROOTFS_IMG" ]]; then
			echo "$0: usage : $0 --reinstall <ROOTFS_IMG>" >&2
			exit 1
		fi
		reinstall
		;;

	--add)
		add "$@"
		;;

	*)
		echo "Option inconnue : $CMD" >&2
		echo "$0: usage :"
		echo "  $0 --install   <ROOTFS_IMG>"
		echo "  $0 --reinstall <ROOTFS_IMG>"
		echo "  $0 --add       <ROOTFS_IMG> <BIN>"
		exit 1
		;;
esac
