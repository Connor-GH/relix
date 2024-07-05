UDIR = userspace
ULIB = $(wildcard $(UDIR)/*.c)
ULIB_OBJ = $(ULIB:$(UDIR)/%.c=$(BIN)/%.o)
ULIB_ASM = $(wildcard $(UDIR)/*.S)
ULIB_ASM_OBJ = $(ULIB_ASM:$(UDIR)/%.S=$(BIN)/%.o)

UCFLAGS = -flto -I$(UDIR)/include -I. $(IVARS)

$(ULIB_OBJ): $(BIN)/%.o : $(UDIR)/%.c
	$(CC) $(CFLAGS) $(UCFLAGS) -c -o $@ $^

$(ULIB_ASM_OBJ): $(BIN)/%.o : $(UDIR)/%.S
	$(CC) $(CFLAGS) $(UCFLAGS) -c -o $@ $^

include $(UDIR)/programs/Makefile
include d/Makefile
