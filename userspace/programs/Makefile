UPROGS_DIR = $(UDIR)/programs
UPROGS_SRC = $(wildcard $(UPROGS_DIR)/*.c)
UPROGS = $(UPROGS_SRC:$(UPROGS_DIR)/%.c=_%)



#ulib

$(BIN)/%.o: $(UPROGS_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

_%: $(BIN)/%.o $(ULIB_OBJ) $(ULIB_ASM_OBJ)
	# $(MAKE) $(ULIB_OBJ)
	objcopy --remove-section .note.gnu.property $(BIN)/ulib.o
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $(BIN)/$@ $^
	#$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $(BIN)/$@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym
