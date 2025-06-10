# Notes on style
We use the Linux/coreboot code style with a few tweaks (unless mandated by standards):
- Checking for null pointers should be ``ptr == NULL``, not ``!ptr``. This applies for integers and pointers, but not for bools.
- We use ``#pragma once`` where we can.
- Rust should avoid ``Arc`` and ``Rc`` as much as it can.
- Annotate functions that acquire/release locks with ``__acquires(lockname)`` and ``__releases(lockname)``.
- After initial fixes, run your code under ``make format``. If it does something weird, turn it off for that section with ``/* clang-format off */``. Similarly, run ``cargo fmt`` in the root of Rust projects.
- Always use braces around ``if``, ``while``, ``for``, etc.
