module unique_ptr;
import memory;
import traits;
import kernel_string;
import core.lifetime : forward;

struct KUniquePtr(T) {
	alias TPtr = toPointer!T;
	private:
		TPtr data;

	public:
	@disable this();
	this(TPtr ptr) {
		data = ptr;
	}

	~this() {
		d_delete(data);
	}

	inout(TPtr) get() inout {
		return data;
	}

}

KUniquePtr!T make_unique(T, A...)(auto ref A args) {
	return KUniquePtr!T(d_new!T(forward!args));
}
