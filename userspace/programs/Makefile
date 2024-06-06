UPROGS_DIR = $(UDIR)/programs
UPROGS_SRC = $(wildcard $(UPROGS_DIR)/*.c)
UPROGS = $(UPROGS_SRC:$(UPROGS_DIR)/%.c=_%)



#ulib

$(BIN)/%.o: $(UPROGS_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^
	objcopy --remove-section .note.gnu.property $@

_%: $(BIN)/%.o $(ULIB_OBJ) $(ULIB_ASM_OBJ)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $(BIN)/$@ $^
