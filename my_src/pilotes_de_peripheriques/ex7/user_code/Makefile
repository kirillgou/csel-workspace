EXE=app
SRCS=$(wildcard *.c)

ifeq ($(target),)
target=nano
endif

CFLAGS=-Wall -Wextra -g -c -O0 -MD -std=gnu11

TOOLCHAIN_PATH=/buildroot/output/host/usr/bin/
TOOLCHAIN=$(TOOLCHAIN_PATH)aarch64-linux-
CFLAGS+=-mcpu=cortex-a53 -funwind-tables
##CFLAGS+=-O2 -fno-omit-frame-pointer
OBJDIR=.obj/nano
EXEC=$(EXE)

CC=$(TOOLCHAIN)gcc
LD=$(TOOLCHAIN)gcc
AR=$(TOOLCHAIN)ar
STRIP=$(TOOLCHAIN)strip

OBJDIR=.obj/$(target)
OBJS= $(addprefix $(OBJDIR)/, $(SRCS:.c=.o))

$(OBJDIR)/%o: %c
	$(CC) $(CFLAGS) $< -o $@

all: $(OBJDIR)/ $(EXEC)

$(EXEC): $(OBJS) $(LINKER_SCRIPT)
	$(LD) $(OBJS) $(LDFLAGS) -o $@

$(OBJDIR)/:
	mkdir -p $(OBJDIR)

clean:
	rm -Rf $(OBJDIR) $(EXEC) $(EXEC)_s *~

clean_all: clean
	rm -Rf .obj $(EXE) $(EXE)_s $(EXE)_a $(EXE)_a_s $(EXE)_h $(EXE)_h_s

-include $(OBJS:.o=.d)

.PHONY: all clean clean_all
