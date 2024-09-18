module kobject;
import inherit : inherit;
import optional;
import console;


__gshared:
enum Diagnostic {
	Warn = 1,
	Error,
	Note,
}

struct KObject {
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
		else static if (__traits(isRef, arg) && is(typeof(*arg.type) == KObject)) cprintf("%s", arg.toString().ptr);
		else static if (is(typeof(arg) == void *)) cprintf("%p", arg);
	}
	if (seen_diagnostic)
		cprintf("\033b0");
}
extern(C) char *kalloc();
extern(C) void kfree(char *ptr);

struct KArray(T) {
	private:
		T *data;
		size_t size;
	mixin(inherit!"KObject");
	public:
	this(T...)(T args) {
		static assert(args.length >= 1);
		this.size = args.length;
		data = cast(typeof(data))kalloc();
		if (data is null)
			panic("kalloc() failed");

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
		return Some!T(data[index]);
	}

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

extern(C) void cprintf(const char *fmt, ...);
extern(C) int example_kernel_binding() {
	KDevice kd = KDevice("foo");
	KArray!int ka = KArray!int(3, 4, 5);
	kwriteln(Diagnostic.Note, "This object is ", ka);
	kwriteln(Diagnostic.Note, "ka[1] is ", ka.get(1).unwrap());
	kwriteln(Diagnostic.Note, "Dlang kernel systems up and running.");
	kwriteln(Diagnostic.Note, "This object is ", kd, " with name ", kd.name);
	return kd.equals(KDevice("foo"));
}
