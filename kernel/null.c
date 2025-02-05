#include "fs.h"
#include "spinlock.h"
#include "file.h"

struct {
	struct spinlock lock;
	int locking;
} nulldrv;

int
nulldrvwrite(struct inode *ip, char *buf, int n)
{
	inode_unlock(ip);
	acquire(&nulldrv.lock);
	release(&nulldrv.lock);
	inode_lock(ip);
	return n;
}

int
nulldrvread(struct inode *ip, char *buf, int n)
{
	inode_unlock(ip);
	acquire(&nulldrv.lock);
	release(&nulldrv.lock);
	inode_lock(ip);
	return 0;
}

static struct mmap_info
nulldrvmmap_noop(size_t length, uintptr_t addr)
{
	return (struct mmap_info){};
}
void
nulldrvinit(void)
{
	initlock(&nulldrv.lock, "nulldrv");
	devsw[NULLDRV].write = nulldrvwrite;
	devsw[NULLDRV].read = nulldrvread;
	devsw[NULLDRV].mmap = nulldrvmmap_noop;
	nulldrv.locking = 1;
}
