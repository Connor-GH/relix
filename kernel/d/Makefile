KERNEL_D_DIR = $(KERNELDIR)/d
include d/Makefile
include kernel/d/bindings/Makefile

KERNEL_D_SRC = $(wildcard $(KERNEL_D_DIR)/*.d)
KERNEL_D_OBJ = $(KERNEL_D_SRC:$(KERNEL_D_DIR)/%.d=$(BIN)/%_d.o)

# we will support dmd-style D compilers only.
# that is, dmd and ldmd2. ldc2 support is not official but may work
KERNEL_D_INCLUDES = $(D_DIR_INCLUDES) -I$(K_D_BINDINGS_DIR) -I$(KERNEL_D_DIR)/


$(BIN)/%_d.o: $(KERNEL_D_DIR)/%.d
	$(DC) $(DFLAGS) $(KERNEL_D_INCLUDES) -c -of$@ $^

