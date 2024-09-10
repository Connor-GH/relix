D_BINDINGS_DIR = $(D_DIR)/bindings
D_BINDINGS_SRC = $(wildcard $(D_BINDINGS_DIR)/*.d)
D_BINDINGS_OBJ = $(D_BINDINGS_SRC:$(D_BINDINGS_DIR)/%.d=$(BIN)/%.o)

$(BIN)/%.o: $(D_BINDINGS_DIR)/%.d
	$(DC) $(DFLAGS) -c -of$@ $^
	objcopy --remove-section .note.gnu.property $@
