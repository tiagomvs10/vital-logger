obj-m := buzzer.o
buzzer-objs:=buzzermodule.o utils.o
KDIR := /home/tiago/buildroot/buildroot-2025.02.6/output/build/linux-custom
ARCH ?= arm64
CROSS_COMPILE ?= /home/tiago/buildroot/buildroot-2025.02.6/output/host/bin/aarch64-buildroot-linux-gnu-

export ARCH CROSS_COMPILE


KBUILD_TARGETS	:= clean help modules modules_install

.PHONY: all $(KBUILD_TARGETS)

all: modules
$(KBUILD_TARGETS):
	#$(MAKE) -C $(KDIR) M=$(shell pwd) $@ EXTRA_CFLAGS="$(MY_CFLAGS)"
	$(MAKE) -C $(KDIR) M="$(shell pwd)" $@
