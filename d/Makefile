D_DIR = d
D_SRC = $(wildcard $(D_DIR)/*.d)
D_PROGS = $(D_SRC:$(D_DIR)/%.d=$(SYSROOT)/$(BIN)/_%)
D_PROGS_OBJ = $(D_SRC:$(D_DIR)/%.d=$(BIN)/%.o)

# we will support dmd-style D compilers only.
# that is, dmd and ldmd2. ldc2 support is not official but may work,
DC ?= dmd
DFLAGS ?= -betterC -O -m32 $(EXTRA_DFLAGS)


$(BIN)/%.o: $(D_DIR)/%.d
	$(DC) $(DFLAGS) -c -of$@ $^
	objcopy --remove-section .note.gnu.property $@

include $(D_DIR)/bindings/Makefile
