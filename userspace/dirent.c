#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

	while (read(fd, &de, sizeof(de)) == sizeof(de) &&
	       strcmp(de.d_name, "") != 0) {
		ll->data = de;
		ll->next = malloc(sizeof(*ll));
		ll->next->prev = ll;
		ll = ll->next;
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
		errno = -EBADF;
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
