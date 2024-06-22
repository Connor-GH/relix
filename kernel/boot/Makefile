BOOTDIR = $(KERNELDIR)/boot
BOOT_INC = -I$(BOOTDIR) $(IVARS) $(KERNEL_INC)

sign:
	$(CC) -Werror -Wall -o $(BIN)/sign $(TOOLSDIR)/sign.c

# boot sector needs to be as small as possible so
# override the optimization with small size
bootblock: $(BOOTDIR)/bootasm.S $(BOOTDIR)/bootmain.c sign
	$(CC) $(CFLAGS) -Oz -fno-pic -nostdinc $(BOOT_INC) -c $(BOOTDIR)/bootmain.c -o $(BIN)/bootmain.o
	$(CC) $(CFLAGS) -fno-pic -nostdinc $(BOOT_INC) -c $(BOOTDIR)/bootasm.S -o $(BIN)/bootasm.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o $(BIN)/bootblock.o $(BIN)/bootasm.o $(BIN)/bootmain.o
	$(OBJCOPY) -S -O binary -j .text $(BIN)/bootblock.o $(BIN)/bootblock
	$(BIN)/sign $(BIN)/bootblock

entryother_: $(BOOTDIR)/entryother.S
	$(CC) $(CFLAGS) -Oz -fno-pic -nostdinc $(BOOT_INC) -c $(BOOTDIR)/entryother.S -o $(BIN)/entryother.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o $(BIN)/bootblockother.o $(BIN)/entryother.o
	$(OBJCOPY) -S -O binary -j .text $(BIN)/bootblockother.o $(BIN)/entryother

initcode: $(BOOTDIR)/initcode.S
	$(CC) $(CFLAGS) -Oz -nostdinc $(BOOT_INC) -c $(BOOTDIR)/initcode.S -o $(BIN)/initcode.o
	objcopy --remove-section .note.gnu.property $(BIN)/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $(BIN)/initcode.out $(BIN)/initcode.o
	$(OBJCOPY) -S -O binary $(BIN)/initcode.out $(BIN)/initcode
entry: $(BOOTDIR)/entry.S
	$(CC) $(CFLAGS) -nostdinc $(BOOT_INC) -c $(BOOTDIR)/entry.S -o $(BIN)/entry.o
	objcopy --remove-section .note.gnu.property $(BIN)/entry.o
