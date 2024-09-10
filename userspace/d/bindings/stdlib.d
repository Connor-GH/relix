module stdlib;

enum NULL = cast(void *)0;

version (__KERNEL__) {} else {
	extern(C) {
		void *
		malloc(uint);
		void
		free(void *);
		int
		atoi(const char *);
		int
		atoi_base(const char *, uint base);
	}
}
