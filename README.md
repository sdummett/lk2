# LK2 - Drivers and Interrupts

> **Educational Project** - This project is developed for learning purposes as part of the 42 school curriculum. It demonstrates Linux kernel module development, device drivers, and input subsystem handling.

## Overview

LK2 is a Linux kernel module that implements a keyboard event logger. It captures all keyboard events through the Linux input subsystem and exposes them via a misc device at `/dev/keylogs`.

**This project is strictly for educational purposes to understand:**
- Linux kernel module development
- Device driver architecture
- Input subsystem and event handling
- Misc device implementation
- Ring buffer data structures in kernel space
- Proper kernel memory management and locking

## Features

- Captures all keyboard press and release events
- Logs timestamp (HH:MM:SS), key name, key code, and state
- Stores events in a lock-protected dynamic buffer (unlimited, starts at 1024)
- Exposes logs via `/dev/keylogs` misc device
- Prints a summary of typed characters on module unload

## Requirements

- Linux kernel >= 4.0 (developed with 6.18.5)
- GCC compiler
- Make
- QEMU (for VM testing)

## Project Structure

```
.
├── Makefile              # Main build system
├── module/
│   ├── Kbuild            # Kernel build configuration
│   ├── Makefile          # Out-of-tree build helper
│   ├── lk2.h             # Shared header and data structures
│   ├── lk2_main.c        # Module init/exit, misc device ops
│   ├── lk2_input.c       # Input subsystem handler
│   ├── lk2_ring.c        # Ring buffer implementation
│   └── lk2_format.c      # Key name mapping and formatting
├── scripts/              # Helper scripts for rootfs and QEMU
└── disks/                # Generated disk images
```

## Building

### Build everything (kernel + rootfs + module)
```bash
make dev
```

### Build only the module
```bash
make module
```

### Install module into rootfs
```bash
make module-install
```

### Quick iteration (build + install + run)
```bash
make dev-module
```

## Usage

### In the VM

1. Load the module:
```bash
insmod /lib/modules/extra/lk2.ko
```

2. Read the keylogs:
```bash
cat /dev/keylogs
```

3. Unload the module (prints summary to kernel log):
```bash
rmmod lk2
```

### Testing on Host System

You can also build and test the module directly on your current Linux system without QEMU.

**Install kernel headers:**

Debian/Ubuntu:
```bash
sudo apt install -y linux-headers-$(uname -r)
```

RHEL-family:
```bash
sudo dnf install -y kernel-devel-$(uname -r)
```

**Build and load:**
```bash
cd module/
make
sudo insmod lk2.ko
cat /dev/keylogs
sudo rmmod lk2
```

The summary is also written to `/tmp/keylogs_final.log` on unload.

### Log Format

Each entry follows the format:
```
HH:MM:SS KeyName(keycode) Pressed/Released
```

Example output:
```
14:32:15 H(35) Pressed
14:32:15 H(35) Released
14:32:16 E(18) Pressed
14:32:16 E(18) Released
14:32:16 L(38) Pressed
14:32:16 L(38) Released
14:32:17 L(38) Pressed
14:32:17 L(38) Released
14:32:17 O(24) Pressed
14:32:17 O(24) Released
```

## Make Targets

| Target | Description |
|--------|-------------|
| `dev` | Build kernel (if needed) + rootfs (if needed) + run QEMU |
| `run` | Launch QEMU with existing bzImage + rootfs |
| `module` | Build the kernel module |
| `module-install` | Inject the .ko into the rootfs image |
| `dev-module` | Module + install + run (fast iteration) |
| `rootfs` | Create rootfs if missing |
| `rootfs-reinstall` | Recreate rootfs (destructive) |
| `clean` | Clean kernel build artifacts |
| `fclean` | Full clean (kernel + rootfs + archive) |

## Technical Details

### Module Components

- **lk2_main.c**: Module initialization, misc device registration, and file operations
- **lk2_input.c**: Registers an input handler to capture keyboard events from all keyboard devices
- **lk2_ring.c**: Thread-safe ring buffer with spinlock protection
- **lk2_format.c**: Keycode to key name mapping (US QWERTY) and log line formatting

### Key Data Structures

```c
struct lk2_entry {
    struct lk2_time t;    // Timestamp (HH:MM:SS)
    u16 keycode;          // Linux input key code
    u8 state;             // Pressed or Released
    char ascii;           // ASCII value (if printable)
};
```

## Disclaimer

This software is created **solely for educational purposes** to learn about Linux kernel development and device drivers. It should only be used in controlled environments (virtual machines) for learning. Unauthorized use of keylogging software is illegal and unethical.
