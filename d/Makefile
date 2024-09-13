D_DIR = d
D_SRC = $(wildcard $(D_DIR)/*.d)
D_OBJ = $(D_SRC:$(D_DIR)/%.d=$(BIN)/%.o)

D_DIR_INCLUDES = -I$(D_DIR)/

$(BIN)/%.o: $(D_DIR)/%.d
	$(DC) $(DFLAGS) $(D_DIR_INCLUDES) -c -of$@ $^
	objcopy --remove-section .note.gnu.property $@
