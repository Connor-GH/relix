UDIR = userspace
ULIB = $(wildcard $(UDIR)/*.c)
ULIB_OBJ = $(ULIB:$(UDIR)/%.c=$(BIN)/%.o)
ULIB_ASM = $(wildcard $(UDIR)/*.S)
ULIB_ASM_OBJ = $(ULIB_ASM:$(UDIR)/%.S=$(BIN)/%.o)

ULIB_STATIC = $(BIN)/ulib.a

UCFLAGS = -fno-lto -I$(UDIR)/include -I. $(IVARS)
ifneq ($(RELEASE),)
	UCFLAGS += -flto
endif

$(ULIB_OBJ): $(BIN)/%.o : $(UDIR)/%.c
	$(CC) $(CFLAGS) $(UCFLAGS) -c -o $@ $^

$(ULIB_ASM_OBJ): $(BIN)/%.o : $(UDIR)/%.S
	$(CC) $(CFLAGS) $(UCFLAGS) -c -o $@ $^

$(ULIB_STATIC): $(ULIB_OBJ) $(ULIB_ASM_OBJ)
	$(AR) -r $@ $^
	$(RANLIB) $@

include $(UDIR)/programs/Makefile
include d/Makefile
