UPROGS_DIR = $(UDIR)/programs
UPROGS_SRC = $(wildcard $(UPROGS_DIR)/*.c)
UPROGS_OBJ = $(UPROGS_SRC:$(UPROGS_DIR)/%.c=$(BIN)/%.o)
UPROGS = $(UPROGS_OBJ:$(BIN)/%.o=$(SYSROOT)/$(BIN)/_%)

define rust_libs
$(foreach x, $(1) $(2) $(3) $(4) $(5) $(6) $(7) $(8) $(9) $(10), $(FULL_BIN)/lib$(x).a)
endef

define _
	$(CC) $(CFLAGS) $(UCFLAGS) $(LINKER_FLAGS) -Wl,-z,noexecstack -Wl,-O1 -Wl,-N,-e,_start -Wl,--section-start=.text=0x1000 -o $@ $^
	$(OBJCOPY) --remove-section .note.gnu.property $@
endef

$(BIN)/%.o: $(UPROGS_DIR)/%.c
	$(CC) $(CFLAGS) -Wno-error $(UCFLAGS) -c -o $@ $^

ULIBS = $(ULIB_STATIC) $(SHAREDLIBS_STATIC)

define prog
$(SYSROOT)/$(BIN)/_$(1): $(BIN)/$(1).o
endef

$(call prog,lspci) $(ULIBS) $(call rust_libs,pci,gui)
	$(call _)

$(call prog,draw) $(ULIBS) $(call rust_libs,gui)
	$(call _)

$(call prog,fps) $(ULIBS) $(call rust_libs,gui)
	$(call _)

# Generic recipe for those who do not require complicated and expensive linkage.
# link together, then remove the .note.gnu.property that GCC generates.
$(SYSROOT)/$(BIN)/_%: $(BIN)/%.o $(ULIB_STATIC) $(SHAREDLIBS_STATIC)
	$(CC) $(CFLAGS) $(UCFLAGS) $(LINKER_FLAGS) -Wl,-z,noexecstack -Wl,-O1 -Wl,-N,-e,_start -Wl,--section-start=.text=0x1000 -o $@ $^
	$(OBJCOPY) --remove-section .note.gnu.property $@
