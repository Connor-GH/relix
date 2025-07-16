#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define USE_HOST_TOOLS
#include <stdint.h>
#include <time.h>
#define SYSROOT "sysroot/"

// Avoid name clashes.
#define stat relix_stat
#define dev_t relix_dev_t
#define __time_t __relix_time_t
#define time_t relix_time_t
#define __clock_t __relix_clock_t
#define clock_t relix_clock_t
#define __suseconds_t __relix_suseconds_t
#define suseconds_t relix_suseconds_t
#define __nlink_t __relix_nlink_t
#define nlink_t relix_nlink_t
#define __uid_t __relix_uid_t
#define uid_t relix_uid_t
#define __gid_t __relix_gid_t
#define gid_t relix_gid_t
#define __id_t __relix_id_t
#define id_t relix_id_t
#define __ino_t __relix_ino_t
#define ino_t relix_ino_t
#define __mode_t __relix_mode_t
#define mode_t relix_mode_t
#undef __size_t
#define __size_t __relix_size_t
#define size_t relix_size_t

#define timespec relix_timespec
#define USE_HOST_TOOLS
#include "../include/dirent.h"
#include "../include/sys/stat.h"
#include "../kernel/include/fs.h"
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

static size_t nbitmap = FSSIZE / (BSIZE * 8) + 1;
static size_t ninodeblocks = NINODES / IPB + 1;
static size_t nlog = LOGSIZE;
static size_t nmeta; // Number of meta blocks (boot, sb, nlog, inode, bitmap)
static size_t nblocks; // Number of data blocks

static int fsfd;
static struct superblock sb;
static char zeroes[BSIZE];
static uint32_t freeinode = 1;
static uint64_t freeblock;

static void balloc(size_t);
static void wsect(off_t, void *);
static void winode(uint32_t, struct dinode *);
static void rinode(uint32_t inum, struct dinode *ip);
static void rsect(off_t sec, void *buf);
static uint32_t ialloc(mode_t type);
static void iappend(uint32_t inum, void *p, ssize_t n);

// convert to intel byte order
static uint16_t
xshort(uint16_t x)
{
	uint8_t a[2];
	a[0] = x;
	a[1] = x >> 8;
	memcpy(&x, a, sizeof(a));
	return x;
}

static uint32_t
xint(uint32_t x)
{
	uint8_t a[4];
	a[0] = x;
	a[1] = x >> 8;
	a[2] = x >> 16;
	a[3] = x >> 24;
	memcpy(&x, a, sizeof(a));
	return x;
}

static uint64_t
xlong(uint64_t x)
{
	uint8_t a[8];
	a[0] = x;
	a[1] = x >> 8;
	a[2] = x >> 16;
	a[3] = x >> 24;
	a[4] = x >> 32;
	a[5] = x >> 40;
	a[6] = x >> 48;
	a[7] = x >> 56;
	memcpy(&x, a, sizeof(a));
	return x;
}

static uint32_t rootino;
static uint32_t etcino;
static uint32_t binino;
static uint32_t slashroot_ino;

static void
make_file(uint32_t currentino, const char *name, uint32_t parentino)
{
	struct dirent de;
	bzero(&de, sizeof(de));
	de.d_ino = xshort(currentino);
	strcpy(de.d_name, name);
	iappend(parentino == 0 ? currentino : parentino, &de, sizeof(de));
}

static uint32_t
make_dir(uint32_t parentino, const char *name)
{
	uint32_t currentino = ialloc(S_IFDIR | S_IRWXU);

	// creates dir
	// parentino/name -> currentino
	make_file(currentino, name, parentino);
	// currentino/. -> currentino
	make_file(currentino, ".", 0);
	// currentino/.. -> parentino
	make_file(parentino, "..", currentino);

	return currentino;
}

static void
makedirs(void)
{
	binino = make_dir(rootino, "bin");
	etcino = make_dir(rootino, "etc");
	slashroot_ino = make_dir(rootino, "root");
}

int
main(int32_t argc, char *argv[])
{
	ssize_t cc;
	int fd;
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
	_Static_assert((BSIZE % sizeof(struct dinode)) == 0, "");
	_Static_assert((BSIZE % sizeof(struct dirent)) == 0, "");

	fsfd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fsfd < 0) {
		perror(argv[1]);
		exit(1);
	}

	// 1 fs block = 1 disk sector
	nmeta = 2 + nlog + ninodeblocks + nbitmap;
	nblocks = FSSIZE - nmeta;

	sb.size = xlong(FSSIZE);
	sb.nblocks = xlong(nblocks);
	sb.ninodes = xlong(NINODES);
	sb.nlog = xlong(nlog);
	sb.logstart = xlong(2);
	sb.inodestart = xlong(2 + nlog);
	sb.bmapstart = xlong(2 + nlog + ninodeblocks);

	printf(
		"nmeta %lu (boot, super, log blocks %lu inode blocks %lu, bitmap blocks %lu) blocks %ld total %u\n",
		nmeta, nlog, ninodeblocks, nbitmap, nblocks, FSSIZE);

	freeblock = nmeta; // the first free block that we can allocate

	for (size_t i = 0; i < FSSIZE; i++) {
		wsect(i, zeroes);
	}

	memset(buf, 0, sizeof(buf));
	memmove(buf, &sb, sizeof(sb));
	wsect(1, buf);

	// make root dir
	rootino = ialloc(S_IFDIR | S_IAUSR);
	assert(rootino == ROOTINO);

	bzero(&de, sizeof(de));
	de.d_ino = xshort(rootino);
	strcpy(de.d_name, ".");
	iappend(rootino, &de, sizeof(de));

	bzero(&de, sizeof(de));
	de.d_ino = xshort(rootino);
	strcpy(de.d_name, "..");
	iappend(rootino, &de, sizeof(de));

	makedirs();

	for (size_t i = 2; i < argc; i++) {
		// assert(index(argv[i], '/') == 0);

		if ((fd = open(argv[i], 0)) < 0) {
			perror(argv[i]);
			exit(1);
		}

		// Skip leading _ in name when writing to file system.
		// The binaries are named _rm, _cat, etc. to keep the
		// build operating system from trying to execute them
		// in place of system binaries like rm and cat.
		// ../bin/_rm => ../bin/rm
		size_t k = 0;
		char *str = malloc(FILENAME_MAX);
		for (size_t j = 0; j < strlen(argv[i]); j++) {
			if (argv[i][j] != '_') {
				str[k] = argv[i][j];
				k++;
			}
		}
		// add NULL terminator
		str[k] = '\0';
		if (k > 0) {
			strcpy(argv[i], str);
		}
		free(str);
		// "../README" => "README"
		// "../bin/rm" => bin/rm
		while (*argv[i] == '.' || *argv[i] == '/') {
			++argv[i];
		}
		if (strncmp(SYSROOT, argv[i], strlen(SYSROOT)) == 0) {
			argv[i] += strlen(SYSROOT);
		}

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

		if (strlen(name) > DIRSIZ - 1) {
			fprintf(stderr, "WARNING: filename being truncated: '%s'\n", name);
		}
		strncpy(de.d_name, name, DIRSIZ - 1);
		de.d_name[DIRSIZ - 1] = '\0';

		inum = ialloc(S_IFREG | S_IAUSR);

		make_file(inum, name, ino);

		while ((cc = read(fd, buf, sizeof(buf))) > 0) {
			iappend(inum, buf, cc);
		}

		close(fd);
	}

	// fix size of root inode dir
	rinode(rootino, &din);
	off = xlong(din.size);
	off = ((off / BSIZE) + 1) * BSIZE;
	din.size = xlong(off);
	winode(rootino, &din);

	balloc(freeblock);

	exit(0);
}

void
wsect(ssize_t sec, void *buf)
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
	uint64_t bn;
	struct dinode *dip;

	bn = IBLOCK(inum, sb);
	rsect(bn, buf);
	dip = ((struct dinode *)buf) + (inum % IPB);
	memcpy(dip, ip, sizeof(*dip));
	wsect(bn, buf);
}

static void
rinode(uint32_t inum, struct dinode *ip)
{
	char buf[BSIZE];
	uint64_t bn;
	struct dinode *dip;

	bn = IBLOCK(inum, sb);
	rsect(bn, buf);
	dip = ((struct dinode *)buf) + (inum % IPB);
	*ip = *dip;
}

static void
rsect(off_t sec, void *buf)
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
ialloc(mode_t mode)
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
balloc(size_t used)
{
	uint8_t buf[BSIZE];

	printf("balloc: first %lu blocks have been allocated\n", used);
	assert(used < BSIZE * 8);
	bzero(buf, BSIZE);
	for (size_t i = 0; i < used; i++) {
		buf[i / 8] = buf[i / 8] | (0x1 << (i % 8));
	}
	printf("balloc: write bitmap block at sector %lu\n", sb.bmapstart);
	wsect(sb.bmapstart, buf);
}

#define min(a, b) ((a) < (b) ? (a) : (b))

static void
iappend(uint32_t inum, void *xp, ssize_t n)
{
	char *p = (char *)xp;
	uintptr_t fbn, off, n1;
	struct dinode din;
	char buf[BSIZE];
	uintptr_t indirect[NINDIRECT];
	uintptr_t double_indirect[NINDIRECT * NINDIRECT];
	// The data that we want is stored in x.
	uintptr_t x;

	rinode(inum, &din);
	off = xlong(din.size);
	// printf("append inum %d at off %lu sz %lu\n", inum, off, n);
	while (n > 0) {
		// Block number needed.
		fbn = off / BSIZE;
		assert(fbn < MAXFILE);
		if (fbn < NDIRECT) {
			if (xlong(din.addrs[fbn]) == 0) {
				din.addrs[fbn] = xlong(freeblock++);
			}
			x = xlong(din.addrs[fbn]);
		} else if (fbn < NDIRECT + NINDIRECT) {
			if (xlong(din.addrs[NDIRECT]) == 0) {
				din.addrs[NDIRECT] = xlong(freeblock++);
			}
			rsect(xlong(din.addrs[NDIRECT]), (char *)indirect);
			if (indirect[fbn - NDIRECT] == 0) {
				indirect[fbn - NDIRECT] = xlong(freeblock++);
				wsect(xlong(din.addrs[NDIRECT]), (char *)indirect);
			}
			x = xlong(indirect[fbn - NDIRECT]);
		} else {
			// Doubly indirect block.
			if (xlong(din.addrs[NDIRECT + 1]) == 0) {
				din.addrs[NDIRECT + 1] = xlong(freeblock++);
			}
			rsect(xlong(din.addrs[NDIRECT + 1]), (char *)indirect);

			size_t double_index = (fbn - NDIRECT - NINDIRECT);

			if (indirect[double_index / NINDIRECT] == 0) {
				indirect[double_index / NINDIRECT] = xlong(freeblock++);
				// Write the first level of indirection.
				wsect(xlong(din.addrs[NDIRECT + 1]), (char *)indirect);
			}
			rsect(xlong(indirect[(double_index) / NINDIRECT]),
			      (char *)double_indirect);
			if (xlong(double_indirect[double_index % NINDIRECT]) == 0) {
				double_indirect[double_index % NINDIRECT] = xlong(freeblock++);
				// Write doubly indirect block.
				wsect(xlong(indirect[double_index / NINDIRECT]),
				      (char *)double_indirect);
			}
			x = xlong(double_indirect[double_index % NINDIRECT]);
		}
		n1 = min(n, (fbn + 1) * BSIZE - off);
		rsect(x, buf);
		bcopy(p, buf + off - (fbn * BSIZE), n1);
		wsect(x, buf);
		n -= n1;
		off += n1;
		p += n1;
	}
	din.size = xlong(off);
	winode(inum, &din);
}
