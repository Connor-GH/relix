UPROGS_DIR = $(UDIR)/programs
UPROGS_SRC = $(wildcard $(UPROGS_DIR)/*.c)
UPROGS = $(UPROGS_SRC:$(UPROGS_DIR)/%.c=_%)



$(BIN)/%.o: $(UPROGS_DIR)/%.c
	$(CC) $(CFLAGS) $(UCFLAGS) -c -o $@ $^
	$(OBJCOPY) --remove-section .note.gnu.property --set-section-flags .text=alloc,readonly $@

_%: $(BIN)/%.o $(ULIB_OBJ) $(ULIB_ASM_OBJ)
	$(CC) $(CFLAGS) $(UCFLAGS) -Wl,-z,noexecstack -Wl,-O1 -Wl,-N,-e,main -Wl,-Ttext,0 -o $(SYSROOT)/bin/$@ $^
