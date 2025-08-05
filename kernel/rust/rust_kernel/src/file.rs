use core::ffi::{c_char, c_ssize_t};

use kernel_bindings::bindings::{
    EINVAL, inode, inode_lock, inode_read, inode_unlock, off_t, pipe, piperead,
};

use crate::lock::ScopeGuard;

enum FileType {
    Pipe,
    Inode,
    Fifo,
    None,
}

struct File {
    kind: FileType,
    flags: u32,
    readable: bool,
    writeable: bool,
    pipe: *mut pipe,
    inode: *mut inode,
    off: off_t,
}

impl File {
    /*
    pub fn close(&mut self) -> Option<i32> {}
    pub fn openat(&mut self) -> Option<i32> {}
    pub fn dup(&mut self) -> Option<&mut File> {}
    pub fn stat(&mut self) -> Option<i32> {}
    pub fn write(&mut self) -> Option<i32> {}
    pub fn seek(&mut self) -> Option<i32> {}
    pub fn readlinkat(&mut self) -> Option<i32> {}
    */
    pub fn read(&mut self, buf: &mut [c_char]) -> Result<(), c_ssize_t> {
        if !self.readable {
            return Err(-(EINVAL as isize));
        }
        match self.kind {
            FileType::Pipe | FileType::Fifo => {
                return match unsafe { piperead(self.pipe, buf.as_mut_ptr(), buf.len()) } {
                    val if val != 0 => Err(val),
                    _ => Ok(()),
                };
            }
            FileType::Inode => {
                let s = ScopeGuard::new(
                    || unsafe { inode_lock(self.inode) },
                    || unsafe { inode_unlock(self.inode) },
                );
                let r: c_ssize_t =
                    unsafe { inode_read(self.inode, buf.as_mut_ptr(), self.off, buf.len()) };
                if r > 0 {
                    self.off += r as i64;
                }
                return match r {
                    val if val != 0 => Err(val),
                    _ => Ok(()),
                };
            }
            _ => panic!("File::read: unknown type"),
        }
    }
}
