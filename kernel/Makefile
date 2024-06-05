KERNELDIR = kernel
KERNEL_SRC = $(wildcard $(KERNELDIR)/*.c)
KERNEL_ASM = $(wildcard $(KERNELDIR)/*.S)
KERNEL_DRIVERS = $(KERNELDIR)/drivers/kbd.c $(KERNELDIR)/drivers/lapic.c
KERNEL_OBJS = $(KERNEL_SRC:$(KERNELDIR)/%.c=$(BIN)/%.o) $(KERNEL_ASM:$(KERNELDIR)/%.S=$(BIN)/%.o) \
	$(KERNEL_DRIVERS:$(KERNELDIR)/drivers/%.c=$(BIN)/%.o)

KERNEL_INC = -I$(KERNELDIR)/include -I$(KERNELDIR)/drivers/include -I$(KERNELDIR)/drivers

include $(KERNELDIR)/boot/Makefile

xv6.img: bootblock kernel
	dd if=/dev/zero of=$(BIN)/xv6.img count=10000
	dd if=$(BIN)/bootblock of=$(BIN)/xv6.img conv=notrunc
	dd if=$(BIN)/kernel of=$(BIN)/xv6.img seek=1 conv=notrunc

xv6memfs.img: bootblock kernelmemfs
	dd if=/dev/zero of=$(BIN)/xv6memfs.img count=10000
	dd if=$(BIN)/bootblock of=$(BIN)/xv6memfs.img conv=notrunc
	dd if=$(BIN)/kernelmemfs of=$(BIN)/xv6memfs.img seek=1 conv=notrunc

kernel: $(KERNELDIR)/kernel.ld $(KERNEL_OBJS) entry entryother_ initcode
	$(LD) $(LDFLAGS) -T $(KERNELDIR)/kernel.ld -o $(BIN)/kernel $(BIN)/entry.o $(KERNEL_OBJS) -b binary $(BIN)/initcode $(BIN)/entryother
	$(OBJDUMP) -t $(BIN)/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel.sym

$(BIN)/%.o: $(KERNELDIR)/%.c
	$(CC) $(CFLAGS) $(KERNEL_INC) -c $^ -o $@

$(BIN)/%.o: $(KERNELDIR)/drivers/%.c
	$(CC) $(CFLAGS) $(KERNEL_INC) -c $^ -o $@

$(BIN)/%.o: $(KERNELDIR)/%.S
	$(CC) $(CFLAGS) $(KERNEL_INC) -c $^ -o $@
# kernelmemfs is a copy of kernel that maintains the
# disk image in memory instead of writing to a disk.
# This is not so useful for testing persistent storage or
# exploring disk buffering implementations, but it is
# great for testing the kernel on real hardware without
# needing a scratch disk.
MEMFSOBJS = $(filter-out ide.o,$(KERNEL_OBJS)) $(BIN)/memide.o
kernelmemfs: $(MEMFSOBJS) entry entryother_ initcode $(KERNELDIR)/kernel.ld fs.img
	$(LD) $(LDFLAGS) -T $(KERNELDIR)/kernel.ld -o $(BIN)/kernelmemfs $(BIN)/entry.o $(MEMFSOBJS) -b binary $(BIN)/initcode $(BIN)/entryother $(BIN)/fs.img
	$(OBJDUMP) -t $(BIN)/kernelmemfs | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernelmemfs.sym
