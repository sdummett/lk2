#!/bin/bash

set -e #x

CMD=$1
DEBIAN_IMG=$2

echo "CMD=$CMD"
echo "DEBIAN_IMG=$DEBIAN_IMG"

DEBIAN_ISO="debian-10.13.0-amd64-netinst.iso"
DEBIAN_ISO_PATH="/tmp/$DEBIAN_ISO"
# DEBIAN_IMG="debian.img"

function install
{
	if [[ -f $DEBIAN_IMG ]]; then
		echo "$0: $DEBIAN_IMG existe deja."
	fi
	qemu-img create -f raw $DEBIAN_IMG 30G

	if [[ ! -f "$DEBIAN_ISO_PATH" ]]; then 
		wget -O $DEBIAN_ISO_PATH "https://cdimage.debian.org/cdimage/archive/10.13.0/amd64/iso-cd/$DEBIAN_ISO"
	fi

	# Start the VM with specified resources
	qemu-system-x86_64 \
	-enable-kvm \
	-cpu host \
	-m 16G \
	-smp 14 \
	-hda $DEBIAN_IMG \
	-cdrom $DEBIAN_ISO_PATH \
	-boot d \
	-net nic -net user
}

function launch
{
	if [[ ! -f $DEBIAN_IMG ]]; then
		echo "$0: $DEBIAN_IMG introuvable."
		exit 1
	fi

	echo "$0: Lancement de la vm avec le disque $DEBIAN_IMG"
	qemu-system-x86_64 \
		-enable-kvm \
		-cpu host \
		-m 16G \
		-smp 14 \
		-hda $DEBIAN_IMG \
		-net nic -net user,hostfwd=tcp::2222-:22
}

case "$CMD" in
	--install)
		if [[ -z "$DEBIAN_IMG" ]]; then
			echo "$0: usage : $0 --install <DEBIAN_IMG>" >&2
			exit 1
		fi
		install
		;;

	--launch)
		if [[ -z "$DEBIAN_IMG" ]]; then
			echo "$0: usage : $0 --launch <DEBIAN_IMG>" >&2
			exit 1
		fi
		launch
		;;

	*)
		echo "Option inconnue : $CMD" >&2
		echo "$0: usage :"
		echo "  $0 --install   <DEBIAN_IMG>"
		echo "  $0 --launch    <DEBIAN_IMG>"
		exit 1
		;;
esac
