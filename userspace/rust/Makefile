USER_RUST_DIR = $(UDIR)/rust

FULL_BIN = $(realpath $(BIN))
USER_RUST_MODULES = gui pci
UR2 = $(foreach module, $(USER_RUST_MODULES), $(realpath $(USER_RUST_DIR))/$(module))

URUST = $(UR2:$(USER_RUST_DIR)/%=$(FULL_BIN)/lib%.a)

USER_RUST_FLAGS = -Ctarget-feature=+sse,+sse2,-avx,-avx2
ifneq ($(RELEASE),)
  ifeq ($(LLVM),1)
		CLANG_LLVM_MAJOR = $(clang --version | sed -nE "s/clang version \([0-9]+\).\([0-9]+\).\([0-9]+\)/\1/p")
		RUSTC_LLVM_MAJOR = $(shell rustc -vV | sed -nE "s/clang version \([0-9]+\).\([0-9]+\).\([0-9]+\)/\1/p")
		ifeq ($(CLANG_LLVM_MAJOR),$(RUSTC_LLVM_MAJOR))
	  	USER_RUST_FLAGS += -Clinker-plugin-lto -Clinker-flavor=ld.lld
		endif
  endif
	USER_RUST_FLAGS += -Copt-level=3 -Cembed-bitcode=yes -Clto
endif

USER_CARGO_FLAGS = $(CARGO_FLAGS)

$(FULL_BIN)/lib%.a: $(USER_RUST_DIR)/%
	RUSTFLAGS="$(RUSTFLAGS) $(USER_RUST_FLAGS)" $(CARGO) build --manifest-path=$^/Cargo.toml $(USER_CARGO_FLAGS) --artifact-dir=$(FULL_BIN) $(CARGO_RELEASE)

user_cargo_clean:
	@for mod in $(USER_RUST_MODULES); do $(CARGO) clean -p $$mod --manifest-path=$(USER_RUST_DIR)/$$mod/Cargo.toml; done
