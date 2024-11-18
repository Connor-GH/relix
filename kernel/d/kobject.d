module kobject;
import inherit : inherit;
import unique_ptr;
import memory;
import kernel_optional;
import libkant;
import traits;
import console;
import kernel_string;
import kalloc : kmalloc, kfree;
import kernel_assert : kernel_assert;

__gshared:
enum Diagnostic {
	Warn = 1,
	Error,
	Note,
}

@safe struct KObject {
	public:
	typeof(this) *type;
	bool equals(KObject o) {
		return o == this;
	}
	string toString() const {
		return "KObject";
	}
}

struct KDevice {
	private:
		string name;
	this(string n) {
		this.name = n;
	}
	mixin(inherit!"KObject");

	public bool equals(KDevice o) {
		return this.name == o.name;
	}
	public string toString() const {
		return __traits(identifier, typeof(this));
	}
}

void kwrite(S...)(auto ref S args) {
	bool seen_diagnostic = false;
	static foreach (arg; args) {
		static if (is(typeof(arg) == Diagnostic)) {
			seen_diagnostic = true;
			with(Diagnostic) switch (arg) {
			case Warn: cprintf("\033b6warn: "); break;
			case Error: cprintf("\033b4error: "); break;
			case Note: cprintf("\033b1note: "); break;
			default: break;
			}
		}
		else static if (is(typeof(arg) == int)) cprintf("%d", arg);
		else static if (is(typeof(arg) == uint)) cprintf("%u", arg);
		else static if (is(typeof(arg) == bool)) {
			// hack to get around messy behavior with ternary operator
			if (arg == true)
				cprintf("true");
			else
				cprintf("false");
		}
		else static if (__traits(isArithmetic, arg)) cprintf("%u", arg);
		else static if (is(typeof(arg) == string)) cprintf("%s", arg.ptr);
		else static if (is(typeof(arg) == char *)) cprintf("%s", arg);
		else static if (__traits(isRef, arg) && is(typeof(*arg.type) == KObject)) cprintf("%s", arg.toString().ptr);
		else static if (is(typeof(arg) == void *)) cprintf("%p", arg);
	}
	if (seen_diagnostic)
		cprintf("\033b0");
}

struct KArray(T) {
	private:
		T *data;
		size_t size;
	mixin(inherit!"KObject");
	public:
	this(T...)(T args) {
		static assert(args.length >= 1);
		this.size = args.length;
		data = cast(typeof(data))kmalloc(args.length * args[0].sizeof);
		if (data is null)
			panic("kmalloc() failed");

		foreach (size_t i, arg; args) {
			this.data[i] = arg;
		}
	}
	this(KArray other) {
		if (this.data !is null)
			kfree(cast(char *)this.data);
		this.data = other.data;
		this.size = other.size;
	}

	~this() { kfree(cast(char *)data); }

	Option!T get(size_t index) {
		if (data is null)
			panic("data is null");
		if (size <= index)
			panic("index too large");
		return some!T(data[index]);
	}

	Option!T opIndex(size_t index) => get(index);

	bool equals(KArray other) {
		if (this.size != other.size)
			return false;
		for (int i = 0; i < other.size; i++)
			if (this.data[i] != other.data[i])
				return false;
		return true;
	}
	string toString() const {
		return __traits(identifier, typeof(this));
	}
}

void kwriteln(S...)(auto ref S args) => kwrite(args, "\n");

enum NULL = cast(void *)0;

@safe struct KNonNull(T) {
	private T ptr;
	public:
	this(T)(T data)
	if (isPointer!T && !is(T == typeof(null)))
	{
		assert(data != NULL);
 		ptr = data;
	}

	void opAssign(T)(T other)
	if (isPointer!T && !is(T == typeof(null)))
	{
		assert(other != NULL);
 		this.ptr = other;
	}
	Option!T release_ptr() => ptr != NULL ? some!T(ptr) : none!T;
}

extern(C) int example_kernel_binding() {
	KDevice kd = KDevice("foo");
	KArray!int ka = KArray!int(3, 4, 5);
	// uncomment to allow NonNull container to give error
	//Option!(KNonNull!(char *)) i_am_null = some(KNonNull!(char *)(cast(char *)null));

	int myfunc(int a) => a * a;

	auto c = KNonNull!(void *)(kmalloc(1));
	scope(exit) kfree(c.release_ptr().unwrap());

	auto d = make_unique!(KArray!int)(3, 4, 5);
	assert(isPointer!(typeof(d.get())) && ((*d.get())[1]).unwrap() == 4);
	assert(isPointer!(typeof(d.get())) && ((*d.get())[0]).unwrap() == 3);


	assert(ka.toString() == "KArray");
	assert(ka[1].unwrap() == 4);
	assert(kd.toString() == "KDevice");
	kant!(myfunc)(9);
	kwriteln(Diagnostic.Note, "Dlang kernel systems up and running.");
	return 0;
}
