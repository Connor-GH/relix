#![no_std]
use bindings::console::cprintf;

#[no_mangle]
pub extern "C" fn rust_hello_world() {
    unsafe {
        cprintf(c"Hello world\n".as_ptr());
    }
}
