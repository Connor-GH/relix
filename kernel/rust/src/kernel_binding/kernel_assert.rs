use core::ffi::c_char;
extern "C" {
    pub fn kernel_assert_fail(
    assertion: *const c_char,
    file: *const c_char,
    lineno: i32,
    func: *const c_char,
    ) -> !;
}

pub fn kernel_assert(
    expr: bool,
    expression: *const c_char,
    file: *const c_char,
    line: i32,
    func: *const c_char,
) {
    if !expr {
        unsafe {
            kernel_assert_fail(expression, file, line, func);
        }
    }
}
