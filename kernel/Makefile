KERNELDIR = kernel
KERNEL_SRC := $(wildcard $(KERNELDIR)/*.c)
KERNEL_ASM := $(wildcard $(KERNELDIR)/*.S)

KERNEL_SRC += $(wildcard $(KERNELDIR)/64/*.c)
KERNEL_ASM += $(wildcard $(KERNELDIR)/64/*.S)

KERNEL_DRIVER_DIR := $(KERNELDIR)/dev

KERNEL_DRIVERS_SRC := $(wildcard $(KERNEL_DRIVER_DIR)/*.c)

KERNEL_OBJS := $(KERNEL_SRC:$(KERNELDIR)/%.c=$(BIN)/%.o) $(KERNEL_ASM:$(KERNELDIR)/%.S=$(BIN)/%.o) \
	$(KERNEL_DRIVERS_SRC:$(KERNEL_DRIVER_DIR)/%.c=$(BIN)/%.o)
KERNEL_EXTRAS = -D__RELIX_KERNEL__=1

ifneq ($(KUBSAN),)
	KUBSAN_FLAGS = -fsanitize=undefined
endif
KERNEL_INC := $(IVARS) \
						 -I$(KERNELDIR)/include \
						 -I$(KERNELDIR)/drivers \
							$(KERNEL_EXTRAS)

KCFLAGS += -mcmodel=kernel


include $(KERNELDIR)/boot/Makefile
include $(KERNELDIR)/rust/Makefile

$(BIN)/relix.img: $(BIN)/kernel
	dd if=/dev/zero of=$@ count=10000
	dd if=$(BIN)/kernel of=$@ seek=1 conv=notrunc

$(BIN)/kernel: $(KERNELDIR)/kernel64.ld $(KERNEL_OBJS) $(BIN)/entry.o $(BIN)/entryother $(BIN)/initcode $(KERNEL_RUST_OBJ) $(LIBSHARED_STATIC)
	$(CC) $(CFLAGS) $(KUBSAN_FLAGS) -Wl,-melf_x86_64,-z,noexecstack,-O1 \
		-Wl,-T,$(KERNELDIR)/kernel64.ld -o $@ $(BIN)/entry.o $(KERNEL_OBJS) \
		$(KERNEL_RUST_OBJ) $(LIBSHARED_STATIC) -Wl,-b,binary $(BIN)/initcode $(BIN)/entryother
	./tools/gensyms.sh > sysroot/etc/ksyms.map

$(BIN)/%.o: $(KERNELDIR)/%.c
	$(CC) $(CFLAGS) $(KCFLAGS) $(KUBSAN_FLAGS) $(KERNEL_INC) -c $^ -o $@

$(BIN)/%.o: $(KERNEL_DRIVER_DIR)/%.c
	$(CC) $(CFLAGS) $(KCFLAGS) $(KUBSAN_FLAGS) $(KERNEL_INC) -c $^ -o $@

$(BIN)/%.o: $(KERNELDIR)/%.S
	$(CC) $(CFLAGS) $(KCFLAGS) $(KERNEL_INC) -c $^ -o $@
