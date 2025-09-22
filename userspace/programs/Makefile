UPROGS_DIR := $(UDIR)/programs
UPROGS_SRC := $(wildcard $(UPROGS_DIR)/*.c)
UPROGS_OBJ := $(UPROGS_SRC:$(UPROGS_DIR)/%.c=$(BIN)/%.o)
UPROGS := $(UPROGS_OBJ:$(BIN)/%.o=$(SYSROOT)/$(BIN)/%)

define rust_libs
$(foreach x, $(1) $(2) $(3) $(4) $(5) $(6) $(7) $(8) $(9) $(10), $(FULL_BIN)/lib$(x).a)
endef

define _
	$(CC) $(CFLAGS) $(UCFLAGS) $(LINKER_FLAGS) -Wl,-z,noexecstack -Wl,-O1 -Wl,-N,-e,_start -Wl,--section-start=.text=0x1000 -o $@ $^
	$(OBJCOPY) --remove-section .note.gnu.* $@
endef

$(BIN)/%.o: $(UPROGS_DIR)/%.c
	$(CC) $(CFLAGS) $(UCFLAGS) -c -o $@ $^

ULIBS := $(LIBC_STATIC) $(LIBSHARED_STATIC) $(LIBM_STATIC)

define prog
$(SYSROOT)/$(BIN)/$(1): $(BIN)/$(1).o
endef

$(call prog,lspci) $(call rust_libs,pci,gui) $(ULIBS)
	$(call _)

$(call prog,draw) $(call rust_libs,gui) $(ULIBS)
	$(call _)

$(call prog,fps) $(call rust_libs,gui) $(ULIBS)
	$(call _)

# Generic recipe for those who do not require complicated and expensive linkage.
# link together, then remove the .note.gnu.property that GCC generates.
$(SYSROOT)/$(BIN)/%: $(BIN)/%.o $(LIBC_STATIC) $(LIBSHARED_STATIC) $(LIBM_STATIC)
	$(CC) $(CFLAGS) $(UCFLAGS) $(LINKER_FLAGS) -Wl,-z,noexecstack -Wl,-O1 -Wl,-N,-e,_start -Wl,--section-start=.text=0x1000 -o $@ $^
	$(OBJCOPY) --remove-section .note.gnu.* $@
