# Device documentation

Relix supports device files (`S_IFBLK` and `S_IFCHR`). Normally, you know of these files as the ones that reside in `/dev`. In Relix, you can interface with several devices with the typical VFS syscalls. One such example is `/dev/null`: whatever you write to it gets swallowed, and you cannot read from it.

# Device tutorial

NOTE: kernel APIs are subject to change at any time.

Suppose you want to create a device in the kernel, called `/dev/four` that [always returns a random number chosen by a fair dice roll](https://xkcd.com/221/).

First, you need to tell the device VFS handler system about your device. Open `kernel/include/file.h` and modify the contents of this enum:
```
enum {
	// The system console for kernel logging. /dev/console
	CONSOLE = 1,
	// /dev/null
	NULLDRV = 2,
    // ...
    DEVFOUR = /* some unique number to represent the device */,
    __DEVSW_last,
};
```

Next, create a new file called `four.c` in `kernel/drivers/`. Inside of it, you will put these contents:
```
static ssize_t
dev_four_write(short minor, struct inode *ip, char *buf, size_t n)
{
	return n;
}

static ssize_t
dev_four_read(short minor, struct inode *ip, char *buf, size_t n)
{
	return n;
}

static struct mmap_info
dev_four_mmap(short minor, size_t length, uintptr_t addr, int perm)
{
	return (struct mmap_info){};
}

static int
dev_four_open(short minor, int flags)
{
	return 0;
}

static int
dev_four_close(short minor)
{
	return 0;
}

void
dev_four_init(void)
{
	devsw[DEVFOUR].write = dev_four_write;
	devsw[DEVFOUR].read = dev_four_read;
	devsw[DEVFOUR].mmap = dev_four_mmap;
	devsw[DEVFOUR].open = dev_four_open;
	devsw[DEVFOUR].close = dev_four_close;
}

```
Inside of `kernel/drivers/four.h`:
```
#pragma once
void dev_four_init(void);
```

You also need to initialize your device. Put these lines in `kernel/main.c`, next to the ones like it:
```
...
#include "drivers/four.h"
...
dev_four_init();
```

That's it for the work needed in the kernel. Now, you need to create the device with `mknod()` in userspace, specifically in `init`.
After the one for `/dev/null`, add some code like this:
```
make_file_device("/dev/four", makedev(/* device number from above */, 0), O_RDWR);
```

After all of that is done, you will now have a device show up when you run `ls -l /dev/`. Congratulations!
