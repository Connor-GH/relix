UPROGS_DIR = $(UDIR)/programs
UPROGS_SRC = $(wildcard $(UPROGS_DIR)/*.c)
UPROGS = $(UPROGS_SRC:$(UPROGS_DIR)/%.c=$(SYSROOT)/$(BIN)/_%)
UPROGS_OBJ = $(UPROGS_SRC:$(UPROGS_DIR)/%.c=$(BIN)/%.o)



$(BIN)/%.o: $(UPROGS_DIR)/%.c
	$(CC) $(CFLAGS) $(UCFLAGS) -c -o $@ $^

# link together, then remove the .note.gnu.property that GCC generates.
$(SYSROOT)/$(BIN)/_%: $(BIN)/%.o $(ULIB_STATIC)
	$(CC) $(CFLAGS) $(UCFLAGS) -Wl,-z,noexecstack -Wl,-O1 -Wl,-N,-e,_start -Wl,--section-start=.text=0 -o $@ $^
	$(OBJCOPY) --remove-section .note.gnu.property $@
