KERNEL_D_DIR = $(KERNELDIR)/d
KERNEL_D_SRC = $(wildcard $(KERNEL_D_DIR)/*.d)
KERNEL_D_OBJ = $(KERNEL_D_SRC:$(KERNEL_D_DIR)/%.d=$(BIN)/%.o)

# we will support dmd-style D compilers only.
# that is, dmd and ldmd2. ldc2 support is not official but may work
KERNEL_D_INCLUDES = -I$(KERNEL_D_DIR)/


$(BIN)/%.o: $(KERNEL_D_DIR)/%.d
	$(DC) $(DFLAGS) $(D_INCLUDES) -c -of$@ $^
