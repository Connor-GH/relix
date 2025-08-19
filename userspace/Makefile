UDIR = userspace
ULIB = $(wildcard $(UDIR)/*.c)
ULIB_OBJ = $(ULIB:$(UDIR)/%.c=$(BIN)/ulib_%.o)
ULIB_ASM = $(wildcard $(UDIR)/*.S)
ULIB_ASM_OBJ = $(ULIB_ASM:$(UDIR)/%.S=$(BIN)/asm_%.o)

USER_EXTRAS = -D__USER__

ULIB_STATIC = $(BIN)/libc.a

UCFLAGS = -fno-lto -m80387 -msse -msse2 \
					-mno-avx -mno-avx2 $(USER_EXTRAS) -I$(UDIR)/include -I. $(IVARS)
ifneq ($(RELEASE),)
	UCFLAGS += -flto -O3
else
	UCFLAGS += -DMEMORY_GUARDS
endif

$(ULIB_OBJ): $(BIN)/ulib_%.o : $(UDIR)/%.c
	$(CC) $(CFLAGS) $(UCFLAGS) -c -o $@ $^

$(ULIB_ASM_OBJ): $(BIN)/asm_%.o : $(UDIR)/%.S
	$(CC) $(CFLAGS) $(UCFLAGS) -c -o $@ $^

$(ULIB_STATIC): $(ULIB_OBJ) $(ULIB_ASM_OBJ)
	$(AR) -r $@ $^
	$(RANLIB) $@

include $(UDIR)/rust/Makefile
include $(UDIR)/programs/Makefile
