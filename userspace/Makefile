UDIR = userspace
ULIB = $(wildcard $(UDIR)/*.c)
ULIB_OBJ = $(ULIB:$(UDIR)/%.c=$(BIN)/%.o)
ULIB_ASM = $(wildcard $(UDIR)/*.S)
ULIB_ASM_OBJ = $(ULIB_ASM:$(UDIR)/%.S=$(BIN)/%.o)

$(ULIB_OBJ): $(BIN)/%.o : $(UDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^
	objcopy --remove-section .note.gnu.property $@

$(ULIB_ASM_OBJ): $(BIN)/%.o : $(UDIR)/%.S
	$(CC) $(CFLAGS) -c -o $@ $^
	objcopy --remove-section .note.gnu.property $@

include $(UDIR)/programs/Makefile
