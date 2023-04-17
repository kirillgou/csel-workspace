# Part executed when called from kernel build system
ifneq ($(KERNELRELEASE),)
obj-m += mod_ex_pilotes_7.o          ## name of the generated module
mod_ex_pilotes_7-objs := my_module.o  ## list of objets needed by that module
CFLAGS_my_module.o := -DDEBUG   ## to enable printing of debugging messages

# Part executed when called from standard make in the module source directory
else
# include ../../../src/kernel_settings
# kernel settings
CVER     := aarch64-buildroot-linux-gnu-
KVER     := 5.15.21
CPU      := arm64

KDIR     := /buildroot/output/build/linux-$(KVER)/
TOOLS    := /buildroot/output/host/usr/bin/$(CVER)
MODPATH  := /rootfs
#MODPATH := /buildroot/output/target

# include ../../../src/buildroot_path
# buildroot path
export PATH := /buildroot/output/host/usr/sbin$\
	:/buildroot/output/host/usr/bin/$\
	:/buildroot/output/host/sbin$\
	:/buildroot/output/host/bin/$\
	:$(PATH)

PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(CPU) CROSS_COMPILE=$(TOOLS) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean;

install:
	$(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_PATH=$(MODPATH) modules_install

endif
