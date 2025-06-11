#pragma once
#include <sys/types.h>

// POSIX definition of passwd
struct passwd {
	char *pw_name;
	uid_t pw_uid;
	gid_t pw_gid;
	char *pw_dir;
	char *pw_shell;
};

void setpwent(void);
void endpwent(void);

struct passwd *getpwent(void);

struct passwd *getpwuid(uid_t uid);
struct passwd *getpwnam(const char *name);
