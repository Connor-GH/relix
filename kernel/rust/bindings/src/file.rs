
pub const S_IFMT: u32 = 61440;
pub const S_IFDIR: u32 = 16384;
pub const S_IFCHR: u32 = 8192;
pub const S_IFBLK: u32 = 24576;
pub const S_IFREG: u32 = 32768;
pub const S_IFIFO: u32 = 4096;
pub const S_IFLNK: u32 = 40960;
pub const S_IFSOCK: u32 = 49152;
pub const S_IRUSR: u32 = 256;
pub const S_IWUSR: u32 = 128;
pub const S_IXUSR: u32 = 64;
pub const S_IRGRP: u32 = 32;
pub const S_IWGRP: u32 = 16;
pub const S_IXGRP: u32 = 8;
pub const S_IROTH: u32 = 4;
pub const S_IWOTH: u32 = 2;
pub const S_IXOTH: u32 = 1;
pub const S_IAUSR: u32 = 448;
pub const S_IAGRP: u32 = 56;
pub const S_IAOTH: u32 = 7;
pub const S_ALLPRIVS: u32 = 511;
pub const DEFAULT_GID: u32 = 0;
pub const DEFAULT_UID: u32 = 0;
pub const __DIRSIZ: u32 = 254;
pub const __NDIRECT: u32 = 8;
pub const __NDINDIRECT_PER_ENTRY: u32 = 8;
pub const __NDINDIRECT_ENTRY: u32 = 8;
pub const __BSIZE: u32 = 2048;
pub const NDIRECT: u32 = 8;
pub const NDINDIRECT_PER_ENTRY: u32 = 8;
pub const NDINDIRECT_ENTRY: u32 = 8;
pub const BSIZE: u32 = 2048;
pub const DIRSIZ: u32 = 254;
pub const BPB: u32 = 16384;
pub const ROOTINO: u32 = 1;
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
