# Kernel driver — `buzzer.ko`

A Linux character device driver that controls the BPT-14X piezoelectric buzzer on GPIO 12 of the Raspberry Pi 4B by writing directly to the BCM2711 GPIO registers.

The driver registers `/dev/buzzer0` as a standard character device. Writing `'0'` to it drives the pin low (silence); writing any other byte drives it high (buzzer on). A user-space `Buzzer` C++ class (in [`../firmware/buzzer.cpp`](../firmware/buzzer.cpp)) wraps this with a POSIX timer for one-shot activation of a configurable duration.

## How it works

1. `alloc_chrdev_region` reserves a major/minor pair.
2. `class_create` + `device_create` create `/dev/buzzer0`.
3. `cdev_init` + `cdev_add` wire up the `file_operations` table (`open`, `release`, `read`, `write`).
4. `ioremap(GPIO_BASE, ...)` maps the BCM2711 GPIO register block into kernel virtual address space.
5. `SetGPIOFunction` configures GPIO 12 as output.
6. On `write`, the driver reads the first byte of the user buffer and calls `SetGPIOOutputValue` accordingly.
7. On module unload, the pin is reset to input, the mapping is undone, and the device/class/region are torn down.

Direct register access is deliberate here — the aim of the exercise was to demonstrate a self-contained GPIO driver without relying on the kernel's `gpiod` abstraction.

## Files

```
kernel-driver/
├── Makefile           out-of-tree kernel module build
├── buzzermodule.c     module entry/exit, file_operations, character device setup
├── utils.c            GPIO register helpers (SetGPIOFunction, SetGPIOOutputValue)
└── utils.h            GpioRegisters struct + GPIO_BASE definition
```

## Build

The `Makefile` builds against the kernel tree produced by Buildroot and uses the Buildroot ARM64 cross-compiler.

```bash
# Edit KDIR and CROSS_COMPILE in the Makefile to point at your Buildroot output
make
```

Output: `buzzer.ko` (loadable module), plus the usual `.o`, `.mod`, `.mod.c` intermediates.

## Install and use on the Pi

```bash
# Copy the module to the device
scp buzzer.ko root@<pi-ip>:/root/

# On the Pi:
insmod /root/buzzer.ko
ls -l /dev/buzzer0          # should exist, major/minor auto-allocated
dmesg | tail                # driver prints "buzzerModule_init: called"

# Manual test (buzzer on for ~1s, then off):
echo -n 1 > /dev/buzzer0 ; sleep 1 ; echo -n 0 > /dev/buzzer0

# Unload:
rmmod buzzer
```

The firmware (`../firmware`) opens `/dev/buzzer0` through `std::ofstream` and does not require any additional tooling.

## Wiring

The buzzer is connected between GPIO 12 (physical pin 32) and GND (physical pin 14). No external driver transistor is needed for the BPT-14X at 3.3 V — it draws ~7 mA.
