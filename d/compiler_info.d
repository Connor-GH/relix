module compiler_info;

immutable {
	string name = __VENDOR__;
	enum Vendor {
		digitalMars = 1,
		gnu = 2,
		llvm = 3,
		unknown,
	}
	version (DigitalMars) Vendor vendor = Vendor.digitalMars;
	else version (GNU) Vendor vendor = Vendor.gnu;
	else version (LDC) Vendor vendor = Vendor.llvm;
	else Vendor vendor = Vendor.unknown;

	uint version_major = __VERSION__ / 1000;
	uint version_minor = __VERSION__ % 1000;

}

