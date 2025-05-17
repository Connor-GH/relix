# relix: xv6 work continued from the MIT PDOS team

xv6 was a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6) for the i386 processor.

# Enchancements from the original XV6:
- pixel graphics support with custom fonts
- fixed broken SMP due to a QEMU regression
- added users, groups, and permissions
- organized files into /etc, /bin, and /dev
- ACPI support
- POSIX alignment and support
- improved and added coreutils
- organized file structure support; kernel and userland has a clear separation.
- syscall fuzzing (in the works)
- Rust language support (look in kernel/rust/ and userspace/rust)
- doubly indirect block pointer inodes (max filesize 1MiB -> 512MiB)
- 64-bit port, code pulled from swetland/xv6
- multiboot2 support
- SATA R/W support
- pwd.h and /etc/passwd

<img src="docs/pictures/relix1.png">
<img src="docs/pictures/relixcolors.png">
<img src="docs/pictures/relix2.png">
# dependencies
- gcc/clang
- ld/lld
- objdump, objcopy, ar, ranlib
- rustc, cargo (nightly)
- qemu
- gmake
- grub-mkrescue
- xorriso

# BUILDING AND RUNNING XV6

run ``make qemu`` with a compiler capable of producing ELF files.

On linux-based systems and FreeBSD systems, this means the native compiler.

Note that ``make qemu -j16`` will parallelize the build.


The password for the `root` and `user` accounts are both `x`. This is temporary until hashes are added in /etc/shadow.

# Oh, and it runs DOOM.
<img src="docs/pictures/doom1.png">
(there is a video in docs/pictures, but github will not embed it)

# TODO
- XHCI USB keyboard support
