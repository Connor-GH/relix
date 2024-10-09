KERNEL_RUST_DIR = $(KERNELDIR)/rust

FULL_BIN = $(realpath $(BIN))
KERNEL_RUST_MODULES = kernel_example
RUST_OBJ = $(foreach module, $(KERNEL_RUST_MODULES), $(FULL_BIN)/lib$(module).a)
RUST_SOURCES = $(foreach module, $(KERNEL_RUST_MODULES), $(wildcard $(KERNEL_RUST_DIR)/src/$(module)/*.rs))

CARGO_FLAGS = -Zunstable-options -Z build-std=core -j$(shell nproc) --target=i686-unknown-none.json

$(KERNEL_RUST_MODULES): $(RUST_SOURCES)
	RUSTFLAGS="$(RUSTFLAGS)" cargo build --manifest-path=$(KERNEL_RUST_DIR)/Cargo.toml $(CARGO_FLAGS) --artifact-dir=$(FULL_BIN) $(CARGO_RELEASE)

cargo_clean:
	cargo clean --manifest-path=$(KERNEL_RUST_DIR)/Cargo.toml
