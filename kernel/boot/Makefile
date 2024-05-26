BOOTDIR = $(KERNELDIR)/boot
BOOT_INC = -I$(BOOTDIR) $(IVARS) $(KERNEL_INC)

bootblock: $(BOOTDIR)/bootasm.S $(BOOTDIR)/bootmain.c
	$(CC) $(CFLAGS) -fno-pic -O -nostdinc $(BOOT_INC) -c $(BOOTDIR)/bootmain.c -o $(BIN)/bootmain.o
	$(CC) $(CFLAGS) -fno-pic -nostdinc $(BOOT_INC) -c $(BOOTDIR)/bootasm.S -o $(BIN)/bootasm.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o $(BIN)/bootblock.o $(BIN)/bootasm.o $(BIN)/bootmain.o
	# $(OBJDUMP) -S $(BIN)/bootblock.o > bootblock.asm
	$(OBJCOPY) -S -O binary -j .text $(BIN)/bootblock.o $(BIN)/bootblock
	./sign.pl $(BIN)/bootblock

entryother_: $(BOOTDIR)/entryother.S
	$(CC) $(CFLAGS) -fno-pic -nostdinc $(BOOT_INC) -c $(BOOTDIR)/entryother.S -o $(BIN)/entryother.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o $(BIN)/bootblockother.o $(BIN)/entryother.o
	$(OBJCOPY) -S -O binary -j .text $(BIN)/bootblockother.o $(BIN)/entryother
	#$(OBJDUMP) -S bootblockother.o > entryother.asm

initcode: $(BOOTDIR)/initcode.S
	$(CC) $(CFLAGS) -nostdinc $(BOOT_INC) -c $(BOOTDIR)/initcode.S -o $(BIN)/initcode.o
	objcopy --remove-section .note.gnu.property $(BIN)/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $(BIN)/initcode.out $(BIN)/initcode.o
	$(OBJCOPY) -S -O binary $(BIN)/initcode.out $(BIN)/initcode
	#$(OBJDUMP) -S initcode.o > initcode.asm
entry: $(BOOTDIR)/entry.S
	$(CC) $(CFLAGS) -nostdinc $(BOOT_INC) -c $(BOOTDIR)/entry.S -o $(BIN)/entry.o
	objcopy --remove-section .note.gnu.property $(BIN)/entry.o
