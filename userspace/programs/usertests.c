#include "kernel/include/fs.h"
#include "kernel/include/memlayout.h"
#include "kernel/include/param.h"
#include "kernel/include/syscall.h"
#include "kernel/include/traps.h"
#include <bits/__MAXFILE.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define SKIP_WRITETEST1 1

#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
char buf[8192];
char *echoargv[] = { "/bin/echo", "ALL", "TESTS", "PASSED", NULL };

// does chdir() call iput(p->cwd) in a transaction?
void
iputtest(void)
{
	fprintf(stdout, "iput test\n");

	if (mkdir("iputdir", 0700) < 0) {
		fprintf(stdout, "mkdir failed\n");
		exit(0);
	}
	if (chdir("iputdir") < 0) {
		fprintf(stdout, "chdir iputdir failed\n");
		exit(0);
	}
	if (unlink("../iputdir") < 0) {
		fprintf(stdout, "unlink ../iputdir failed\n");
		exit(0);
	}
	if (chdir("/") < 0) {
		fprintf(stdout, "chdir / failed\n");
		exit(0);
	}
	fprintf(stdout, "iput test ok\n");
}

// does exit(0) call iput(p->cwd) in a transaction?
void
exitiputtest(void)
{
	int pid;

	fprintf(stdout, "exitiput test\n");

	pid = fork();
	if (pid < 0) {
		fprintf(stdout, "fork failed\n");
		exit(0);
	}
	if (pid == 0) {
		if (mkdir("iputdir", 0700) < 0) {
			fprintf(stdout, "mkdir failed\n");
			exit(0);
		}
		if (chdir("iputdir") < 0) {
			fprintf(stdout, "child chdir failed\n");
			exit(0);
		}
		if (unlink("../iputdir") < 0) {
			fprintf(stdout, "unlink ../iputdir failed\n");
			exit(0);
		}
		exit(0);
	}
	wait(NULL);
	fprintf(stdout, "exitiput test ok\n");
}

// does the error path in open() for attempt to write a
// directory call iput() in a transaction?
// needs a hacked kernel that pauses just after the namei()
// call in sys_open():
//    if((ip = namei(path)) == 0)
//      return -1;
//    {
//      int i;
//      for(i = 0; i < 10000; i++)
//        yield();
//    }
void
openiputtest(void)
{
	int pid;

	fprintf(stdout, "openiput test\n");
	if (mkdir("oidir", 0700) < 0) {
		fprintf(stdout, "mkdir oidir failed\n");
		exit(0);
	}
	pid = fork();
	if (pid < 0) {
		fprintf(stdout, "fork failed\n");
		exit(0);
	}
	if (pid == 0) {
		int fd = open("oidir", O_RDWR);
		if (fd >= 0) {
			fprintf(stdout, "open directory for write succeeded\n");
			exit(0);
		}
		exit(0);
	}
	sleep(1);
	if (unlink("oidir") != 0) {
		fprintf(stdout, "unlink failed\n");
		exit(0);
	}
	wait(NULL);
	fprintf(stdout, "openiput test ok\n");
}

// simple file system tests

void
opentest(void)
{
	int fd;

	fprintf(stdout, "open test\n");
	fd = open("/bin/echo", 0);
	if (fd < 0) {
		fprintf(stdout, "open echo failed!\n");
		exit(0);
	}
	close(fd);
	fd = open("doesnotexist", 0);
	if (fd >= 0) {
		fprintf(stdout, "open doesnotexist succeeded!\n");
		exit(0);
	}
	fprintf(stdout, "open test ok\n");
}

void
writetest(void)
{
	int fd;
	int i;

	fprintf(stdout, "small file test\n");
	fd = open("small", O_CREATE | O_RDWR, 0777);
	if (fd >= 0) {
		fprintf(stdout, "creat small succeeded; ok\n");
	} else {
		fprintf(stdout, "error: creat small failed!\n");
		exit(0);
	}
	for (i = 0; i < 100; i++) {
		if (write(fd, "aaaaaaaaaa", 10) != 10) {
			fprintf(stdout, "error: write aa %d new file failed\n", i);
			exit(0);
		}
		if (write(fd, "bbbbbbbbbb", 10) != 10) {
			fprintf(stdout, "error: write bb %d new file failed\n", i);
			exit(0);
		}
	}
	fprintf(stdout, "writes ok\n");
	close(fd);
	fd = open("small", O_RDONLY);
	if (fd >= 0) {
		fprintf(stdout, "open small succeeded ok\n");
	} else {
		fprintf(stdout, "error: open small failed!\n");
		exit(0);
	}
	i = read(fd, buf, 2000);
	if (i == 2000) {
		fprintf(stdout, "read succeeded ok\n");
	} else {
		fprintf(stdout, "read failed\n");
		exit(0);
	}
	close(fd);

	if (unlink("small") < 0) {
		fprintf(stdout, "unlink small failed\n");
		exit(0);
	}
	fprintf(stdout, "small file test ok\n");
}

void
writetest1(void)
{
	char *writetest1_buf =
		malloc(__NDIRECT + __NINDIRECT + __NINDIRECT * __NINDIRECT + 1);
	int fd;
	const size_t writetest_max =
		__NDIRECT + __NINDIRECT + __NINDIRECT * __NINDIRECT + 1;

	fprintf(stdout, "big files test\n");
#if SKIP_WRITETEST1
	fprintf(stdout, "big files skipped\n");
	return;
#endif

	fd = open("big", O_CREATE | O_RDWR, 0777);
	if (fd < 0) {
		fprintf(stdout, "error: creat big failed!\n");
		exit(0);
	}

	for (size_t i = 0; i < writetest_max; i++) {
		((size_t *)writetest1_buf)[0] = i;
		if (write(fd, buf, __BSIZE) != __BSIZE) {
			fprintf(stdout, "error: write big file failed %lu\n", i);
			exit(0);
		}
	}

	close(fd);

	fd = open("big", O_RDONLY);
	if (fd < 0) {
		fprintf(stdout, "error: open big failed!\n");
		exit(0);
	}
	size_t i, n;

	n = 0;
	for (;;) {
		i = read(fd, buf, __BSIZE);
		if (i == 0) {
			if (n == writetest_max - 1) {
				fprintf(stdout, "read only %lu blocks from big", n);
				exit(0);
			}
			break;
		} else if (i != __BSIZE) {
			fprintf(stdout, "read failed %lu\n", i);
			exit(0);
		}
		if (((size_t *)buf)[0] != n) {
			fprintf(stdout, "read content of block %lu is %lu\n", n,
			        ((size_t *)buf)[0]);
			exit(0);
		}
		n++;
	}
	close(fd);
	if (unlink("big") < 0) {
		fprintf(stdout, "unlink big failed\n");
		exit(0);
	}
	fprintf(stdout, "big files ok\n");
}

void
createtest(void)
{
	int i, fd;
	char name[3];

	fprintf(stdout, "many creates, followed by unlink test\n");

	name[0] = 'a';
	name[2] = '\0';
	for (i = 0; i < 52; i++) {
		name[1] = '0' + i;
		fd = open(name, O_CREATE | O_RDWR, 0777);
		close(fd);
	}
	name[0] = 'a';
	name[2] = '\0';
	for (i = 0; i < 52; i++) {
		name[1] = '0' + i;
		unlink(name);
	}
	fprintf(stdout, "many creates, followed by unlink; ok\n");
}

void
dirtest(void)
{
	fprintf(stdout, "mkdir test\n");

	if (mkdir("dir0", 0700) < 0) {
		fprintf(stdout, "mkdir failed\n");
		exit(0);
	}

	if (chdir("dir0") < 0) {
		fprintf(stdout, "chdir dir0 failed\n");
		exit(0);
	}

	if (chdir("..") < 0) {
		fprintf(stdout, "chdir .. failed\n");
		exit(0);
	}

	if (unlink("dir0") < 0) {
		fprintf(stdout, "unlink dir0 failed\n");
		exit(0);
	}
	fprintf(stdout, "mkdir test ok\n");
}

void
exectest(void)
{
	fprintf(stdout, "exec test\n");
	if (execv("/bin/echo", echoargv) < 0) {
		fprintf(stdout, "exec echo failed\n");
		exit(0);
	}
}

// simple fork and pipe read/write

void
pipe1(void)
{
	int fds[2], pid;
	int seq, i, n, cc, total;

	if (pipe(fds) != 0) {
		fprintf(stdout, "pipe() failed\n");
		exit(0);
	}
	pid = fork();
	seq = 0;
	if (pid == 0) {
		close(fds[0]);
		for (n = 0; n < 5; n++) {
			for (i = 0; i < 1033; i++) {
				buf[i] = seq++;
			}
			if (write(fds[1], buf, 1033) != 1033) {
				fprintf(stdout, "pipe1 oops 1\n");
				exit(0);
			}
		}
		exit(0);
	} else if (pid > 0) {
		close(fds[1]);
		total = 0;
		cc = 1;
		while ((n = read(fds[0], buf, cc)) > 0) {
			for (i = 0; i < n; i++) {
				if ((buf[i] & 0xff) != (seq++ & 0xff)) {
					fprintf(stdout, "pipe1 oops 2\n");
					return;
				}
			}
			total += n;
			cc = cc * 2;
			if (cc > sizeof(buf)) {
				cc = sizeof(buf);
			}
		}
		if (total != 5 * 1033) {
			fprintf(stdout, "pipe1 oops 3 total %d\n", total);
			exit(0);
		}
		close(fds[0]);
		wait(NULL);
	} else {
		fprintf(stdout, "fork() failed\n");
		exit(0);
	}
	fprintf(stdout, "pipe1 ok\n");
}

// meant to be run w/ at most two CPUs
void
preempt(void)
{
	int pid1, pid2, pid3;
	int pfds[2];

	fprintf(stdout, "preempt: ");
	pid1 = fork();
	if (pid1 == 0) {
		for (;;)
			;
	}

	pid2 = fork();
	if (pid2 == 0) {
		for (;;)
			;
	}

	pipe(pfds);
	pid3 = fork();
	if (pid3 == 0) {
		close(pfds[0]);
		if (write(pfds[1], "x", 1) != 1) {
			fprintf(stdout, "preempt write error");
		}
		close(pfds[1]);
		for (;;)
			;
	}

	close(pfds[1]);
	if (read(pfds[0], buf, sizeof(buf)) != 1) {
		fprintf(stdout, "preempt read error");
		return;
	}
	close(pfds[0]);
	fprintf(stdout, "kill... ");
	kill(pid1, SIGKILL);
	kill(pid2, SIGKILL);
	kill(pid3, SIGKILL);
	fprintf(stdout, "wait... ");
	wait(NULL);
	wait(NULL);
	wait(NULL);
	fprintf(stdout, "preempt ok\n");
}

// try to find any races between exit and wait
void
exitwait(void)
{
#pragma GCC diagnostic ignored "-Wclobbered"
#pragma GCC diagnostic push
	int i, pid;

	for (i = 0; i < 100; i++) {
		pid = fork();
		if (pid < 0) {
			fprintf(stdout, "fork failed\n");
			return;
		}
		if (pid) {
			if (wait(NULL) != pid) {
				fprintf(stdout, "wait wrong pid\n");
				return;
			}
		} else {
			exit(0);
		}
	}
	fprintf(stdout, "exitwait ok\n");

#pragma GCC diagnostic pop
}

void
largemem(void)
{
	// 200MiB
	fprintf(stderr, "largemem test\n");
	char *ptr = malloc(200 * 1024 * 1024);
	if (ptr == NULL) {
		perror("largemem malloc");
		exit(EXIT_FAILURE);
	}
	srand((unsigned long)ptr);
	for (int i = 0; i < 20 * 1024 * 1024; i++) {
		ptr[i] = rand();
	}
	fprintf(stderr, "%d %d %d\n", ptr[1], ptr[5], ptr[200010]);
	free(ptr);
	fprintf(stderr, "largemem test ok\n");
}
void
mem(void)
{
	void *m1, *m2;
	int pid, ppid;

	fprintf(stdout, "mem test\n");
	ppid = getpid();
	if ((pid = fork()) == 0) {
		m1 = NULL;
		while ((m2 = malloc(10001)) != NULL) {
			*(char **)m2 = m1;
			m1 = m2;
		}
		while (m1) {
			m2 = *(char **)m1;
			free(m1);
			m1 = m2;
		}
		m1 = malloc(1024 * 20);
		if (m1 == NULL) {
			fprintf(stdout, "couldn't allocate mem?!!\n");
			kill(ppid, SIGKILL);
			exit(0);
		}
		free(m1);
		fprintf(stdout, "mem ok\n");
		exit(0);
	} else {
		wait(NULL);
	}
}

// More file system tests

// two processes write to the same file descriptor
// is the offset shared? does inode locking work?
void
sharedfd(void)
{
	int fd, pid, i, n, nc, np;
	char sharedfd_buf[10];

	fprintf(stdout, "sharedfd test\n");

	unlink("sharedfd");
	fd = open("sharedfd", O_CREATE | O_RDWR, 0777);
	if (fd < 0) {
		fprintf(stdout, "fstests: cannot open sharedfd for writing");
		return;
	}
	pid = fork();
	memset(sharedfd_buf, pid == 0 ? 'c' : 'p', sizeof(sharedfd_buf));
	for (i = 0; i < 1000; i++) {
		if (write(fd, sharedfd_buf, sizeof(sharedfd_buf)) != sizeof(sharedfd_buf)) {
			fprintf(stdout, "fstests: write sharedfd failed: %s\n", strerror(errno));
			break;
		}
	}
	if (pid == 0) {
		exit(0);
	} else {
		wait(NULL);
	}
	close(fd);
	fd = open("sharedfd", 0);
	if (fd < 0) {
		fprintf(stdout, "fstests: cannot open sharedfd for reading\n");
		return;
	}
	nc = np = 0;
	while ((n = read(fd, sharedfd_buf, sizeof(sharedfd_buf))) > 0) {
		for (i = 0; i < sizeof(sharedfd_buf); i++) {
			if (sharedfd_buf[i] == 'c') {
				nc++;
			}
			if (sharedfd_buf[i] == 'p') {
				np++;
			}
		}
	}
	close(fd);
	unlink("sharedfd");
	if (nc == 10000 && np == 10000) {
		fprintf(stdout, "sharedfd ok\n");
	} else {
		fprintf(stdout, "sharedfd oops %d %d\n", nc, np);
		exit(0);
	}
}

// four processes write different files at the same
// time, to test block allocation.
void
fourfiles(void)
{
	int fd, pid, i, j, n, total, pi;
	char *names[] = { "f0", "f1", "f2", "f3" };
#pragma GCC diagnostic ignored "-Wclobbered"
#pragma GCC diagnostic push
	char *fname;

	fprintf(stdout, "fourfiles test\n");

	for (pi = 0; pi < 4; pi++) {
		fname = names[pi];
		unlink(fname);

		pid = fork();
		if (pid < 0) {
			fprintf(stdout, "fork failed\n");
			exit(0);
		}

		if (pid == 0) {
			fd = open(fname, O_CREATE | O_RDWR, 0777);
			if (fd < 0) {
				fprintf(stdout, "create failed\n");
				exit(0);
			}

			memset(buf, '0' + pi, 512);
			for (i = 0; i < 12; i++) {
				if ((n = write(fd, buf, 500)) != 500) {
					fprintf(stdout, "write failed %d\n", n);
					exit(0);
				}
			}
			exit(0);
		}
	}

	for (pi = 0; pi < 4; pi++) {
		wait(NULL);
	}

	for (i = 0; i < 2; i++) {
		fname = names[i];
		fd = open(fname, 0);
		total = 0;
		while ((n = read(fd, buf, sizeof(buf))) > 0) {
			for (j = 0; j < n; j++) {
				if (buf[j] != '0' + i) {
					fprintf(stdout, "wrong char\n");
					exit(0);
				}
			}
			total += n;
		}
		close(fd);
		if (total != 12 * 500) {
			fprintf(stdout, "wrong length %d\n", total);
			exit(0);
		}
		unlink(fname);
	}

#pragma GCC diagnostic pop
	fprintf(stdout, "fourfiles ok\n");
}

// four processes create and delete different files in same directory
void
createdelete(void)
{
	enum { N = 20 };
	int pid, i, fd, pi;
	char createdelete_name[32];

	fprintf(stdout, "createdelete test\n");

	for (pi = 0; pi < 4; pi++) {
		pid = fork();
		if (pid < 0) {
			fprintf(stdout, "fork failed\n");
			exit(0);
		}

		if (pid == 0) {
			createdelete_name[0] = 'p' + pi;
			createdelete_name[2] = '\0';
			for (i = 0; i < N; i++) {
				createdelete_name[1] = '0' + i;
				fd = open(createdelete_name, O_CREATE | O_RDWR, 0777);
				if (fd < 0) {
					fprintf(stdout, "create failed\n");
					exit(0);
				}
				close(fd);
				if (i > 0 && (i % 2) == 0) {
					createdelete_name[1] = '0' + (i / 2);
					if (unlink(createdelete_name) < 0) {
						fprintf(stdout, "unlink failed\n");
						exit(0);
					}
				}
			}
			exit(0);
		}
	}

	for (pi = 0; pi < 4; pi++) {
		wait(NULL);
	}

	createdelete_name[0] = createdelete_name[1] = createdelete_name[2] = 0;
	for (i = 0; i < N; i++) {
		for (pi = 0; pi < 4; pi++) {
			createdelete_name[0] = 'p' + pi;
			createdelete_name[1] = '0' + i;
			fd = open(createdelete_name, 0);
			if ((i == 0 || i >= N / 2) && fd < 0) {
				fprintf(stdout, "oops createdelete %s didn't exist\n",
				        createdelete_name);
				exit(0);
			} else if ((i >= 1 && i < N / 2) && fd >= 0) {
				fprintf(stdout, "oops createdelete %s did exist\n", createdelete_name);
				exit(0);
			}
			if (fd >= 0) {
				close(fd);
			}
		}
	}

	for (i = 0; i < N; i++) {
		for (pi = 0; pi < 4; pi++) {
			createdelete_name[0] = 'p' + i;
			createdelete_name[1] = '0' + i;
			unlink(createdelete_name);
		}
	}

	fprintf(stdout, "createdelete ok\n");
}

// can I unlink a file and still read it?
void
unlinkread(void)
{
	int fd, fd1;

	fprintf(stdout, "unlinkread test\n");
	fd = open("unlinkread", O_CREATE | O_RDWR, 0777);
	if (fd < 0) {
		fprintf(stdout, "create unlinkread failed\n");
		exit(0);
	}
	write(fd, "hello", 5);
	close(fd);

	fd = open("unlinkread", O_RDWR);
	if (fd < 0) {
		fprintf(stdout, "open unlinkread failed\n");
		exit(0);
	}
	if (unlink("unlinkread") != 0) {
		fprintf(stdout, "unlink unlinkread failed\n");
		exit(0);
	}

	fd1 = open("unlinkread", O_CREATE | O_RDWR, 0777);
	write(fd1, "yyy", 3);
	close(fd1);

	if (read(fd, buf, sizeof(buf)) != 5) {
		fprintf(stdout, "unlinkread read failed");
		exit(0);
	}
	if (buf[0] != 'h') {
		fprintf(stdout, "unlinkread wrong data\n");
		exit(0);
	}
	if (write(fd, buf, 10) != 10) {
		fprintf(stdout, "unlinkread write failed\n");
		exit(0);
	}
	close(fd);
	unlink("unlinkread");
	fprintf(stdout, "unlinkread ok\n");
}

void
linktest(void)
{
	int fd;

	fprintf(stdout, "linktest\n");

	unlink("lf1");
	unlink("lf2");

	fd = open("lf1", O_CREATE | O_RDWR, 0777);
	if (fd < 0) {
		fprintf(stdout, "create lf1 failed\n");
		exit(0);
	}
	if (write(fd, "hello", 5) != 5) {
		fprintf(stdout, "write lf1 failed\n");
		exit(0);
	}
	close(fd);

	if (link("lf1", "lf2") < 0) {
		fprintf(stdout, "link lf1 lf2 failed\n");
		exit(0);
	}
	unlink("lf1");

	if (open("lf1", 0) >= 0) {
		fprintf(stdout, "unlinked lf1 but it is still there!\n");
		exit(0);
	}

	fd = open("lf2", 0);
	if (fd < 0) {
		fprintf(stdout, "open lf2 failed\n");
		exit(0);
	}
	if (read(fd, buf, sizeof(buf)) != 5) {
		fprintf(stdout, "read lf2 failed\n");
		exit(0);
	}
	close(fd);

	if (link("lf2", "lf2") >= 0) {
		fprintf(stdout, "link lf2 lf2 succeeded! oops\n");
		exit(0);
	}

	unlink("lf2");
	if (link("lf2", "lf1") >= 0) {
		fprintf(stdout, "link non-existant succeeded! oops\n");
		exit(0);
	}

	if (link(".", "lf1") >= 0) {
		fprintf(stdout, "link . lf1 succeeded! oops\n");
		exit(0);
	}

	fprintf(stdout, "linktest ok\n");
}

// test concurrent create/link/unlink of the same file
void
concreate(void)
{
	char file[3];
	int i, pid, n, fd;
	char fa[40];
	struct {
		uint16_t inum;
		char name[14];
	} de;

	fprintf(stdout, "concreate test\n");
	file[0] = 'C';
	file[2] = '\0';
	for (i = 0; i < 40; i++) {
		file[1] = '0' + i;
		unlink(file);
		pid = fork();
		if (pid && (i % 3) == 1) {
			link("C0", file);
		} else if (pid == 0 && (i % 5) == 1) {
			link("C0", file);
		} else {
			fd = open(file, O_CREATE | O_RDWR, 0777);
			if (fd < 0) {
				fprintf(stdout, "concreate create %s failed\n", file);
				exit(0);
			}
			close(fd);
		}
		if (pid == 0) {
			exit(0);
		} else {
			wait(NULL);
		}
	}

	memset(fa, 0, sizeof(fa));
	fd = open(".", 0);
	n = 0;
	while (read(fd, &de, sizeof(de)) > 0) {
		if (de.inum == 0) {
			continue;
		}
		if (de.name[0] == 'C' && de.name[2] == '\0') {
			i = de.name[1] - '0';
			if (i < 0 || i >= sizeof(fa)) {
				fprintf(stdout, "concreate weird file %s\n", de.name);
				exit(0);
			}
			if (fa[i]) {
				fprintf(stdout, "concreate duplicate file %s\n", de.name);
				exit(0);
			}
			fa[i] = 1;
			n++;
		}
	}
	close(fd);

	if (n != 40) {
		fprintf(stdout, "concreate not enough files in directory listing\n");
		exit(0);
	}

	for (i = 0; i < 40; i++) {
		file[1] = '0' + i;
		pid = fork();
		if (pid < 0) {
			fprintf(stdout, "fork failed\n");
			exit(0);
		}
		if (((i % 3) == 0 && pid == 0) || ((i % 3) == 1 && pid != 0)) {
			close(open(file, 0));
			close(open(file, 0));
			close(open(file, 0));
			close(open(file, 0));
		} else {
			unlink(file);
			unlink(file);
			unlink(file);
			unlink(file);
		}
		if (pid == 0) {
			exit(0);
		} else {
			wait(NULL);
		}
	}

	fprintf(stdout, "concreate ok\n");
}

// another concurrent link/unlink/create test,
// to look for deadlocks.
void
linkunlink(void)
{
	int pid, i;

	fprintf(stdout, "linkunlink test\n");

	unlink("x");
	pid = fork();
	if (pid < 0) {
		fprintf(stdout, "fork failed\n");
		exit(0);
	}

	unsigned int x = (pid ? 1 : 97);
	for (i = 0; i < 100; i++) {
		x = x * 1103515245 + 12345;
		if ((x % 3) == 0) {
			close(open("x", O_RDWR | O_CREATE, 0777));
		} else if ((x % 3) == 1) {
			link("cat", "x");
		} else {
			unlink("x");
		}
	}

	if (pid) {
		wait(NULL);
	} else {
		exit(0);
	}

	fprintf(stdout, "linkunlink ok\n");
}

// directory that uses indirect blocks
void
bigdir(void)
{
	int i, fd;
	char bigdir_name[10];

	fprintf(stdout, "bigdir test\n");
	unlink("bd");

	fd = open("bd", O_CREATE, 0777);
	if (fd < 0) {
		fprintf(stdout, "bigdir create failed\n");
		exit(0);
	}
	close(fd);

	for (i = 0; i < 500; i++) {
		bigdir_name[0] = 'x';
		bigdir_name[1] = '0' + (i / 64);
		bigdir_name[2] = '0' + (i % 64);
		bigdir_name[3] = '\0';
		if (link("bd", bigdir_name) != 0) {
			fprintf(stdout, "bigdir link failed\n");
			exit(0);
		}
	}

	unlink("bd");
	for (i = 0; i < 500; i++) {
		bigdir_name[0] = 'x';
		bigdir_name[1] = '0' + (i / 64);
		bigdir_name[2] = '0' + (i % 64);
		bigdir_name[3] = '\0';
		if (unlink(bigdir_name) != 0) {
			fprintf(stdout, "bigdir unlink failed");
			exit(0);
		}
	}

	fprintf(stdout, "bigdir ok\n");
}

void
subdir(void)
{
	int fd, cc;

	fprintf(stdout, "subdir test\n");

	unlink("ff");
	if (mkdir("dd", 0700) != 0) {
		fprintf(stdout, "subdir mkdir dd failed\n");
		exit(0);
	}

	fd = open("dd/ff", O_CREATE | O_RDWR, 0777);
	if (fd < 0) {
		fprintf(stdout, "create dd/ff failed\n");
		exit(0);
	}
	write(fd, "ff", 2);
	close(fd);

	if (unlink("dd") >= 0) {
		fprintf(stdout, "unlink dd (non-empty dir) succeeded!\n");
		exit(0);
	}

	if (mkdir("/dd/dd", 0700) != 0) {
		fprintf(stdout, "subdir mkdir dd/dd failed\n");
		exit(0);
	}

	fd = open("dd/dd/ff", O_CREATE | O_RDWR, 0777);
	if (fd < 0) {
		fprintf(stdout, "create dd/dd/ff failed\n");
		exit(0);
	}
	write(fd, "FF", 2);
	close(fd);

	fd = open("dd/dd/../ff", 0);
	if (fd < 0) {
		fprintf(stdout, "open dd/dd/../ff failed\n");
		exit(0);
	}
	cc = read(fd, buf, sizeof(buf));
	if (cc != 2 || buf[0] != 'f') {
		fprintf(stdout, "dd/dd/../ff wrong content\n");
		exit(0);
	}
	close(fd);

	if (link("dd/dd/ff", "dd/dd/ffff") != 0) {
		fprintf(stdout, "link dd/dd/ff dd/dd/ffff failed\n");
		exit(0);
	}

	if (unlink("dd/dd/ff") != 0) {
		fprintf(stdout, "unlink dd/dd/ff failed\n");
		exit(0);
	}
	if (open("dd/dd/ff", O_RDONLY) >= 0) {
		fprintf(stdout, "open (unlinked) dd/dd/ff succeeded\n");
		exit(0);
	}

	if (chdir("dd") != 0) {
		fprintf(stdout, "chdir dd failed\n");
		exit(0);
	}
	if (chdir("dd/../../dd") != 0) {
		fprintf(stdout, "chdir dd/../../dd failed\n");
		exit(0);
	}
	if (chdir("dd/../../../dd") != 0) {
		fprintf(stdout, "chdir dd/../../dd failed\n");
		exit(0);
	}
	if (chdir("./..") != 0) {
		fprintf(stdout, "chdir ./.. failed\n");
		exit(0);
	}

	fd = open("dd/dd/ffff", 0);
	if (fd < 0) {
		fprintf(stdout, "open dd/dd/ffff failed\n");
		exit(0);
	}
	if (read(fd, buf, sizeof(buf)) != 2) {
		fprintf(stdout, "read dd/dd/ffff wrong len\n");
		exit(0);
	}
	close(fd);

	if (open("dd/dd/ff", O_RDONLY) >= 0) {
		fprintf(stdout, "open (unlinked) dd/dd/ff succeeded!\n");
		exit(0);
	}

	if (open("dd/ff/ff", O_CREATE | O_RDWR, 0777) >= 0) {
		fprintf(stdout, "create dd/ff/ff succeeded!\n");
		exit(0);
	}
	if (open("dd/xx/ff", O_CREATE | O_RDWR, 0777) >= 0) {
		fprintf(stdout, "create dd/xx/ff succeeded!\n");
		exit(0);
	}
	if (open("dd", O_CREATE, 0777) >= 0) {
		fprintf(stdout, "create dd succeeded!\n");
		exit(0);
	}
	if (open("dd", O_RDWR) >= 0) {
		fprintf(stdout, "open dd rdwr succeeded!\n");
		exit(0);
	}
	if (open("dd", O_WRONLY) >= 0) {
		fprintf(stdout, "open dd wronly succeeded!\n");
		exit(0);
	}
	if (link("dd/ff/ff", "dd/dd/xx") == 0) {
		fprintf(stdout, "link dd/ff/ff dd/dd/xx succeeded!\n");
		exit(0);
	}
	if (link("dd/xx/ff", "dd/dd/xx") == 0) {
		fprintf(stdout, "link dd/xx/ff dd/dd/xx succeeded!\n");
		exit(0);
	}
	if (link("dd/ff", "dd/dd/ffff") == 0) {
		fprintf(stdout, "link dd/ff dd/dd/ffff succeeded!\n");
		exit(0);
	}
	if (mkdir("dd/ff/ff", 0700) == 0) {
		fprintf(stdout, "mkdir dd/ff/ff succeeded!\n");
		exit(0);
	}
	if (mkdir("dd/xx/ff", 0700) == 0) {
		fprintf(stdout, "mkdir dd/xx/ff succeeded!\n");
		exit(0);
	}
	if (mkdir("dd/dd/ffff", 0700) == 0) {
		fprintf(stdout, "mkdir dd/dd/ffff succeeded!\n");
		exit(0);
	}
	if (unlink("dd/xx/ff") == 0) {
		fprintf(stdout, "unlink dd/xx/ff succeeded!\n");
		exit(0);
	}
	if (unlink("dd/ff/ff") == 0) {
		fprintf(stdout, "unlink dd/ff/ff succeeded!\n");
		exit(0);
	}
	if (chdir("dd/ff") == 0) {
		fprintf(stdout, "chdir dd/ff succeeded!\n");
		exit(0);
	}
	if (chdir("dd/xx") == 0) {
		fprintf(stdout, "chdir dd/xx succeeded!\n");
		exit(0);
	}

	if (unlink("dd/dd/ffff") != 0) {
		fprintf(stdout, "unlink dd/dd/ff failed\n");
		exit(0);
	}
	if (unlink("dd/ff") != 0) {
		fprintf(stdout, "unlink dd/ff failed\n");
		exit(0);
	}
	if (unlink("dd") == 0) {
		fprintf(stdout, "unlink non-empty dd succeeded!\n");
		exit(0);
	}
	if (unlink("dd/dd") < 0) {
		fprintf(stdout, "unlink dd/dd failed\n");
		exit(0);
	}
	if (unlink("dd") < 0) {
		fprintf(stdout, "unlink dd failed\n");
		exit(0);
	}

	fprintf(stdout, "subdir ok\n");
}

// test writes that are larger than the log.
void
bigwrite(void)
{
	int fd, sz;

	fprintf(stdout, "bigwrite test\n");

	unlink("bigwrite");
	for (sz = 499; sz < 12 * 512; sz += 471) {
		fd = open("bigwrite", O_CREATE | O_RDWR, 0777);
		if (fd < 0) {
			fprintf(stdout, "cannot create bigwrite\n");
			exit(0);
		}
		int i;
		for (i = 0; i < 2; i++) {
			int cc = write(fd, buf, sz);
			if (cc != sz) {
				fprintf(stdout, "write(%d) ret %d\n", sz, cc);
				exit(0);
			}
		}
		close(fd);
		unlink("bigwrite");
	}

	fprintf(stdout, "bigwrite ok\n");
}

void
bigfile(void)
{
	int fd, i, total, cc;

	fprintf(stdout, "bigfile test\n");

	unlink("bigfile");
	fd = open("bigfile", O_CREATE | O_RDWR, 0777);
	if (fd < 0) {
		fprintf(stdout, "cannot create bigfile");
		exit(0);
	}
	for (i = 0; i < 20; i++) {
		memset(buf, i, 600);
		if (write(fd, buf, 600) != 600) {
			fprintf(stdout, "write bigfile failed\n");
			exit(0);
		}
	}
	close(fd);

	fd = open("bigfile", 0);
	if (fd < 0) {
		fprintf(stdout, "cannot open bigfile\n");
		exit(0);
	}
	total = 0;
	for (i = 0;; i++) {
		cc = read(fd, buf, 300);
		if (cc < 0) {
			fprintf(stdout, "read bigfile failed\n");
			exit(0);
		}
		if (cc == 0) {
			break;
		}
		if (cc != 300) {
			fprintf(stdout, "short read bigfile\n");
			exit(0);
		}
		if (buf[0] != i / 2 || buf[299] != i / 2) {
			fprintf(stdout, "read bigfile wrong data\n");
			exit(0);
		}
		total += cc;
	}
	close(fd);
	if (total != 20 * 600) {
		fprintf(stdout, "read bigfile wrong total\n");
		exit(0);
	}
	unlink("bigfile");

	fprintf(stdout, "bigfile test ok\n");
}

void
dirsiz(void)
{
	int fd;

	// 14 is the POSIX minimum, so we check for that.
	fprintf(stdout, "dirsiz test\n");

	if (mkdir("12345678901234", 0700) != 0) {
		fprintf(stdout, "mkdir 12345678901234 failed\n");
		exit(0);
	}
	if (mkdir("12345678901234/123456789012345", 0700) != 0) {
		fprintf(stdout, "mkdir 12345678901234/123456789012345 failed\n");
		exit(0);
	}
	if (mkdir("123456789012345", 0700) != 0) {
		fprintf(stdout, "mkdir 123456789012345 failed\n");
		exit(0);
	}
	if (mkdir("123456789012345/123456789012345", 0700) != 0) {
		fprintf(stdout, "mkdir 123456789012345/123456789012345 failed\n");
		exit(0);
	}
	fd = open("123456789012345/123456789012345/123456789012345", O_CREATE, 0777);
	if (fd < 0) {
		fprintf(stdout,
		        "create 123456789012345/123456789012345/123456789012345 failed\n");
		exit(0);
	}
	close(fd);
	if (mkdir("12345678901234/12345678901234", 0700) != 0) {
		fprintf(stdout, "mkdir 12345678901234/12345678901234 failed\n");
		exit(0);
	}
	fd = open("12345678901234/12345678901234/12345678901234", O_CREATE, 0777);
	if (fd < 0) {
		fprintf(stdout,
		        "open 12345678901234/12345678901234/12345678901234 failed\n");
		exit(0);
	}
	close(fd);

	if (mkdir("123456789012345/12345678901234", 0700) != 0) {
		fprintf(stdout, "mkdir 12345678901234/123456789012345 failed\n");
		exit(0);
	}

	fprintf(stdout, "dirsiz ok\n");
}

void
rmdot(void)
{
	fprintf(stdout, "rmdot test\n");
	if (mkdir("dots", 0700) != 0) {
		fprintf(stdout, "mkdir dots failed\n");
		exit(0);
	}
	if (chdir("dots") != 0) {
		fprintf(stdout, "chdir dots failed\n");
		exit(0);
	}
	if (unlink(".") == 0) {
		fprintf(stdout, "rm . worked!\n");
		exit(0);
	}
	if (unlink("..") == 0) {
		fprintf(stdout, "rm .. worked!\n");
		exit(0);
	}
	if (chdir("/") != 0) {
		fprintf(stdout, "chdir / failed\n");
		exit(0);
	}
	if (unlink("dots/.") == 0) {
		fprintf(stdout, "unlink dots/. worked!\n");
		exit(0);
	}
	if (unlink("dots/..") == 0) {
		fprintf(stdout, "unlink dots/.. worked!\n");
		exit(0);
	}
	if (unlink("dots") != 0) {
		fprintf(stdout, "unlink dots failed!\n");
		exit(0);
	}
	fprintf(stdout, "rmdot ok\n");
}

void
dirfile(void)
{
	int fd;

	fprintf(stdout, "dir vs file\n");

	fd = open("dirfile", O_CREATE, 0777);
	if (fd < 0) {
		fprintf(stdout, "create dirfile failed\n");
		exit(0);
	}
	close(fd);
	if (chdir("dirfile") == 0) {
		fprintf(stdout, "chdir dirfile succeeded!\n");
		exit(0);
	}
	fd = open("dirfile/xx", 0);
	if (fd >= 0) {
		fprintf(stdout, "create dirfile/xx succeeded!\n");
		exit(0);
	}
	fd = open("dirfile/xx", O_CREATE, 0777);
	if (fd >= 0) {
		fprintf(stdout, "create dirfile/xx succeeded!\n");
		exit(0);
	}
	if (mkdir("dirfile/xx", 0700) == 0) {
		fprintf(stdout, "mkdir dirfile/xx succeeded!\n");
		exit(0);
	}
	if (unlink("dirfile/xx") == 0) {
		fprintf(stdout, "unlink dirfile/xx succeeded!\n");
		exit(0);
	}
	if (link("README", "dirfile/xx") == 0) {
		fprintf(stdout, "link to dirfile/xx succeeded!\n");
		exit(0);
	}
	if (unlink("dirfile") != 0) {
		fprintf(stdout, "unlink dirfile failed!\n");
		exit(0);
	}

	fd = open(".", O_RDWR);
	if (fd >= 0) {
		fprintf(stdout, "open . for writing succeeded!\n");
		exit(0);
	}
	fd = open(".", 0);
	if (write(fd, "x", 1) > 0) {
		fprintf(stdout, "write . succeeded!\n");
		exit(0);
	}
	close(fd);

	fprintf(stdout, "dir vs file OK\n");
}

// test that iput() is called at the end of _namei()
void
iref(void)
{
	int i, fd;

	fprintf(stdout, "empty file name\n");

	// the 50 is NINODE
	for (i = 0; i < 50 + 1; i++) {
		if (mkdir("irefd", 0700) != 0) {
			fprintf(stdout, "mkdir irefd failed\n");
			exit(0);
		}
		if (chdir("irefd") != 0) {
			fprintf(stdout, "chdir irefd failed\n");
			exit(0);
		}

		mkdir("", 0700);
		link("README", "");
		fd = open("", O_CREATE, 0777);
		if (fd >= 0) {
			close(fd);
		}
		fd = open("xx", O_CREATE, 0777);
		if (fd >= 0) {
			close(fd);
		}
		unlink("xx");
	}

	chdir("/");
	fprintf(stdout, "empty file name OK\n");
}

// test that fork fails gracefully
// the forktest binary also does this, but it runs out of proc entries first.
// inside the bigger usertests binary, we run out of memory first.
void
forktest(void)
{
	int n, pid;

	fprintf(stdout, "fork test\n");

	for (n = 0; n < 1000; n++) {
		pid = fork();
		if (pid < 0) {
			break;
		}
		if (pid == 0) {
			exit(0);
		}
	}

	if (n == 1000) {
		fprintf(stdout, "fork claimed to work 1000 times!\n");
		exit(0);
	}

	for (; n > 0; n--) {
		if (wait(NULL) < 0) {
			fprintf(stdout, "wait stopped early\n");
			exit(0);
		}
	}

	if (wait(NULL) != -1) {
		fprintf(stdout, "wait got too many\n");
		exit(0);
	}

	fprintf(stdout, "fork test OK\n");
}

void
sbrktest(void)
{
	int fds[2], pid, pids[10], ppid;
	char *a, *b, *c, *lastaddr, *oldbrk, *p, scratch;
	uint32_t amt;

	fprintf(stdout, "sbrk test\n");
	oldbrk = sbrk(0);

	// can one sbrk() less than a page?
	a = sbrk(0);
	int i;
	for (i = 0; i < 5000; i++) {
		b = sbrk(1);
		if (b != a) {
			fprintf(stdout, "sbrk test failed %d %p %p\n", i, a, b);
			exit(0);
		}
		*b = 1;
		a = b + 1;
	}
	pid = fork();
	if (pid < 0) {
		fprintf(stdout, "sbrk test fork failed\n");
		exit(0);
	}
	c = sbrk(1);
	c = sbrk(1);
	if (c != a + 1) {
		fprintf(stdout, "sbrk test failed post-fork\n");
		exit(0);
	}
	if (pid == 0) {
		exit(0);
	}
	wait(NULL);

	// can one grow address space to something big?
#define BIG (100 * 1024 * 1024)
	a = sbrk(0);
	amt = (BIG) - (uintptr_t)a;
	p = sbrk(amt);
	if (p != a) {
		fprintf(stdout,
		        "sbrk test failed to grow big address space; enough phys mem?\n");
		exit(0);
	}
	lastaddr = (char *)(BIG - 1);
	*lastaddr = 99;

	// can one de-allocate?
	a = sbrk(0);
	c = sbrk(-4096);
	if (c == (char *)0xffffffff) {
		fprintf(stdout, "sbrk could not deallocate\n");
		exit(0);
	}
	c = sbrk(0);
	if (c != a - 4096) {
		fprintf(stdout, "sbrk deallocation produced wrong address, a %p c %p\n", a,
		        c);
		exit(0);
	}

	// can one re-allocate that page?
	a = sbrk(0);
	c = sbrk(4096);
	if (c != a || sbrk(0) != a + 4096) {
		fprintf(stdout, "sbrk re-allocation failed, a %p c %p\n", a, c);
		exit(0);
	}
	if (*lastaddr == 99) {
		// should be zero
		fprintf(stdout, "sbrk de-allocation didn't really deallocate\n");
		exit(0);
	}

	a = sbrk(0);
	c = sbrk(-((char *)sbrk(0) - oldbrk));
	if (c != a) {
		fprintf(stdout, "sbrk downsize failed, a %p c %p\n", a, c);
		exit(0);
	}

	// can we read the kernel's memory?
	for (a = (char *)(KERNBASE); a < (char *)(KERNBASE + 2000000); a += 50000) {
		ppid = getpid();
		pid = fork();
		if (pid < 0) {
			fprintf(stdout, "fork failed\n");
			exit(0);
		}
		if (pid == 0) {
			fprintf(stdout, "oops could read %p = %d\n", a, *a);
			kill(ppid, SIGKILL);
			exit(0);
		}
		wait(NULL);
	}

	// if we run the system out of memory, does it clean up the last
	// failed allocation?
	if (pipe(fds) != 0) {
		fprintf(stdout, "pipe() failed\n");
		exit(0);
	}
	for (i = 0; i < sizeof(pids) / sizeof(pids[0]); i++) {
		if ((pids[i] = fork()) == 0) {
			// allocate a lot of memory
			sbrk(BIG - (uintptr_t)sbrk(0));
			write(fds[1], "x", 1);
			// sit around until killed
			for (;;) {
				sleep(1000);
			}
		}
		if (pids[i] != -1) {
			read(fds[0], &scratch, 1);
		}
	}
	// if those failed allocations freed up the pages they did allocate,
	// we'll be able to allocate here
	c = sbrk(4096);
	for (i = 0; i < sizeof(pids) / sizeof(pids[0]); i++) {
		if (pids[i] == -1) {
			continue;
		}
		kill(pids[i], SIGKILL);
		wait(NULL);
	}
	if (c == (char *)0xffffffff) {
		fprintf(stdout, "failed sbrk leaked memory\n");
		exit(0);
	}

	if ((char *)sbrk(0) > oldbrk) {
		sbrk(-((char *)sbrk(0) - oldbrk));
	}

	fprintf(stdout, "sbrk test OK\n");
}

void
validateint(__attribute__((unused)) int *p)
{
	int res;
	asm("mov %%esp, %%ebx\n\t"
	    "mov %2, %%rsp\n\t"
	    "syscall\n\t"
	    "mov %%ebx, %%esp"
	    : "=a"(res)
	    : "a"(SYS_alarm), "c"(p), "d"(1)
	    : "ebx");
}

void
validatetest(void)
{
	int hi, pid;
	uintptr_t p;

	fprintf(stdout, "validate test\n");
	hi = 1100 * 1024;

	for (p = 0; p <= (uint32_t)hi; p += 4096) {
		if ((pid = fork()) == 0) {
			// try to crash the kernel by passing in a badly placed integer
			validateint((int *)p);
			exit(0);
		}
		sleep(0);
		sleep(0);
		kill(pid, SIGKILL);
		wait(NULL);

		// try to crash the kernel by passing in a bad string pointer
		if (link("nosuchfile", (char *)p) != -1) {
			fprintf(stdout, "link should not succeed\n");
			exit(0);
		}
	}

	fprintf(stdout, "validate ok\n");
}

// does unintialized data start out zero?
char uninit[10000];
void
bsstest(void)
{
	int i;

	fprintf(stdout, "bss test\n");
	for (i = 0; i < sizeof(uninit); i++) {
		if (uninit[i] != '\0') {
			fprintf(stdout, "bss test failed\n");
			exit(0);
		}
	}
	fprintf(stdout, "bss test ok\n");
}

// does exec return an error if the arguments
// are larger than a page? or does it write
// below the stack and wreck the instructions/data?
void
bigargtest(void)
{
	int pid, fd;

	unlink("bigarg-ok");
	pid = fork();
	if (pid == 0) {
		static char *args[MAXARG];
		int i;
		for (i = 0; i < MAXARG - 1; i++) {
			args[i] =
				"bigargs test: failed\n                                                                                                                                                                                                       ";
		}
		args[MAXARG - 1] = NULL;
		fprintf(stdout, "bigarg test\n");
		execv("echo", args);
		fprintf(stdout, "bigarg test ok\n");
		fd = open("bigarg-ok", O_CREATE, 0777);
		close(fd);
		exit(0);
	} else if (pid < 0) {
		fprintf(stdout, "bigargtest: fork failed\n");
		exit(0);
	}
	wait(NULL);
	fd = open("bigarg-ok", 0);
	if (fd < 0) {
		fprintf(stdout, "bigarg test failed!\n");
		exit(0);
	}
	close(fd);
	unlink("bigarg-ok");
}

// what happens when the file system runs out of blocks?
// answer: balloc panics, so this test is not useful.
void
fsfull(void)
{
	int nfiles;

	fprintf(stdout, "fsfull test\n");

	for (nfiles = 0;; nfiles++) {
		char fsfull_name[64];
		fsfull_name[0] = 'f';
		fsfull_name[1] = '0' + nfiles / 1000;
		fsfull_name[2] = '0' + (nfiles % 1000) / 100;
		fsfull_name[3] = '0' + (nfiles % 100) / 10;
		fsfull_name[4] = '0' + (nfiles % 10);
		fsfull_name[5] = '\0';
		fprintf(stdout, "writing %s\n", fsfull_name);
		int fd = open(fsfull_name, O_CREATE | O_RDWR, 0777);
		if (fd < 0) {
			fprintf(stdout, "open %s failed\n", fsfull_name);
			break;
		}
		int total = 0;
		while (1) {
			int cc = write(fd, buf, 512);
			if (cc < 512) {
				break;
			}
			total += cc;
		}
		fprintf(stdout, "wrote %d bytes\n", total);
		close(fd);
		if (total == 0) {
			break;
		}
	}

	while (nfiles >= 0) {
		char fsfull_name[64];
		fsfull_name[0] = 'f';
		fsfull_name[1] = '0' + nfiles / 1000;
		fsfull_name[2] = '0' + (nfiles % 1000) / 100;
		fsfull_name[3] = '0' + (nfiles % 100) / 10;
		fsfull_name[4] = '0' + (nfiles % 10);
		fsfull_name[5] = '\0';
		unlink(fsfull_name);
		nfiles--;
	}

	fprintf(stdout, "fsfull test finished\n");
}

void
uio(void)
{
#define RTC_ADDR 0x70
#define RTC_DATA 0x71

	uint16_t port = 0;
	uint8_t val = 0;
	int pid;

	fprintf(stdout, "uio test\n");
	pid = fork();
	if (pid == 0) {
		port = RTC_ADDR;
		val = 0x09; /* year */
		/* http://wiki.osdev.org/Inline_Assembly/Examples */
		asm volatile("outb %0,%1" ::"a"(val), "d"(port));
		port = RTC_DATA;
		asm volatile("inb %1,%0" : "=a"(val) : "d"(port));
		fprintf(stdout, "uio: uio succeeded; test FAILED\n");
		exit(0);
	} else if (pid < 0) {
		fprintf(stdout, "fork failed\n");
		exit(0);
	}
	wait(NULL);
	fprintf(stdout, "uio test done\n");
}

void
argptest(void)
{
	int fd;
	fd = open("/bin/init", O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "open failed\n");
		exit(0);
	}
	read(fd, sbrk(0) - 1, -1);
	close(fd);
	fprintf(stdout, "arg test passed\n");
}

int
main(int argc, char *argv[])
{
	fprintf(stdout, "usertests starting\n");

	if (open("usertests.ran", 0) >= 0) {
		fprintf(stdout, "already ran user tests -- rebuild fs.img\n");
		exit(0);
	}
	close(open("usertests.ran", O_CREATE, 0777));
	if (argc > 1) {
		if (strcmp(argv[1], "iotest") == 0) {
			if (argc == 3 && strcmp(argv[2], "full") == 0) {
				linktest();
				unlinkread();
				createdelete();
				linkunlink();
			}
			concreate();
			bigwrite();
			writetest();
			writetest1();
			bigdir();
			return 0;
		}
		if (strcmp(argv[1], "memtest") == 0) {
			largemem();
			sbrktest();
			mem();
			return 0;
		}
	}

	argptest();
	createdelete();
	linkunlink();
	concreate();
	fourfiles();
	sharedfd();

	bigargtest();
	bigwrite();
	bigargtest();
	bsstest();
	sbrktest();
	validatetest();

	opentest();
	writetest();
	writetest1();
	createtest();

	openiputtest();
	exitiputtest();
	iputtest();

	mem();
	pipe1();
	preempt();
	exitwait();

	rmdot();
	dirsiz();
	bigfile();
	subdir();
	linktest();
	unlinkread();
	dirfile();
	iref();
	forktest();
	bigdir(); // slow

#ifndef X86_64
	uio();
#endif

	exectest();
	exit(0);
}
#pragma GCC diagnostic pop
