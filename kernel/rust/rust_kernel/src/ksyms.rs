use bindings::file::{fileopen, fileclose, fileread, O_RDONLY, file};
use alloc::vec::Vec;

struct FileDesc(i32);
impl FileDesc {
    fn to_file(&self) -> *mut file {
        unsafe { bindings::file::fd_to_struct_file(self.0) }
    }
}
struct File {
    file_ptr: *mut file,
}
impl File {
    fn read(&mut self, into: &mut [core::ffi::c_char]) -> Self {
        unsafe { fileread(self.file_ptr, into.as_mut_ptr(), into.len() as i32) };
        File { file_ptr: self.file_ptr }
    }
    fn open(path: *mut core::ffi::c_char, omode: i32) -> Option<Self> {
        let ret: FileDesc = FileDesc { 0: unsafe { fileopen(path, omode) } };
        if ret.0 < 0 {
            None
        } else {
            Some(File { file_ptr: ret.to_file() } )
        }
    }
}
impl Drop for File {
    fn drop(&mut self) {
        unsafe { fileclose(self.file_ptr) };
    }
}


#[unsafe(no_mangle)]
pub extern "C" fn read_kernel_symbols() {
    let path = c"/etc/ksyms.map".as_ptr() as *mut _;
    let mut file = File::open(path, O_RDONLY).expect("File did not open");
    let buf: &mut [core::ffi::c_char; 100] = &mut [0; 100];
    file.read(buf);
    let vec: Vec<u8> = buf.iter().map(|c| *c as u8).collect();
    let str_buf: &str = core::str::from_utf8(vec.as_slice()).expect("Failed to make string");
    let _address = str_buf.trim().parse().unwrap_or(0);



}
