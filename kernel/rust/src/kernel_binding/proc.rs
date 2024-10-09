use core::ffi::{c_char, c_void};
use super::mmu::*;
use crate::kernel_binding::param;
use super::x86::TrapFrame;
pub struct Inode;
#[repr(C)]
pub struct Groups {
    pub gr_name: *mut c_char,
    pub gr_passwd: *mut c_char,
    pub gr_gid: u32,
    pub gr_mem: *mut *mut c_char,
}

#[repr(C)]
pub struct Cred {
    pub uid: u32,
    pub gid: u32,
    pub groups: [Groups; param::MAXGROUPS],
}

#[repr(C)]
pub struct Context {
    pub edi: u32,
    pub esi: u32,
    pub ebx: u32,
    pub ebp: u32,
    pub eip: u32,
}

#[repr(C)]
pub struct Proc {
    pub sz: usize,       // Size of process memory (bytes)
    pub pgdir: *mut i32, // Page table
    pub kstack: *mut u8, // Bottom of kernel stack for this process
    pub state: u32,      // Process state
    pub pid: u32,        // Process ID
    pub status: i32,
    pub parent: *mut Proc,                    // Parent process
    pub tf: *mut TrapFrame,                   // Trap frame for current syscall
    pub context: *mut Context,                // Context to switch to
    pub chan: *mut c_void,                    // If non-zero, sleeping on chan
    pub killed: i32,                          // If non-zero, have been killed
    pub ofile: [*mut c_void; param::NOFILE],  // Open files
    pub cwd: *mut Inode,                      // Current directory
    pub cred: Cred,                           // User's credentials for the process
    pub name: [u8; 16],                       // Process name (for debugging)
    pub strace_mask_ptr: [u8; param::MAXARG], // Mask for tracing syscalls
}

#[repr(C)]
pub struct Cpu {
    apicid: u8,                            // Local APIC ID
    scheduler: *mut Context,               // Pointer to the scheduler context (unsafe)
    ts: TaskState,                         // Used by x86 to find stack for interrupt
    gdt: [SegDesc; NSEGS],                 // x86 global descriptor table
    started: core::sync::atomic::AtomicU32, // Has the CPU started? Using atomic for concurrency safety
    ncli: i32,                             // Depth of pushcli nesting
    intena: i32,                           // Were interrupts enabled before pushcli?
    proc: *mut Proc, // The process running on this CPU or null (use Option for nullability)
}
