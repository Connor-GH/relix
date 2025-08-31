# VFS documentation

Relix is like any other UNIX-like kernel in this sense: it thinks of
files as inodes which are backed by disk inodes (dinodes).

Below is a demonstration of what happens when you call `open()`:

```
open(filename, O_RDONLY);
|
*-> openat(AT_FDCWD, ...);
    |
    [syscall]
    |
    *-> sys_openat(...)
        |
        *-> vfs_openat(...)
               |
           ip = resolve_filename(filename);
           // ...
           switch (ip->type) {
           case dir:
               dir_open();
               break;
           case fifo:
           case pipe:
               pipe_open();
               break;
           case device:
               dev_open();
               break;
           default:
               break;
           }
           f = filealloc();
           fd = fdalloc(f);
           // ...
           return fd;
```

As you can see, `open()` is just a call to `openat()`. `openat()` is just `open()` with an argument for a directory file descriptor (`dirfd`) and another argument for flags such as `AT_NOFOLLOW`. We then go through a syscall and go into the kernel. The `openat()` implementation is called `sys_openat()` in the kernel. After validating its arguments, it immediately calls `vfs_openat()`, which does all of the real work. All of the code examples are summaries of what is in `vfs_openat()`. Depending on the type of file that `filename` happens to be after canonicalizing the path and resolving symlinks, we do different things. After the filetype-specific code is done, we make space for the kernel-side `file` object for accounting, and then allocate a file descriptor for the program to use.

The process is similar for every other VFS function. VFS functions are `open()`, `close()`, `read()`, `write()`, `stat()`, and `mmap()`. This means that any given device could do exotic things in any of these functions. For example, `/dev/fb0` has a special `mmap()` implementation that returns the hardware MMIO-mapped memory, which doesn't actually allocate any memory. This is conceptually okay since `mmap()`'s job is just to map a file into memory.

In terms of abstractions, every `struct file` has a `struct inode *ip`. Some `struct file`s have a `struct pipe` if the filetype is pipe or FIFO. Every `struct inode` contains all of the information needed to `stat()` a file, and also is a superset of `struct dinode` since certain inode characteristics are only needed at runtime and are not needed when loading from the disk. Every `struct dinode` contains several `uintptr_t addr`s, which are either direct or indirect addresses to disk blocks. Everything is stored in disk blocks, so `sizeof(struct dinode)` must perfectly divide `BLOCK_SIZE`. Once we need a block, we then ask the IDE or SATA driver. Congratulations, you now have a file from disk!

Since Relix only supports RelixFS right now, the code paths for regular files directly enter the block layer for the disk.
