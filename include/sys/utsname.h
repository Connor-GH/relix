#pragma once

#include "limits.h"
#define _UTSNAME_LEN 64
struct utsname {
	char sysname[_UTSNAME_LEN];
	char nodename[HOST_NAME_MAX + 1];
	char release[_UTSNAME_LEN];
	char version[_UTSNAME_LEN];
	char machine[_UTSNAME_LEN];
};
int uname(struct utsname *);
