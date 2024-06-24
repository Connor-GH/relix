# Cross-compiling (e.g., on Mac OS X)
# TOOLPREFIX = i386-jos-elf

# Using native tools (e.g., on X86 Linux)
#TOOLPREFIX =

# Try to infer the correct TOOLPREFIX if not set
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if i386-jos-elf-objdump -i 2>&1 | grep '^elf32-i386$$' >/dev/null 2>&1; \
	then echo 'i386-jos-elf-'; \
	elif objdump -i 2>&1 | grep 'elf32-i386' >/dev/null 2>&1; \
	then echo ''; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find an i386-*-elf version of GCC/binutils." 1>&2; \
	echo "*** Is the directory with i386-jos-elf-gcc in your PATH?" 1>&2; \
	echo "*** If your i386-*-elf toolchain is installed with a command" 1>&2; \
	echo "*** prefix other than 'i386-jos-elf-', set your TOOLPREFIX" 1>&2; \
	echo "*** environment variable to that prefix and run 'make' again." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

# If the makefile can't find QEMU, specify its path here
# QEMU = qemu-system-i386

# Try to infer the correct QEMU
ifndef QEMU
QEMU = $(shell if which qemu > /dev/null; \
	then echo qemu; exit; \
	elif which qemu-system-i386 > /dev/null; \
	then echo qemu-system-i386; exit; \
	elif which qemu-system-x86_64 > /dev/null; \
	then echo qemu-system-x86_64; exit; \
	else \
	qemu=/Applications/Q.app/Contents/MacOS/i386-softmmu.app/Contents/MacOS/i386-softmmu; \
	if test -x $$qemu; then echo $$qemu; exit; fi; fi; \
	echo "***" 1>&2; \
	echo "*** Error: Couldn't find a working QEMU executable." 1>&2; \
	echo "*** Is the directory containing the qemu binary in your PATH" 1>&2; \
	echo "*** or have you tried setting the QEMU variable in Makefile?" 1>&2; \
	echo "***" 1>&2; exit 1)
endif

# TODO get rid of this
WNOFLAGS = -Wno-error=infinite-recursion
ARCHNOFLAGS = -mno-sse -mno-red-zone -mno-avx -mno-avx2

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld

OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

# llvm stuff
ifeq ($(LLVM),1)
	CC = clang
	AS = clang -c
	LD = ld.lld
	OBJCOPY = llvm-objcopy
	OBJDUMP = llvm-objdump
endif

WFLAGS = -Wnonnull

CFLAGS = -fno-pic -static -fno-builtin -ffreestanding \
				 -fno-strict-aliasing -nostdlib -O2 -Wall -ggdb -m32 -Werror -fno-omit-frame-pointer \
				 -nostdinc -fno-builtin $(ARCHNOFLAGS) $(WNOFLAGS) $(WFLAGS)
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
ASFLAGS = -m32 -gdwarf-2 -Wa,-divide --mx86-used-note=no
# FreeBSD ld wants ``elf_i386_fbsd''
ifeq ($(HOST_OS),FreeBSD)
	LDFLAGS += -m $(shell $(LD) -V | grep elf_i386 2>/dev/null | head -n 1)
else
	LDFLAGS += -m elf_i386
endif
LDFLAGS += -z noexecstack -O1


# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif



IVARS = -Iinclude/ -I.
# directories
TOOLSDIR = tools
BIN = bin
SYSROOT = sysroot

default:
	mkdir -p $(BIN)
	mkdir -p $(SYSROOT)/{bin,etc}
	$(MAKE) fs.img
	$(MAKE) xv6.img

include userspace/Makefile
include kernel/Makefile



mkfs: $(TOOLSDIR)/mkfs.c
	$(CC) -Werror -Wall -o $(BIN)/mkfs $(TOOLSDIR)/mkfs.c \
		-I$(KERNELDIR)/include -I$(KERNELDIR)/drivers/include -I$(KERNELDIR)/drivers -I.

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o


fs.img: mkfs $(UPROGS) $(D_PROGS)
	./$(BIN)/mkfs bin/fs.img README.md sysroot/etc/passwd $(UPROGS) $(D_PROGS)

clean:
	rm -f $(BIN)/*.o $(BIN)/*.sym $(BIN)/bootblock $(BIN)/entryother \
	$(SYSROOT)/bin/* \
	$(SYSROOT)/mkfs \
	$(BIN)/initcode \
	$(BIN)/initcode.out \
	$(BIN)/kernel \
	$(BIN)/xv6.img \
	$(BIN)/fs.img \
	$(BIN)/kernelmemfs \
	$(BIN)/xv6memfs.img mkfs $(BIN)/*

# run in emulators

bochs : fs.img xv6.img
	if [ ! -e .bochsrc ]; then ln -s dot-bochsrc .bochsrc; fi
	bochs -q

# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
CPUS := 2
endif
ifndef MEM
MEM := 2G
endif
QEMUOPTS = -drive file=$(BIN)/fs.img,index=1,media=disk,format=raw,if=ide,aio=native,cache.direct=on \
					 -drive file=$(BIN)/xv6.img,index=0,media=disk,format=raw,if=ide,aio=native,cache.direct=on \
					 -smp cpus=$(CPUS),cores=1,threads=1,sockets=$(CPUS) -m $(MEM) $(QEMUEXTRA)

ifdef CONSOLE_LOG
	QEMUOPTS += -serial mon:stdio
endif
qemu:
	$(MAKE) fs.img
	$(MAKE) xv6.img
	$(QEMU) $(QEMUOPTS)

qemu-memfs: xv6memfs.img
	$(QEMU) -drive file=$(BIN)/xv6memfs.img,index=0,media=disk,format=raw -smp $(CPUS) -m 256

qemu-nox: fs.img xv6.img
	$(QEMU) -nographic $(QEMUOPTS)

.gdbinit: debug/.gdbinit.tmpl
	sed "s/localhost:1234/localhost:$(GDBPORT)/" < $^ > $@

qemu-gdb:
	$(MAKE) fs.img
	$(MAKE) xv6.img
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)
	@echo "*** Now run 'gdb'." 1>&2

qemu-nox-gdb: fs.img xv6.img .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -nographic $(QEMUOPTS) -S $(QEMUGDB)

