USER_RUST_DIR = $(UDIR)/rust

FULL_BIN = $(realpath $(BIN))
USER_RUST_MODULES = gui pci
USER_RUST_OBJ = $(foreach module, $(USER_RUST_MODULES), $(FULL_BIN)/lib$(module).a)

USER_RUST_FLAGS = -Ctarget-feature=+sse,+sse2
ifneq ($(RELEASE),)
  ifeq ($(LLVM),1)
	  USER_RUST_FLAGS += -Clinker-plugin-lto -Clinker-flavor=ld.lld
  endif
	USER_RUST_FLAGS += -Copt-level=3
endif

CARGO_FLAGS = -Zunstable-options --target=x86_64-unknown-none

$(USER_RUST_OBJ):
	@for mod in $(USER_RUST_MODULES); do RUSTFLAGS="$(RUSTFLAGS) $(USER_RUST_FLAGS)" $(CARGO) build --manifest-path=$(USER_RUST_DIR)/$$mod/Cargo.toml $(CARGO_FLAGS) --artifact-dir=$(FULL_BIN) $(CARGO_RELEASE); done

user_cargo_clean:
	@for mod in $(USER_RUST_MODULES); do $(CARGO) clean --manifest-path=$(USER_RUST_DIR)/$$mod/Cargo.toml; done
