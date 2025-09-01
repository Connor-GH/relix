#include "dev/null.h"

#include "file.h"
#include "fs.h"

static ssize_t
dev_null_write(short minor, struct inode *ip, char *buf, size_t n)
{
	return n;
}

static ssize_t
dev_null_read(short minor, struct inode *ip, char *buf, size_t n)
{
	return n;
}

static struct mmap_info
dev_null_mmap(short minor, size_t length, uintptr_t addr, int perm)
{
	return (struct mmap_info){};
}

static int
dev_null_open(short minor, int flags)
{
	return 0;
}

static int
dev_null_close(short minor)
{
	return 0;
}

void
dev_null_init(void)
{
	devsw[DEV_NULL].write = dev_null_write;
	devsw[DEV_NULL].read = dev_null_read;
	devsw[DEV_NULL].mmap = dev_null_mmap;
	devsw[DEV_NULL].open = dev_null_open;
	devsw[DEV_NULL].close = dev_null_close;
}
