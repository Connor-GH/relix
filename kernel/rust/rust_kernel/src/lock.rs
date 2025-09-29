use core::ffi::CStr;
use core::ops::{Deref, DerefMut, Drop};
use kernel_bindings::bindings::{acquire, release};
use kernel_bindings::bindings::{sleeplock, spinlock};

pub struct SpinLock<T> {
    data: T,
    inner: spinlock,
}

impl<T: Default> SpinLock<T> {
    pub const fn new() -> Self {
        let inner: spinlock = spinlock {
            locked: 0,
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

pub struct ScopeGuard<F>
where
    F: FnOnce(),
{
    fini: Option<F>,
    active: bool,
}

impl<F> ScopeGuard<F>
where
    F: FnOnce(),
{
    pub fn new<I>(init: I, fini: F) -> Self
    where
        I: FnOnce(),
    {
        init();

        ScopeGuard {
            fini: Some(fini),
            active: true,
        }
    }

    pub fn dismiss(mut self) {
        self.active = false;
        self.fini = None;
    }
}

impl<F> Drop for ScopeGuard<F>
where
    F: FnOnce(),
{
    fn drop(&mut self) {
        if self.active {
            if let Some(fini) = self.fini.take() {
                fini();
            }
        }
    }
}
