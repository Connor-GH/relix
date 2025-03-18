#include "fs.h"
#include "spinlock.h"
#include "file.h"

static ssize_t
nulldrvwrite(short minor, struct inode *ip, char *buf, size_t n)
{
	return n;
}

static ssize_t
nulldrvread(short minor, struct inode *ip, char *buf, size_t n)
{
	return 0;
}

static struct mmap_info
nulldrvmmap_noop(short minor, size_t length, uintptr_t addr)
{
	return (struct mmap_info){};
}

static int
nulldrvopen_noop(short minor, int flags)
{
	return 0;
}

static int
nulldrvclose_noop(short minor)
{
	return 0;
}

void
nulldrvinit(void)
{
	devsw[NULLDRV].write = nulldrvwrite;
	devsw[NULLDRV].read = nulldrvread;
	devsw[NULLDRV].mmap = nulldrvmmap_noop;
	devsw[NULLDRV].open = nulldrvopen_noop;
	devsw[NULLDRV].close = nulldrvclose_noop;
}
