BOOTDIR := $(KERNELDIR)/boot
BOOT_INC := -I$(BOOTDIR) $(IVARS) $(KERNEL_INC)

$(BIN)/entryother: $(BOOTDIR)/entryother.S
	$(CC) $(CFLAGS) -Oz -fno-pic -nostdinc $(BOOT_INC) -c $(BOOTDIR)/entryother.S -o $(BIN)/entryother.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o $(BIN)/bootblockother.o $(BIN)/entryother.o
	$(OBJCOPY) -S -O binary -j .text $(BIN)/bootblockother.o $(BIN)/entryother

$(BIN)/initcode: $(BOOTDIR)/initcode64.S
	$(CC) $(CFLAGS) -Oz -nostdinc $(BOOT_INC) -c $(BOOTDIR)/initcode64.S -o $(BIN)/initcode.o
	$(OBJCOPY) --remove-section .note.gnu.property $(BIN)/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $(BIN)/initcode.out $(BIN)/initcode.o
	$(OBJCOPY) -S -O binary $(BIN)/initcode.out $(BIN)/initcode

$(BIN)/entry.o: $(BOOTDIR)/entry64.S
	$(CC) $(CFLAGS) -O0 -nostdinc $(BOOT_INC) -c $(BOOTDIR)/entry64.S -o $@
	$(OBJCOPY) --remove-section .note.gnu.property $(BIN)/entry.o
