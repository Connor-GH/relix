include d/Makefile
USER_D_DIR = $(UDIR)/d
USER_D_SRC = $(wildcard $(USER_D_DIR)/*.d)
D_PROGS = $(USER_D_SRC:$(USER_D_DIR)/%.d=$(SYSROOT)/$(BIN)/_%)
D_PROGS_OBJ = $(USER_D_SRC:$(USER_D_DIR)/%.d=$(BIN)/%.o)

# we will support dmd-style D compilers only.
# that is, dmd and ldmd2. ldc2 support is not official but may work
USER_D_INCLUDES = $(D_DIR_INCLUDES) -I$(USER_D_DIR)/


$(BIN)/%.o: $(USER_D_DIR)/%.d
	$(DC) $(DFLAGS) $(USER_D_INCLUDES) -c -of$@ $^
	objcopy --remove-section .note.gnu.property $@

include $(USER_D_DIR)/bindings/Makefile
