pub struct ScopeGuard<F2>
where
    F2: FnOnce(),
{
    pub func: Option<F2>,
}

impl<F2> ScopeGuard<F2>
where
    F2: FnOnce(),
{
    pub fn new<F1>(f1: F1, f2: F2) -> Self
    where
        F1: FnOnce(),
    {
        f1();
        ScopeGuard::<F2> { func: Some(f2) }
    }
}

impl<F2> Drop for ScopeGuard<F2>
where
    F2: FnOnce(),
{
    fn drop(&mut self) {
        if let Some(func) = self.func.take() {
            func();
        }
    }
}
