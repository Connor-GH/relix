KERNEL_D_DIR = $(KERNELDIR)/d
include kernel/d/bindings/Makefile

KERNEL_D_SRC = $(wildcard $(KERNEL_D_DIR)/*.d)
KERNEL_D_OBJ = $(KERNEL_D_SRC:$(KERNEL_D_DIR)/%.d=$(BIN)/%_d.o)

ifneq ($(DC_type),dmd)
	DASH_D = -d
else
	DASH_D =
endif
	KDFLAGS = $(DASH_D)-version=__KERNEL__
# we will support dmd-style D compilers only.
# that is, dmd and ldmd2. ldc2 support is not official but may work
KERNEL_D_INCLUDES = -Ikernel/include $(D_DIR_INCLUDES) -I$(K_D_BINDINGS_DIR) -I$(KERNEL_D_DIR)/


$(BIN)/%_d.o: $(KERNEL_D_DIR)/%.d
	$(DC) $(DFLAGS) $(KDFLAGS) -checkaction=C $(KERNEL_D_INCLUDES) -c -of$@ $^

