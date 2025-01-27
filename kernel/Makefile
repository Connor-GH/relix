KERNELDIR = kernel
KERNEL_SRC = $(wildcard $(KERNELDIR)/*.c)
KERNEL_ASM = $(wildcard $(KERNELDIR)/*.S)
ifeq ($(BITS),64)
	KERNEL_SRC += $(wildcard $(KERNELDIR)/64/*.c)
	KERNEL_ASM += $(wildcard $(KERNELDIR)/64/*.S)
else
	KERNEL_SRC += $(wildcard $(KERNELDIR)/32/*.c)
	KERNEL_ASM += $(wildcard $(KERNELDIR)/32/*.S)
endif
KERNEL_DRIVERS = $(KERNELDIR)/drivers/kbd.c \
								 $(KERNELDIR)/drivers/lapic.c \
								 $(KERNELDIR)/drivers/acpi.c
KERNEL_OBJS = $(KERNEL_SRC:$(KERNELDIR)/%.c=$(BIN)/%.o) $(KERNEL_ASM:$(KERNELDIR)/%.S=$(BIN)/%.o) \
	$(KERNEL_DRIVERS:$(KERNELDIR)/drivers/%.c=$(BIN)/%.o)
KERNEL_EXTRAS = -D__KERNEL__

ifneq ($(KUBSAN),)
	KUBSAN_FLAGS = -fsanitize=undefined
endif
KERNEL_INC = $(IVARS) \
						 -I$(KERNELDIR)/include \
						 -I$(KERNELDIR)/drivers/include \
						 -I$(KERNELDIR)/drivers \
						 $(KERNEL_EXTRAS)


include $(KERNELDIR)/boot/Makefile
include $(KERNELDIR)/rust/Makefile

$(BIN)/xv6.img: $(BIN)/kernel
	dd if=/dev/zero of=$@ count=10000
	dd if=$(BIN)/bootblock of=$@ conv=notrunc
	dd if=$(BIN)/kernel of=$@ seek=1 conv=notrunc

$(BIN)/xv6memfs.img: bootblock kernelmemfs
	dd if=/dev/zero of=$@ count=10000
	dd if=$(BIN)/bootblock of=$@ conv=notrunc
	dd if=$(BIN)/kernelmemfs of=$@ seek=1 conv=notrunc

$(BIN)/kernel: $(KERNELDIR)/kernel64.ld $(KERNEL_OBJS) $(BIN)/entry.o $(BIN)/entryother.o $(BIN)/initcode.o $(BIN)/bootblock $(BIN)/sign $(KERNEL_RUST_MODULES) $(SHAREDLIBS_STATIC)
	$(CC) $(CFLAGS) $(KUBSAN_FLAGS) -Wl,-melf_x86_64,-z,noexecstack,-O1 -Wl,-T,$(KERNELDIR)/kernel$(BITS).ld -o $@ $(BIN)/entry.o $(KERNEL_OBJS) $(RUST_OBJ) $(SHAREDLIBS_STATIC) -Wl,-b,binary $(BIN)/initcode $(BIN)/entryother

$(BIN)/%.o: $(KERNELDIR)/%.c
	$(CC) $(CFLAGS) $(KCFLAGS) $(KUBSAN_FLAGS) $(KERNEL_INC) -c $^ -o $@

$(BIN)/%.o: $(KERNELDIR)/drivers/%.c
	$(CC) $(CFLAGS) $(KCFLAGS) $(KUBSAN_FLAGS) $(KERNEL_INC) -c $^ -o $@

$(BIN)/%.o: $(KERNELDIR)/%.S
	$(CC) $(CFLAGS) $(KCFLAGS) $(KERNEL_INC) -c $^ -o $@
# kernelmemfs is a copy of kernel that maintains the
# disk image in memory instead of writing to a disk.
# This is not so useful for testing persistent storage or
# exploring disk buffering implementations, but it is
# great for testing the kernel on real hardware without
# needing a scratch disk.
MEMFSOBJS = $(filter-out ide.o,$(KERNEL_OBJS)) $(BIN)/memide.o
kernelmemfs: $(MEMFSOBJS) entry entryother_ initcode $(KERNELDIR)/kernel.ld fs.img
	$(LD) $(LDFLAGS) -T $(KERNELDIR)/kernel.ld -o $(BIN)/kernelmemfs $(BIN)/entry.o $(MEMFSOBJS) -b binary $(BIN)/initcode $(BIN)/entryother $(BIN)/fs.img
