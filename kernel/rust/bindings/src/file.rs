
pub const DIRSIZ: u32 = 254;
pub const PROT_READ: u32 = 1;
pub const PROT_WRITE: u32 = 2;
pub const MAP_SHARED: u32 = 1;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct stat {
    pub st_dev: ::core::ffi::c_int,
    pub st_ino: u32,
    pub st_nlink: ::core::ffi::c_short,
    pub st_size: u64,
    pub st_mode: u32,
    pub st_uid: u16,
    pub st_gid: u16,
    pub st_ctime: u64,
    pub st_atime: u64,
    pub st_mtime: u64,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct sleeplock {
    pub locked: u32,
    pub lk: spinlock,
    pub name: *mut ::core::ffi::c_char,
    pub pid: ::core::ffi::c_int,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct spinlock {
    pub locked: u32,
    pub name: *mut ::core::ffi::c_char,
    pub cpu: *mut cpu,
    pub pcs: [usize; 10usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct inode {
    pub dev: u32,
    pub inum: u32,
    pub ref_: ::core::ffi::c_int,
    pub lock: sleeplock,
    pub valid: ::core::ffi::c_int,
    pub ctime: u64,
    pub atime: u64,
    pub mtime: u64,
    pub size: u64,
    pub mode: u32,
    pub gid: u16,
    pub uid: u16,
    pub addrs: [u64; 10usize],
    pub major: ::core::ffi::c_short,
    pub minor: ::core::ffi::c_short,
    pub nlink: ::core::ffi::c_short,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct dinode {
    pub ctime: u64,
    pub atime: u64,
    pub mtime: u64,
    pub size: u64,
    pub mode: u32,
    pub gid: u16,
    pub uid: u16,
    pub addrs: [u64; 10usize],
    pub major: ::core::ffi::c_short,
    pub minor: ::core::ffi::c_short,
    pub nlink: ::core::ffi::c_short,
}
unsafe extern "C" {
    pub fn inode_alloc(arg1: u32, arg2: i32) -> *mut inode;
    pub fn inode_dup(arg1: *mut inode) -> *mut inode;
    pub fn inode_init(dev: ::core::ffi::c_int);
    pub fn inode_lock(arg1: *mut inode);
    pub fn inode_put(arg1: *mut inode);
    pub fn inode_unlock(arg1: *mut inode);
    pub fn inode_unlockput(arg1: *mut inode);
    pub fn inode_update(arg1: *mut inode);
    pub fn namei(arg1: *const ::core::ffi::c_char) -> *mut inode;
    pub fn nameiparent(
        arg1: *const ::core::ffi::c_char,
        arg2: *mut ::core::ffi::c_char,
    ) -> *mut inode;
    pub fn inode_read(
        arg1: *mut inode,
        arg2: *mut ::core::ffi::c_char,
        arg3: u64,
        arg4: u64,
    ) -> ::core::ffi::c_int;
    pub fn inode_stat(arg1: *mut inode, arg2: *mut stat);
    pub fn inode_write(
        arg1: *mut inode,
        arg2: *mut ::core::ffi::c_char,
        arg3: u64,
        arg4: u64,
    ) -> ::core::ffi::c_int;
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct mmap_info {
    pub length: usize,
    pub addr: usize,
    pub virt_addr: usize,
    pub file: *mut file,
}
#[repr(C)]
pub struct pipe {
    _address: u8,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct file {
    pub type_: file__bindgen_ty_1,
    pub ref_: ::core::ffi::c_int,
    pub readable: ::core::ffi::c_char,
    pub writable: ::core::ffi::c_char,
    pub pipe: *mut pipe,
    pub ip: *mut inode,
    pub off: u32,
}
pub const file_FD_NONE: file__bindgen_ty_1 = 0;
pub const file_FD_PIPE: file__bindgen_ty_1 = 1;
pub const file_FD_INODE: file__bindgen_ty_1 = 2;
pub type file__bindgen_ty_1 = ::core::ffi::c_uint;

pub const CONSOLE: _bindgen_ty_1 = 1;
pub const NULLDRV: _bindgen_ty_1 = 2;
pub const FB: _bindgen_ty_1 = 3;
pub type _bindgen_ty_1 = ::core::ffi::c_uint;

unsafe extern "C" {
    pub fn filealloc() -> *mut file;
    pub fn fileclose(arg1: *mut file);
    pub fn fileopen(
        path: *mut ::core::ffi::c_char,
        omode: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
    pub fn filedup(arg1: *mut file) -> *mut file;
    pub fn fileinit();
    pub fn fileread(
        arg1: *mut file,
        arg2: *mut ::core::ffi::c_char,
        n: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
    pub fn filestat(arg1: *mut file, arg2: *mut stat) -> ::core::ffi::c_int;
    pub fn filewrite(
        arg1: *mut file,
        arg2: *mut ::core::ffi::c_char,
        n: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
    pub fn fileseek(
        f: *mut file,
        n: ::core::ffi::c_int,
        whence: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;

    pub fn fd_to_struct_file(fd: ::core::ffi::c_int) -> *mut file;
}
#[repr(C)]
pub struct cpu {
    _address: u8,
}

pub const O_RDONLY: i32 = 0;
pub const O_WRONLY: i32 = 1;
pub const O_RDWR: i32 = 2;
pub const O_CREATE: i32 = 512;
pub const O_APPEND: i32 = 1024;
