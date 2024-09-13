module kobject;
import inherit : inherit;

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
	mixin(inherit!"KObject");

	public bool equals(KDevice o) {
		return this.name == o.name;
	}
	public string toString() const {
		return "KDevice";
	}
}

void kwriteln(S...)(S args) {
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
		else static if (__traits(isArithmetic, arg)) cprintf("%u", arg);
		else static if (is(typeof(arg) == string)) cprintf("%s", arg.ptr);
		else static if (is(typeof(*arg.type) == KObject)) cprintf("%s", arg.toString().ptr);
		else static if (is(typeof(arg) == void *)) cprintf("%p", arg);
	}
	if (seen_diagnostic)
		cprintf("\033b0");
	cprintf("\n");
}

extern(C) void cprintf(const char *fmt, ...);
extern(C) int example_kernel_binding() {
	KDevice kd = KDevice("foo");
	kwriteln(Diagnostic.Note, "Dlang kernel systems up and running.");
	kwriteln(Diagnostic.Note, "This object is ", kd, " with name ", kd.name);
	return kd.equals(KDevice("foo"));
}
