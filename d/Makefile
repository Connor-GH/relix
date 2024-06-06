D_DIR = d
D_SRC = $(wildcard $(D_DIR)/*.d)
D_PROGS = $(D_SRC:$(D_DIR)/%.d=_%)

# we will support dmd-style D compilers only.
DC ?= dmd
DFLAGS ?= -betterC -O -m32 $(EXTRA_DFLAGS)


$(BIN)/%.o: $(D_DIR)/%.d
	$(DC) $(DFLAGS) -c -of=$@ $^
	objcopy --remove-section .note.gnu.property $@

_%: $(BIN)/%.o $(ULIB_OBJ) $(ULIB_ASM_OBJ)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $(BIN)/$@ $^
