BOOTDIR = $(KERNELDIR)/boot
BOOT_INC = -I$(BOOTDIR) $(IVARS) $(KERNEL_INC)

$(BIN)/sign:
	$(CC) -Werror -Wall -o $@ $(TOOLSDIR)/sign.c

# boot sector needs to be as small as possible so
# override the optimization with small size

$(BIN)/bootmain.o: $(BOOTDIR)/bootmain.c
	$(CC) -fno-builtin -Oz -m32 -fno-pic -nostdinc $(BOOT_INC) -c $(BOOTDIR)/bootmain.c -o $@

$(BIN)/bootblock: $(BOOTDIR)/bootasm.S $(BIN)/bootmain.o $(BIN)/sign
	$(CC) -fno-builtin -fno-pic -m32 -nostdinc $(BOOT_INC) -c $(BOOTDIR)/bootasm.S -o $(BIN)/bootasm.o
	$(LD) -melf_i386 -N -e start -Ttext 0x7C00 -o $(BIN)/bootblock.o $(BIN)/bootasm.o $(BIN)/bootmain.o
	$(OBJCOPY) -S -O binary -j .text $(BIN)/bootblock.o $@
	$(BIN)/sign $(BIN)/bootblock

$(BIN)/entryother.o: $(BOOTDIR)/entryother.S
	$(CC) $(CFLAGS) -Oz -fno-pic -nostdinc $(BOOT_INC) -c $(BOOTDIR)/entryother.S -o $(BIN)/entryother.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o $(BIN)/bootblockother.o $@
	$(OBJCOPY) -S -O binary -j .text $(BIN)/bootblockother.o $(BIN)/entryother

$(BIN)/initcode.o: $(BOOTDIR)/initcode$(BITS).S
	$(CC) $(CFLAGS) -Oz -nostdinc $(BOOT_INC) -c $(BOOTDIR)/initcode$(BITS).S -o $(BIN)/initcode.o
	objcopy --remove-section .note.gnu.property $(BIN)/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $(BIN)/initcode.out $@
	$(OBJCOPY) -S -O binary $(BIN)/initcode.out $(BIN)/initcode
$(BIN)/entry.o: $(BOOTDIR)/entry$(BITS).S
	$(CC) $(CFLAGS) -O0 -nostdinc $(BOOT_INC) -c $(BOOTDIR)/entry$(BITS).S -o $@
	objcopy --remove-section .note.gnu.property $(BIN)/entry.o
