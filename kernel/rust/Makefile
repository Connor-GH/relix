KERNEL_RUST_DIR = $(KERNELDIR)/rust

FULL_BIN = $(realpath $(BIN))
KERNEL_RUST_MODULES = kernel_binding
RUST_OBJ = $(foreach module, $(KERNEL_RUST_MODULES), $(FULL_BIN)/rs_$(module).a)


$(RUST_OBJ): $(KERNEL_RUST_DIR)/src/lib.rs
	cd $(KERNEL_RUST_DIR); \
	cargo build -Z build-std=core --target i686-unknown-none.json; \
	cp target/i686-unknown-none/debug/libkernel_binding.a $@; \
	cd -

