use core::ffi::{c_void, c_int, c_char};

unsafe extern "C" {
    pub fn strcpy(dest: *mut c_char, src: *const c_char) -> *mut c_char;
    pub fn strchr(s: *const c_char, c: c_int) -> *mut c_char;
    pub fn strcat(dest: *mut c_char, src: *const c_char) -> *mut c_char;
    pub fn strcmp(s1: *const c_char, s2: *const c_char) -> c_int;
    pub fn strncmp(s1: *const c_char, s2: *const c_char, n: usize) -> c_int;
    pub fn strncpy(dest: *mut c_char, src: *const c_char, n: usize) -> *mut c_char;
    pub fn strlen(s: *const c_char) -> usize;
    pub fn strrchr(s: *const c_char, c: c_int) -> *mut c_char;
    pub fn memset(s: *mut c_void, c: c_int, n: usize) -> *mut c_void;
    pub fn memcmp(s1: *const c_void, s2: *const c_void, n: usize) -> c_int;
    pub fn memmove(dest: *mut c_void, src: *const c_void, n: usize) -> *mut c_void;
    pub fn memcpy(dest: *mut c_void, src: *const c_void, n: usize) -> *mut c_void;
    pub fn strerror(errnum: c_int) -> *mut c_char;
    pub fn strtok(s: *mut c_char, delim: *const c_char) -> *mut c_char;
    pub fn strdup(s: *const c_char) -> *mut c_char;
}
