KERNEL_RUST_DIR = $(KERNELDIR)/rust

FULL_BIN = $(realpath $(BIN))
KERNEL_RUST_MODULES = rust_kernel
KERNEL_RUST_OBJ = $(foreach module, $(KERNEL_RUST_MODULES), $(FULL_BIN)/lib$(module).a)

KERNEL_RUST_FLAGS = -Ccode-model=kernel
KERNEL_CARGO_FLAGS = $(CARGO_FLAGS)

$(FULL_BIN)/lib%.a: $(KERNEL_RUST_DIR)/%
	RUSTFLAGS="$(RUSTFLAGS) $(KERNEL_RUST_FLAGS)" $(CARGO) build --manifest-path=$^/Cargo.toml $(KERNEL_CARGO_FLAGS) --artifact-dir=$(FULL_BIN) $(CARGO_RELEASE)

kernel_cargo_clean:
	@for mod in $(KERNEL_RUST_MODULES); do $(CARGO) clean --manifest-path=$(KERNEL_RUST_DIR)/$$mod/Cargo.toml; done
