module memory;
import traits;
import kalloc : kmalloc, kfree;

T *d_new(T, Args...)(auto ref Args args) {
	import core.lifetime : emplace, forward;
	T *mem = cast(T *)kmalloc(T.sizeof);
	return mem.emplace(forward!args);
}
void d_delete(T)(T *data) {
	if (data is null)
		return;
	kfree(data);
}
