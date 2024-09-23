module libkant;
// ...because everyone deserves the truth.

// act out of virtue, not of inclination.

// something exists, even without senses.
// a feeling of self and mind, with minimal body.

version (__KERNEL__) {
	import kobject : kwrite, kwriteln;
	alias write = kwrite;
	alias writeln = kwriteln;
} else {}

void kant(alias Fn, Args...)(Args args, string file = __FILE__, size_t line = __LINE__) {
	write(file, "(", line, "): ",  __traits(fullyQualifiedName, Fn), "(");
	write(args[0]);
	static foreach (arg; args[1..$])
		write(", ", arg);
	writeln(") = ", Fn(args));
}



unittest { kant!(func)(2, 7); }
