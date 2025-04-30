# How to Contribute

Prerequisites:
- Have all of the dependencies as listed in the README.
- Knowledge of C, Rust, and OSDev.
- A good text editor. Connor-GH personally uses neovim with the NvChad config and LSPs for C and Rust.


Notes for contributing:
- ``tools/ci.sh`` is our makeshift CI. If you are to contribute, your changes MUST pass OS tests when the system is built in all of these configurations.
- It's probably easier to contribute a program to userspace than it is to contribute a driver for the kernel. In any event though, more kernel infrastructure is more valuable.
