LIBCDIR = $(UDIR)/libc
LIBC_SRC := $(wildcard $(LIBCDIR)/*.c)
LIBC_OBJ := $(LIBC_SRC:$(LIBCDIR)/%.c=$(BIN)/libc_%.o)
LIBC_ASM := $(wildcard $(LIBCDIR)/*.S)
LIBC_ASM_OBJ := $(LIBC_ASM:$(LIBCDIR)/%.S=$(BIN)/asm_%.o)

LIBC_STATIC := $(BIN)/libc.a

LIBC_CFLAGS := $(UCFLAGS) -I$(LIBCDIR)/private

$(LIBC_OBJ): $(BIN)/libc_%.o : $(LIBCDIR)/%.c
	$(CC) $(CFLAGS) $(LIBC_CFLAGS) -c -o $@ $^

$(LIBC_ASM_OBJ): $(BIN)/asm_%.o : $(LIBCDIR)/%.S
	$(CC) $(CFLAGS) $(LIBC_CFLAGS) -c -o $@ $^

$(LIBC_STATIC): $(LIBC_OBJ) $(LIBC_ASM_OBJ)
	$(AR) -r $@ $^
	$(RANLIB) $@
