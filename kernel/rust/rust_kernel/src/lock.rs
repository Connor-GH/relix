use core::ffi::CStr;
use core::ops::{Deref, DerefMut, Drop};
use kernel_bindings::bindings::{ATOMIC_FLAG_INIT, acquire, release};
use kernel_bindings::bindings::{sleeplock, spinlock};

#[derive(Clone, Copy)]
pub struct SpinLock<T> {
    data: T,
    inner: spinlock,
}

impl<T: Default> SpinLock<T> {
    pub const fn new() -> Self {
        let inner: spinlock = spinlock {
            locked: ATOMIC_FLAG_INIT,
            name: core::ptr::null(),
            cpu: core::ptr::null_mut(),
            pcs: [0; 10],
        };

        Self {
            data: unsafe { core::mem::MaybeUninit::zeroed().assume_init() },
            inner,
        }
    }
    pub const fn with_name(mut self, name: &'static CStr) -> Self {
        self.inner.name = name.as_ptr();
        self
    }

    pub fn lock(mut self) -> LockedSpinLock<T> {
        unsafe {
            acquire(&raw mut self.inner);
        }

        let lock = LockedSpinLock {
            data: self.data,
            inner: self.inner,
        };
        lock
    }
    pub fn guard<F>(&mut self, func: F)
    where
        F: FnOnce(),
    {
        unsafe {
            acquire(&raw mut self.inner);
        }
        func();
        unsafe {
            release(&raw mut self.inner);
        }
    }
}

unsafe impl<T> Sync for SpinLock<T> {}

pub struct LockedSpinLock<T> {
    data: T,
    inner: spinlock,
}

impl<T> LockedSpinLock<T> {
    pub fn get(&self) -> &T {
        &self.data
    }
}

impl<T> Deref for LockedSpinLock<T> {
    type Target = T;
    fn deref(&self) -> &Self::Target {
        &self.data
    }
}

impl<T> DerefMut for LockedSpinLock<T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.data
    }
}

impl<T> Drop for LockedSpinLock<T> {
    fn drop(&mut self) {
        unsafe {
            release(&raw mut self.inner);
        }
    }
}
pub struct SleepLock;
