UDIR = userspace
ULIB = $(wildcard $(UDIR)/*.c)
ULIB_OBJ = $(ULIB:$(UDIR)/%.c=$(BIN)/%.o)
ULIB_ASM = $(wildcard $(UDIR)/*.S)
ULIB_ASM_OBJ = $(ULIB_ASM:$(UDIR)/%.S=$(BIN)/%.o)

UCFLAGS = -I$(UDIR)/include -I. $(IVARS) -DKERNEL_PREFIX="\"../kernel/include\""

$(ULIB_OBJ): $(BIN)/%.o : $(UDIR)/%.c
	$(CC) $(CFLAGS) $(UCFLAGS) -c -o $@ $^
	$(OBJCOPY) --remove-section .note.gnu.property --set-section-flags .text=alloc,readonly $@

$(ULIB_ASM_OBJ): $(BIN)/%.o : $(UDIR)/%.S
	$(CC) $(CFLAGS) $(UCFLAGS) -c -o $@ $^
	$(OBJCOPY) --remove-section .note.gnu.property --set-section-flags .text=alloc,readonly $@

include $(UDIR)/programs/Makefile
include d/Makefile
