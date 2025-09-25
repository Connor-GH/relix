/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include "private/libc_syscalls.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 1024

DIR *
fdopendir(int fd)
{
	if (fd == -1) {
		return NULL;
	}
	struct dirent de;
	struct __linked_list_dirent *ll = malloc(sizeof(*ll));
	if (ll == NULL) {
		return NULL;
	}
	DIR *dirp = (DIR *)malloc(sizeof(DIR));
	if (dirp == NULL) {
		free(ll);
		return NULL;
	}
	dirp->fd = fd;
	ll->prev = NULL;
	char buf[BUF_SIZE];

	for (;;) {
		ssize_t nread = posix_getdents(fd, buf, BUF_SIZE, 0);
		if (nread == -1) {
			return NULL;
		}

		if (nread == 0) {
			break;
		}

		for (size_t bpos = 0; bpos < nread;) {
			struct posix_dent *d = (struct posix_dent *)(buf + bpos);
			ll->data.d_ino = d->d_ino;
			strncpy(ll->data.d_name, d->d_name, strlen(d->d_name));
			ll->data.d_name[strlen(d->d_name)] = '\0';
			ll->next = malloc(sizeof(*ll));
			ll->next->prev = ll;
			ll = ll->next;

			bpos += d->d_reclen;
		}
	}

	ll->next = NULL;
	ll->data = (struct dirent){ 0, "" };

	// "Rewind" dir
	while (ll->prev != NULL) {
		ll = ll->prev;
	}
	dirp->list = ll;
	return dirp;
}

struct dirent *
readdir(DIR *dirp)
{
	if (dirp == NULL || dirp->list == NULL) {
		errno = EBADF;
		return NULL;
	}
	struct dirent *to_ret = &dirp->list->data;
	dirp->list = dirp->list->next;
	if (strcmp(to_ret->d_name, "") == 0 && to_ret->d_ino == 0) {
		return NULL;
	}
	return to_ret;
}

DIR *
opendir(const char *path)
{
	int fd = open(path, O_RDONLY /*| O_DIRECTORY*/);
	if (fd == -1) {
		return NULL;
	}
	return fdopendir(fd);
}
int
closedir(DIR *dir)
{
	if (dir == NULL || dir->fd == -1) {
		errno = EBADF;
		return -1;
	}
	int rc = close(dir->fd);
	if (rc == 0) {
		dir->fd = -1;
	}
	if (dir->list == NULL) {
		errno = EBADF;
		return -1;
	}
	// Walk to the end of the list.
	while (dir->list != NULL && dir->list->next != NULL) {
		dir->list = dir->list->next;
	}

	// Step back one level.
	if (dir->list != NULL) {
		dir->list = dir->list->prev;
	}

	// Start freeing the list.
	while (dir->list->prev != NULL) {
		free(dir->list->next);
		dir->list = dir->list->prev;
	}
	free(dir->list);
	free(dir);
	return rc;
}

ssize_t
posix_getdents(int fd, void *buf, size_t nbyte, int flags)
{
	return __syscall_ret(
		__syscall4(SYS_getdents, fd, (long)buf, (long)nbyte, flags));
}
