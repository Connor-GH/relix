#include <fcntl.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static FILE *s_file_passwd = NULL;
static struct passwd s_passwd_entry;
static char s_buf[1024] = { 0 };

void
endpwent(void)
{
	if (s_file_passwd != NULL) {
		fclose(s_file_passwd);
		s_file_passwd = NULL;
	}
}

void
setpwent(void)
{
	if (s_file_passwd == NULL) {
		s_file_passwd = fopen("/etc/passwd", "r");

		if (s_file_passwd == NULL) {
			perror("fopen /etc/passwd");
		}
	}
	rewind(s_file_passwd);
}

struct passwd *
getpwent(void)
{
	int fd;
	memset(s_buf, 0, sizeof(s_buf));
	size_t n;

	char *ret = fgets(s_buf, sizeof(s_buf), s_file_passwd);
	if (ret == NULL) {
		return NULL;
	}

	// Get rid of the newline at the end of the buffer.
	s_buf[strcspn(s_buf, "\n")] = '\0';

	char *username = strtok(s_buf, ":");
	char *password = strtok(NULL, ":");

	char *s_uid = strtok(NULL, ":");
	uid_t this_uid = strtoll(s_uid, NULL, 10);

	char *s_gid = strtok(NULL, ":");
	gid_t this_gid = strtoll(s_gid, NULL, 10);

	char *home = strtok(NULL, ":");
	char *shell = strtok(NULL, ":");

	s_passwd_entry.pw_name = username;
	s_passwd_entry.pw_uid = this_uid;
	s_passwd_entry.pw_gid = this_gid;
	s_passwd_entry.pw_dir = home;
	s_passwd_entry.pw_shell = shell;

	return &s_passwd_entry;
}

struct passwd *
getpwuid(uid_t uid)
{
	setpwent();
	struct passwd *entry;
	while ((entry = getpwent()) != NULL) {
		if (entry->pw_uid == uid) {
			endpwent();
			return entry;
		}
	}
	endpwent();
	return NULL;
}

struct passwd *
getpwnam(const char *name)
{
	setpwent();
	struct passwd *entry;
	while ((entry = getpwent()) != NULL) {
		if (strcmp(entry->pw_name, name) == 0) {
			endpwent();
			return entry;
		}
	}
	endpwent();
	return NULL;
}
