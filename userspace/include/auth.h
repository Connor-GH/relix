#pragma once

int
usertouid(const char *user);
// returns allocated pointer that needs to be freed.
char *
userto_allocated_passwd(const char *user);
