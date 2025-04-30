# Notes on style
We use the Linux/coreboot code style with a few tweaks:
- Checking for null pointers should be ``ptr == NULL``, not ``!ptr``. This applies for integers and pointers, but not for bools.
- We use ``#pragma once`` where we can. Some exceptions might be in libc for POSIX reasons.
- Rust should avoid ``Arc`` and ``Rc`` as much as it should.
- Annotate functions that acquire/release locks with ``__acquires(lockname)`` and ``__releases(lockname)``.
- After initial fixes, run your code under ``make format``. If it does something weird, turn it off for that section with ``/* clang-format off */``. Similarly, run ``cargo fmt`` in the root of Rust projects.
