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
	iunlock(ip);
	acquire(&nulldrv.lock);
	release(&nulldrv.lock);
	ilock(ip);
	return n;
}

int
nulldrvread(struct inode *ip, char *buf, int n)
{
	iunlock(ip);
	acquire(&nulldrv.lock);
	release(&nulldrv.lock);
	ilock(ip);
	return 0;
}

void
nulldrvinit(void)
{
	initlock(&nulldrv.lock, "nulldrv");
	devsw[NULLDRV].write = nulldrvwrite;
	devsw[NULLDRV].read = nulldrvread;
	nulldrv.locking = 1;
}
