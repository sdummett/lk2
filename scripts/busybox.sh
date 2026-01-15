#!/bin/bash
#
# busybox.sh — generation and maintenance of a minimalist ext4 rootfs image (BusyBox)
#
# Prerequisites (host):
#   - busybox installed and available in PATH
#   - sudo disponible (to mknod, mkfs, mount, rsync, andc.)
#
# Usage :
#   ./busybox.sh --install   <ROOTFS_IMG>
#   ./busybox.sh --reinstall <ROOTFS_IMG>
#   ./busybox.sh --add       <ROOTFS_IMG> <SRC> [DEST_IN_IMG]
#
# Notes :
#   - --add copies by default to /bin/<basename SRC>
#   - DEST_IN_IMG can be:
#       * an absolute path (e.g., /lib/modules/extra/lk2.ko)
#       * a relative path (e.g., lib/modules/extra/lk2.ko)
#       * a directory (e.g., /bin/ or lib/modules/extra/)
#   - Permissions :
#       * *.ko => 0644
#       * otherwise => 0755
#

set -euo pipefail

CMD="${1:-}"
ROOTFS_IMG="${2:-}"
SRC="${3:-}"
DEST="${4:-}"

die() {
  echo "$0: Erreur : $*" >&2
  exit 1
}

usage() {
  cat >&2 <<EOF
$0: usage :
  $0 --install   <ROOTFS_IMG>
  $0 --reinstall <ROOTFS_IMG>
  $0 --add       <ROOTFS_IMG> <SRC> [DEST_IN_IMG]
EOF
  exit 1
}

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || die "'$1' est requis mais introuvable dans le PATH"
}

cleanup_mount() {
  # $1: mountpoint (peut ne pas exister)
  local mp="${1:-}"
  if [[ -n "$mp" && -d "$mp" ]]; then
    if mountpoint -q "$mp" 2>/dev/null; then
      sudo umount "$mp" >/dev/null 2>&1 || true
    fi
    rmdir "$mp" >/dev/null 2>&1 || true
  fi
}

install_rootfs() {
  local img="$ROOTFS_IMG"
  [[ -n "$img" ]] || usage

  if [[ -f "$img" ]]; then
    echo "$0: $img existe déjà."
    exit 0
  fi

  require_cmd busybox
  require_cmd ldd
  require_cmd awk
  require_cmd rsync
  require_cmd dd
  require_cmd mkfs.ext4
  require_cmd du
  require_cmd cut
  require_cmd tr

  local rootfs_dir
  rootfs_dir="$(mktemp -d /tmp/rootfs.ext4.XXXXXX)"

# Ensure rootfs_dir is cleaned up even on error
  trap 'sudo rm -rf "'"$rootfs_dir"'" >/dev/null 2>&1 || true' RETURN

  echo "$0: [+] Préparation arborescence rootfs dans $rootfs_dir"
  sudo mkdir -p "$rootfs_dir"/{bin,sbin,etc,proc,sys,dev,dev/pts,tmp,root}
  sudo mkdir -p "$rootfs_dir"/usr/{bin,sbin}
  sudo chmod 1777 "$rootfs_dir/tmp"

  local busybox_bin
  busybox_bin="$(command -v busybox)"
  echo "$0: BusyBox = $busybox_bin"

  # BusyBox inside /bin
  sudo install -m 0755 "$busybox_bin" "$rootfs_dir/bin/busybox"

# Copy required libs if busybox is dynamically linked
# (ldd outputs lines where the 3rd column may be an absolute path)
  while IFS= read -r lib; do
    [[ -n "$lib" ]] || continue
    sudo cp --parents "$lib" "$rootfs_dir"
  done < <(ldd "$busybox_bin" | awk '{if (substr($3,1,1)=="/") print $3}')

  # Usual symlinks (to ajust if needed)
  for app in sh ash ls cat mount umount dmesg echo ps top uname \
             mkdir rmdir mv cp rm vi sleep kill ping; do
    sudo ln -sf /bin/busybox "$rootfs_dir/bin/$app"
  done

  # /dev minimal
  sudo mknod -m 600 "$rootfs_dir/dev/console" c 5 1 2>/dev/null || true
  sudo mknod -m 666 "$rootfs_dir/dev/null"    c 1 3 2>/dev/null || true

  # super simple /init
  cat << 'EOF' | sudo tee "$rootfs_dir/init" >/dev/null
#!/bin/sh

echo "[init] Booting minimal BusyBox rootfs"

# Mount pseudo filesystems
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev 2>/dev/null || echo "[init] devtmpfs mount failed"

# Ensure that /dev/console exist
[ -c /dev/console ] || mknod -m 600 /dev/console c 5 1

echo "[init] Spawning shell on /dev/console"
exec /bin/sh </dev/console >/dev/console 2>&1
EOF

  sudo chmod +x "$rootfs_dir/init"
  echo "$0: [+] Rootfs BusyBox construit dans $rootfs_dir"

  # Taille (MB) + marge
  local size_mb
  size_mb="$(sudo du -s -BM "$rootfs_dir" | cut -f1 | tr -d 'M')"
  [[ "$size_mb" =~ ^[0-9]+$ ]] || die "Impossible d'évaluer la taille de rootfs (size_mb='$size_mb')"
  local img_mb=$((size_mb + 64))

  echo "$0: [+] Taille rootfs: ${size_mb}M, image: ${img_mb}M"

  sudo rm -f "$img"
  sudo dd if=/dev/zero of="$img" bs=1M count="$img_mb" status=none
  sudo mkfs.ext4 -F "$img" >/dev/null

# Mount the image and copy the rootfs
  local mp
  mp="$(mktemp -d /tmp/busyrootfs.XXXXXX)"
  trap 'cleanup_mount "'"$mp"'"' RETURN

  echo "$0: [+] Montage image $img sur $mp"
  sudo mount -o loop "$img" "$mp"

  echo "$0: [+] Copie du rootfs vers l'image"
  sudo rsync -aHAX --numeric-ids "$rootfs_dir"/ "$mp"/

  echo "$0: [+] Démontage"
  sudo umount "$mp"
  rmdir "$mp"
  trap - RETURN

# Chown to the current user (convenient)
  sudo chown "$(id -u)":"$(id -g)" "$img"

  echo "$0: [+] Image rootfs prête: $img"
}

reinstall_rootfs() {
  local img="$ROOTFS_IMG"
  [[ -n "$img" ]] || usage

  echo "$0: [+] Re-installation (recréation) de l'image rootfs"
  sudo rm -f "$img"
  install_rootfs
}

add_file() {
  local img="$ROOTFS_IMG"
  local src="$SRC"
  local dest="$DEST"

  [[ -n "$img" && -n "$src" ]] || usage
  [[ -f "$img" ]] || die "image introuvable: $img"
  [[ -f "$src" ]] || die "fichier source introuvable: $src"

# Default destination: /bin/<basename>
  if [[ -z "$dest" ]]; then
    dest="/bin/$(basename "$src")"
  fi

# If DEST ends with '/', it is a directory
  if [[ "$dest" == */ ]]; then
    dest="${dest}$(basename "$src")"
  fi

# If DEST is relative, make it absolute inside the image
  if [[ "$dest" != /* ]]; then
    dest="/$dest"
  fi

# Mode: .ko => 0644, otherwise executable
  local mode="0755"
  if [[ "$src" == *.ko ]]; then
    mode="0644"
  fi

  local mp
  mp="$(mktemp -d /tmp/busyrootfs.XXXXXX)"
  trap 'cleanup_mount "'"$mp"'"' RETURN

  echo "$0: [+] Montage de l'image $img sur $mp"
  sudo mount -o loop "$img" "$mp"

  echo "$0: [+] Copie: $src -> $mp$dest (mode $mode)"
  sudo install -D -m "$mode" "$src" "$mp$dest"

  echo "$0: [+] Démontage"
  sudo umount "$mp"
  rmdir "$mp"
  trap - RETURN

  echo "$0: [+] Ajout OK : $dest dans $img"
}

# ---- Script entrypoint ----
[[ -n "$CMD" && -n "$ROOTFS_IMG" ]] || usage

case "$CMD" in
  --install)
    install_rootfs
    ;;
  --reinstall)
    reinstall_rootfs
    ;;
  --add)
    add_file
    ;;
  *)
    echo "Option inconnue : $CMD" >&2
    usage
    ;;
esac
