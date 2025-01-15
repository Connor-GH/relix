KERNEL_RUST_DIR = $(KERNELDIR)/rust

FULL_BIN = $(realpath $(BIN))
KERNEL_RUST_MODULES = kernel_example
RUST_OBJ = $(foreach module, $(KERNEL_RUST_MODULES), $(FULL_BIN)/lib$(module).a)

CARGO_FLAGS = -Zunstable-options --target=x86_64-unknown-none

$(KERNEL_RUST_MODULES):
	@for mod in $(KERNEL_RUST_MODULES); do RUSTFLAGS="$(RUSTFLAGS)" $(CARGO) build --manifest-path=$(KERNEL_RUST_DIR)/$$mod/Cargo.toml $(CARGO_FLAGS) --artifact-dir=$(FULL_BIN) $(CARGO_RELEASE); done

cargo_clean:
	@for mod in $(KERNEL_RUST_MODULES); do $(CARGO) clean --manifest-path=$(KERNEL_RUST_DIR)/$$mod/Cargo.toml; done
