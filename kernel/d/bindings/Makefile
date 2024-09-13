K_D_BINDINGS_DIR = $(KERNEL_D_DIR)/bindings
K_D_BINDINGS_SRC = $(wildcard $(K_D_BINDINGS_DIR)/*.d)
K_D_BINDINGS_OBJ = $(K_D_BINDINGS_SRC:$(K_D_BINDINGS_DIR)/%.d=$(BIN)/%_d.o)

KERNEL_D_BINDINGS_INCLUDES = $(D_DIR_INCLUDES) -I$(K_D_BINDINGS_DIR)

$(BIN)/%_d.o: $(K_D_BINDINGS_DIR)/%.d
	$(DC) $(DFLAGS) $(KERNEL_D_BINDINGS_INCLUDES) -c -of$@ $^
	objcopy --remove-section .note.gnu.property $@
