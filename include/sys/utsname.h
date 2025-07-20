#pragma once

#define _UTSNAME_LEN 64
struct utsname {
	char sysname[_UTSNAME_LEN];
	char nodename[_UTSNAME_LEN];
	char release[_UTSNAME_LEN];
	char version[_UTSNAME_LEN];
	char machine[_UTSNAME_LEN];
};
int uname(struct utsname *);
