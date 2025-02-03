#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#define USE_HOST_TOOLS
#include <time.h>
#include <stdint.h>
#define SYSROOT "sysroot/"

#define stat xv6_stat // avoid clash with host struct stat
#define USE_HOST_TOOLS
#include "../kernel/include/fs.h"
#include "../include/stat.h"
#include "../include/dirent.h"
#include "../kernel/include/param.h"

#ifndef static_assert
#define static_assert(a, b) \
	do {                      \
		switch (0)              \
		case 0:                 \
		case (a):;              \
	} while (0)
#endif

#define NINODES 200

// Disk layout:
// [ boot block | sb block | log | inode blocks | free bit map | data blocks ]

static uint32_t nbitmap = FSSIZE / (BSIZE * 8) + 1;
static uint32_t ninodeblocks = NINODES / IPB + 1;
static uint32_t nlog = LOGSIZE;
static uint32_t nmeta; // Number of meta blocks (boot, sb, nlog, inode, bitmap)
static uint32_t nblocks; // Number of data blocks

static uint32_t fsfd;
static struct superblock sb;
static char zeroes[BSIZE];
static uint32_t freeinode = 1;
static uint32_t freeblock;

static void
balloc(int32_t);
static void
wsect(uint32_t, void *);
static void
winode(uint32_t, struct dinode *);
static void
rinode(uint32_t inum, struct dinode *ip);
static void
rsect(uint32_t sec, void *buf);
static uint32_t
ialloc(uint32_t type);
static void
iappend(uint32_t inum, void *p, uint32_t n);

// convert to intel byte order
static uint16_t
xshort(uint16_t x)
{
	uint16_t y;
	uint8_t *a = (uint8_t *)&y;
	a[0] = x;
	a[1] = x >> 8;
	return y;
}

static uint32_t
xint(uint32_t x)
{
	uint32_t y;
	uint8_t *a = (uint8_t *)&y;
	a[0] = x;
	a[1] = x >> 8;
	a[2] = x >> 16;
	a[3] = x >> 24;
	return y;
}

static uint32_t rootino;
static uint32_t etcino;
static uint32_t binino;

static void
make_file(uint32_t currentino, const char *name, uint32_t parentino)
{
	struct dirent de;
	bzero(&de, sizeof(de));
	de.inum = xshort(currentino);
	strcpy(de.name, name);
	iappend(parentino == 0 ? currentino : parentino, &de, sizeof(de));
}

static uint32_t
make_dir(uint32_t parentino, const char *name)
{
	uint32_t currentino = ialloc(S_IFDIR | S_IAUSR);

	// creates dir
	make_file(currentino, name, parentino);
	make_file(currentino, ".", 0);
	make_file(currentino, "..", 0);

	return currentino;
}

static void
makedirs(void)
{
	binino = make_dir(rootino, "bin");
	etcino = make_dir(rootino, "etc");
}

int
main(int32_t argc, char *argv[])
{
	uint32_t i, cc, fd;
	uint32_t ino, inum, off;
	struct dirent de;
	char buf[BSIZE];
	struct dinode din;
	char *name;

	static_assert(sizeof(int) == 4, "Integers must be 4 bytes!");

	if (argc < 2) {
		fprintf(stderr, "Usage: mkfs fs.img files...\n");
		exit(1);
	}

	printf("sizeof(struct dinode): %lu\n", sizeof(struct dinode));
	assert((BSIZE % sizeof(struct dinode)) == 0);
	assert((BSIZE % sizeof(struct dirent)) == 0);

	fsfd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fsfd < 0) {
		perror(argv[1]);
		exit(1);
	}

	// 1 fs block = 1 disk sector
	nmeta = 2 + nlog + ninodeblocks + nbitmap;
	nblocks = FSSIZE - nmeta;

	sb.size = xint(FSSIZE);
	sb.nblocks = xint(nblocks);
	sb.ninodes = xint(NINODES);
	sb.nlog = xint(nlog);
	sb.logstart = xint(2);
	sb.inodestart = xint(2 + nlog);
	sb.bmapstart = xint(2 + nlog + ninodeblocks);

	printf(
		"nmeta %d (boot, super, log blocks %u inode blocks %u, bitmap blocks %u) blocks %d total %d\n",
		nmeta, nlog, ninodeblocks, nbitmap, nblocks, FSSIZE);

	freeblock = nmeta; // the first free block that we can allocate

	for (i = 0; i < FSSIZE; i++)
		wsect(i, zeroes);

	memset(buf, 0, sizeof(buf));
	memmove(buf, &sb, sizeof(sb));
	wsect(1, buf);

	// make root dir
	rootino = ialloc(S_IFDIR | S_IAUSR);
	assert(rootino == ROOTINO);

	bzero(&de, sizeof(de));
	de.inum = xshort(rootino);
	strcpy(de.name, ".");
	iappend(rootino, &de, sizeof(de));

	bzero(&de, sizeof(de));
	de.inum = xshort(rootino);
	strcpy(de.name, "..");
	iappend(rootino, &de, sizeof(de));

	makedirs();

	for (i = 2; i < argc; i++) {
		//assert(index(argv[i], '/') == 0);

		if ((fd = open(argv[i], 0)) < 0) {
			perror(argv[i]);
			exit(1);
		}

		// Skip leading _ in name when writing to file system.
		// The binaries are named _rm, _cat, etc. to keep the
		// build operating system from trying to execute them
		// in place of system binaries like rm and cat.
		// ../bin/_rm => ../bin/rm
		uint32_t k = 0;
		char *str = malloc(FILENAME_MAX);
		for (int32_t j = 0; j < strlen(argv[i]); j++) {
			if (argv[i][j] != '_') {
				str[k] = argv[i][j];
				k++;
			}
		}
		// add NULL terminator
		str[k] = '\0';
		if (k > 0)
			strcpy(argv[i], str);
		free(str);
		// "../README" => "README"
		// "../bin/rm" => bin/rm
		while (*argv[i] == '.' || *argv[i] == '/') {
			++argv[i];
		}
		if (strncmp(SYSROOT, argv[i], strlen(SYSROOT)) == 0)
			argv[i] += strlen(SYSROOT);

		if (strncmp("bin/", argv[i], 4) == 0) {
			name = (argv[i] += 4);
			ino = binino;
		} else if (strncmp("etc/", argv[i], 4) == 0) {
			name = (argv[i] += 4);
			ino = etcino;
		} else {
			name = argv[i];
			ino = rootino;
		}
		strncpy(de.name, name, DIRSIZ);

		inum = ialloc(S_IFREG | S_IAUSR);

		make_file(inum, name, ino);

		while ((cc = read(fd, buf, sizeof(buf))) > 0)
			iappend(inum, buf, cc);

		close(fd);
	}

	// fix size of root inode dir
	rinode(rootino, &din);
	off = xint(din.size);
	off = ((off / BSIZE) + 1) * BSIZE;
	din.size = xint(off);
	winode(rootino, &din);

	balloc(freeblock);

	exit(0);
}

void
wsect(uint32_t sec, void *buf)
{
	if (lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE) {
		perror("lseek");
		exit(1);
	}
	if (write(fsfd, buf, BSIZE) != BSIZE) {
		perror("write");
		exit(1);
	}
}

static void
winode(uint32_t inum, struct dinode *ip)
{
	char buf[BSIZE];
	uint32_t bn;
	struct dinode *dip;

	bn = IBLOCK(inum, sb);
	rsect(bn, buf);
	dip = ((struct dinode *)buf) + (inum % IPB);
	*dip = *ip;
	wsect(bn, buf);
}

static void
rinode(uint32_t inum, struct dinode *ip)
{
	char buf[BSIZE];
	uint32_t bn;
	struct dinode *dip;

	bn = IBLOCK(inum, sb);
	rsect(bn, buf);
	dip = ((struct dinode *)buf) + (inum % IPB);
	*ip = *dip;
}

static void
rsect(uint32_t sec, void *buf)
{
	if (lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE) {
		perror("lseek");
		exit(1);
	}
	if (read(fsfd, buf, BSIZE) == -1) {
		perror("read");
		exit(1);
	}
}

static uint32_t
ialloc(uint32_t mode)
{
	uint32_t inum = freeinode++;
	struct dinode din;

	bzero(&din, sizeof(din));
	din.nlink = xshort(1);
	din.size = xint(0);
	din.mode = xint(mode);
	din.uid = xshort(DEFAULT_UID);
	din.gid = xshort(DEFAULT_GID);
	din.atime = xint(time(NULL));
	din.ctime = xint(time(NULL));
	din.mtime = xint(time(NULL));
	winode(inum, &din);
	return inum;
}

static void
balloc(int32_t used)
{
	uint8_t buf[BSIZE];
	uint32_t i;

	printf("balloc: first %d blocks have been allocated\n", used);
	assert(used < BSIZE * 8);
	bzero(buf, BSIZE);
	for (i = 0; i < used; i++) {
		buf[i / 8] = buf[i / 8] | (0x1 << (i % 8));
	}
	printf("balloc: write bitmap block at sector %lu\n", sb.bmapstart);
	wsect(sb.bmapstart, buf);
}

#define min(a, b) ((a) < (b) ? (a) : (b))

static void
iappend(uint32_t inum, void *xp, uint32_t n)
{
	char *p = (char *)xp;
	uint32_t fbn, off, n1;
	struct dinode din;
	char buf[BSIZE];
	uint32_t indirect[NINDIRECT];
	uint32_t double_indirect[NINDIRECT];
	uint32_t x;

	rinode(inum, &din);
	off = xint(din.size);
	//printf("append inum %d at off %d sz %d\n", inum, off, n);
	while (n > 0) {
		fbn = off / BSIZE;
		assert(fbn < MAXFILE);
		if (fbn < NDIRECT) {
			if (xint(din.addrs[fbn]) == 0) {
				din.addrs[fbn] = xint(freeblock++);
			}
			x = xint(din.addrs[fbn]);
		} else {
			if (xint(din.addrs[NDIRECT]) == 0) {
				din.addrs[NDIRECT] = xint(freeblock++);
			}
			rsect(xint(din.addrs[NDIRECT]), (char *)indirect);

			if (indirect[(fbn - NDIRECT) / NINDIRECT] == 0) {
				indirect[(fbn - NDIRECT) / NINDIRECT] = xint(freeblock++);
				wsect(xint(din.addrs[NDIRECT]), (char *)indirect);
			}
			//2nd indirect
			rsect(xint(indirect[(fbn - NDIRECT) / NINDIRECT]),
						(char *)double_indirect);
			if (double_indirect[(fbn - NDIRECT) % NINDIRECT] == 0) {
				double_indirect[(fbn - NDIRECT) % NINDIRECT] = xint(freeblock++);
				wsect(xint(indirect[(fbn - NDIRECT) / NINDIRECT]),
							(char *)double_indirect);
			}
			//x is the address of the block to be written
			// so it should correspond to the doubly indirect addr
			x = xint(double_indirect[(fbn - NDIRECT) % NINDIRECT]);
			//}
			if (indirect[fbn - NDIRECT] == 0) {
				indirect[fbn - NDIRECT] = xint(freeblock++);
				wsect(xint(din.addrs[NDIRECT]), (char *)indirect);
			}
			x = xint(indirect[fbn - NDIRECT]);
		}
		n1 = min(n, (fbn + 1) * BSIZE - off);
		rsect(x, buf);
		bcopy(p, buf + off - (fbn * BSIZE), n1);
		wsect(x, buf);
		n -= n1;
		off += n1;
		p += n1;
	}
	din.size = xint(off);
	winode(inum, &din);
}
