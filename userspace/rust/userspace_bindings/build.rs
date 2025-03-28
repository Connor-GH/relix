use std::path::PathBuf;

fn main() {
    // The bindgen::Builder is the main entry point
    // to bindgen, and lets you build up options for
    // the resulting bindings.
    let bindings = bindgen::Builder::default()
        // The input header we would like to generate
        // bindings for.
        .header("wrapper.h")
        .use_core()
        .constified_enum_module(".*")
        .emit_builtins()
        .derive_default(true)
        .rust_edition(bindgen::RustEdition::Edition2024)
        .override_abi(bindgen::Abi::C, ".*")
        .wrap_unsafe_ops(true)
        .flexarray_dst(true)
        .rust_target(bindgen::RustTarget::nightly())
        .wrap_static_fns(true)
        .translate_enum_integer_types(true)
        .conservative_inline_namespaces()
        .no_hash(".*")
        .formatter(bindgen::Formatter::Rustfmt)
        .enable_function_attribute_detection()

        .clang_args(["-I../../../", "-I../../../include", "-D__USER__=1", "-fno-builtin"])
        // Tell cargo to invalidate the built crate whenever any of the
        // included header files changed.
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        // Finish the builder and generate the bindings.
        .generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from("src");
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
