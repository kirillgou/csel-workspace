EXEC=oleds
DTB = mydt.dtb
SRCS=$(wildcard *.c)

target=nano

TOOLCHAIN_PATH=/buildroot/output/host/usr/bin/
TOOLCHAIN=$(TOOLCHAIN_PATH)aarch64-linux-
CFLAGS=-Wall -Wextra -g -c -O0 -MD -std=gnu11
CFLAGS+=-mcpu=cortex-a53 -funwind-tables
##CFLAGS+=-O2 -fno-omit-frame-pointer
OBJDIR=.obj/nano

CC=$(TOOLCHAIN)gcc
LD=$(TOOLCHAIN)gcc
AR=$(TOOLCHAIN)ar
STRIP=$(TOOLCHAIN)strip

OBJDIR=.obj/$(target)
OBJS= $(addprefix $(OBJDIR)/, $(SRCS:.c=.o))

$(OBJDIR)/%o: %c
	$(CC) $(CFLAGS) $< -o $@

all: $(OBJDIR)/ $(EXEC) dtb
	echo $(EXEC)
	echo $(SRCS)

$(EXEC): $(OBJS) $(LINKER_SCRIPT)
	$(LD) $(OBJS) $(LDFLAGS) -o $@

$(OBJDIR)/:
	mkdir -p $(OBJDIR)

clean:
	rm -Rf $(OBJDIR) $(EXEC) $(EXEC)_s .obj *.dtb *~

clean_all: clean

CVER     := aarch64-linux-
KVER     := 5.15.21
CPU      := arm64

KDIR     := /buildroot/output/build/linux-$(KVER)/
PWD := $(shell pwd)

INCL+=-I. -I$(KDIR)/include -I$(KDIR)/arch/arm64/boot/dts/
DTS = $(DTB:.dtb=.dts)

dtb: $(DTB)
$(DTB) : $(DTS)
	ln -s $(KDIR)/arch/arm/boot/dts arm
	-cpp $(INCL) -E -P -x assembler-with-cpp $(DTS) | dtc -I dts -O dtb -o $(DTB) -
	rm arm

-include $(OBJS:.o=.d)

.PHONY: all clean clean_all dtb
